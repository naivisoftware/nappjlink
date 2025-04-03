/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local includes
#include "pjlinkconnection.h"
#include "pjlinkprojector.h"
#include "pjlinkcommand.h"

// External includes
#include <asio/connect.hpp>
#include <asio/read.hpp>
#include <nap/logger.h>
#include <asio/read_until.hpp>
#include <asio/write.hpp>
#include <asio/use_future.hpp>
#include <asio/defer.hpp>

using namespace asio::ip;

namespace nap
{
	PJLinkConnection::PJLinkConnection(pjlink::Context& context, const asio::ip::address& address, PJLinkProjector& projector) :
		mSocket(context),
		mProjector(projector),
		mAddress(address)
	{ }


	std::shared_ptr<nap::PJLinkConnection> PJLinkConnection::create(pjlink::Context& context, const asio::ip::address& address, PJLinkProjector& projector)
	{
		return std::shared_ptr<PJLinkConnection>(new PJLinkConnection(context, address, projector));
	}


	std::future<bool> PJLinkConnection::connect()
	{
		mEndpoint = tcp::endpoint(mAddress, pjlink::port);
		auto handle = shared_from_this();
		auto cf = mSocket.async_connect(mEndpoint, asio::use_future([handle](std::error_code ec)
			{
				// Handle error
				if (ec)
				{
					nap::Logger::error("Failed (ec '%d') to connect to projector '%s', endpoint: %s, port: %d",
						ec.value(),
						handle->mProjector.mID.c_str(),
						handle->mAddress.to_string().c_str(),
						handle->mEndpoint.port());
					return false;
				}

				// Connection success -> verify authentification
				nap::Logger::info("%s: Connected to projector '%s', port: %d",
					handle->mAddress.to_string().c_str(),
					handle->mProjector.mID.c_str(),
					handle->mEndpoint.port());

				// Authenticate
				if (handle->authenticate())
				{
					// Start running timeout timer
					handle->setTimer();
					return true;
				}
				return false;
			}));
		return cf;
	}


	std::future<void> PJLinkConnection::disconnect()
	{
		// Schedule task to close socket when connected
		auto handle = shared_from_this();
		auto f = asio::post(mSocket.get_executor(), asio::use_future([handle]
			{
				// Cancel and delete timers
				handle->mTimeout.reset(nullptr);

				// Bail if we're already shut down
				if (!handle->mSocket.is_open())
					return;

				// Close socket
				std::error_code ec;
				handle->mSocket.close(ec);
				if (ec)
				{
					nap::Logger::error("Close request failed (ec '%d'), projector endpoint : %s",
						ec, handle->mAddress.to_string().c_str());
					return;
				}
			}
		));
		return f;
	}


	bool PJLinkConnection::authenticate()
	{
		std::error_code ec;
		auto size = asio::read_until(mSocket, mAuthBuffer, pjlink::terminator, ec);
		if (ec)
		{
			nap::Logger::error("Failed (ec '%d') to authorize projector at endpoint: %s",
				ec.value(), mAddress.to_string().c_str());

			close();
			return false;
		}

		// Commit to string
		nap::Logger::info("%s: Received %d authorization bytes", mAddress.to_string().c_str(), size);
		std::istream is(&mAuthBuffer); std::string response;
		std::getline(std::istream(&mAuthBuffer), response, pjlink::terminator);

		// Ensure it's an authentication header
		if (!utility::startsWith(response, pjlink::response::authenticate::header, false))
		{
			nap::Logger::error("Projector '%s' authentication failed, invalid response: %s",
				mAddress.to_string().c_str(), response.c_str());

			close();
			return false;
		}

		// Ensure authentication is diabled
		if (!utility::startsWith(response, pjlink::response::authenticate::disabled, false))
		{
			nap::Logger::error("Projector authentication requested -> not supported, \
						disable authentication at endpoint: %s",
				mAddress.to_string().c_str());

			close();
			return true;
		}

		// All good
		nap::Logger::info("%s: Authentication succeeded",
			mAddress.to_string().c_str());

		// Start connection timer
		return true;
	}


	void PJLinkConnection::send(PJLinkCommand&& command)
	{
		// Submit task for execution -> it is queued and called from the socket execution thread
		auto handle = shared_from_this();
		asio::post(mSocket.get_executor(), [handle, cmd = std::move(command)]
			{
				bool writing = !handle->mCmds.empty();
				handle->mCmds.emplace(std::move(cmd));
				if (!writing)
					handle->write();
			}
		);
	}


	void PJLinkConnection::write()
	{
		assert(!mCmds.empty()); 
		auto& cmd_ref = mCmds.front();
		auto write_buffer = asio::buffer(cmd_ref.data(), cmd_ref.size());

		nap::Logger::info("%s: Writing '%s'", mAddress.to_string().c_str(),
			cmd_ref.getCommand().substr(0, cmd_ref.size() - 1).c_str());

		assert(mSocket.is_open());
		auto handle = shared_from_this();
		asio::async_write(mSocket, write_buffer, [handle](std::error_code ec, std::size_t size)
			{
				// Writing failed
				if (ec)
				{
					nap::Logger::error("Writing failed (ec '%d'), projector endpoint: %s",
						ec.value(), handle->mAddress.to_string().c_str());

					handle->close();
					return;
				}

				// Writing succeeded -> schedule a response read before attempting a new write
				nap::Logger::info("%s: Written %d byte(s)", handle->mAddress.to_string().c_str(), size);
				auto current = handle->mCmds.front(); handle->mCmds.pop();
				handle->read(std::move(current));

				// Continue if there are more commands
				if (!handle->mCmds.empty())
					handle->write();
			});
	}


	void PJLinkConnection::read(PJLinkCommand&& source_cmd)
	{
		assert(mSocket.is_open());
		auto handle = shared_from_this();
		asio::async_read_until(mSocket, mRespBuffer, pjlink::terminator, [handle, src = std::move(source_cmd)] (std::error_code ec, std::size_t size)
			{
				if (ec)
				{
					nap::Logger::error("Reading failed (ec '%d'), projector endpoint : %s",
						ec.value(), handle->mAddress.to_string().c_str());

					handle->close();
					return;
				}

				// Read succeeded
				nap::Logger::info("%s: Read %d byte(s)", handle->mAddress.to_string().c_str(), size);

				// Commit response from buffer input to response 
				PJLinkCommand reply(src);
				std::istream is(&handle->mRespBuffer);
				std::getline(std::istream(&handle->mRespBuffer), reply.mResponse, pjlink::terminator);

				// All good
				nap::Logger::info("%s: Reply '%s', cmd: '%s'",
					handle->mAddress.to_string().c_str(),
					reply.mResponse.substr(0, reply.mResponse.size()-1).c_str(),
					reply.mCommand.substr(0, reply.mCommand.size()-1).c_str());

				// Reset timer
				handle->setTimer();
			});
	}


	void PJLinkConnection::close()
	{
		// Delete timer -> bail if closed
		mTimeout.reset(nullptr);

		// Close -> must be open when called deferred
		assert(mSocket.is_open()); std::error_code ec;
		mSocket.close(ec);
		if (ec)
		{
			nap::Logger::error("Close request failed (ec '%d'), projector endpoint : %s",
				ec.value(), mAddress.to_string().c_str());
			return;
		}

		// Cancel outstanding timing operations
		nap::Logger::info("%s: Connection closed", mAddress.to_string().c_str());
		mProjector.connectionClosed();
	}


	void PJLinkConnection::timeout(const std::error_code& ec)
	{
		if (!ec)
		{
			nap::Logger::info("%s: Connection timed out", mAddress.to_string().c_str());
			assert(mSocket.is_open());
			close();
		}
	}


	void PJLinkConnection::setTimer()
	{
		mTimeout = std::make_unique<asio::steady_timer>(mSocket.get_executor(), nap::Seconds(20));
		mTimeout->async_wait(
			std::bind(&PJLinkConnection::timeout, shared_from_this(), std::placeholders::_1)
		);
	}
}
