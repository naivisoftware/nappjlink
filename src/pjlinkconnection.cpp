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
	PJLinkConnection::PJLinkConnection(pjlink::Context& context, PJLinkProjector& projector) :
		mSocket(context), mProjector(projector), mTimeout(context, nap::Seconds(20))
	{ }


	std::future<bool> PJLinkConnection::connect()
	{
		mEndpoint = mEndpoint = tcp::endpoint(address::from_string(mProjector.mIPAddress), pjlink::port);
		auto cf = mSocket.async_connect(mEndpoint, asio::use_future([this, ep = mEndpoint, id = mProjector.mID](std::error_code ec)
			{
				if (ec)
				{
					nap::Logger::error("Failed (ec '%d') to connect to projector '%s', endpoint: %s, port: %d",
						ec.value(), id.c_str(), ep.address().to_string().c_str(), ep.port());
					return false;
				}

				// Connection success -> verify authentification
				nap::Logger::info("%s: Connected to projector '%s', port: %d",
					ep.address().to_string().c_str(), id.c_str(), ep.port());

				// Authenticate
				if (authenticate(ep))
				{
					mTimeout.async_wait(std::bind(&PJLinkConnection::timeout, this, std::placeholders::_1));
					return true;
				}
				return false;
			}));
		return cf;
	}


	std::future<void> PJLinkConnection::disconnect()
	{
		// Schedule task to close socket when connected
		auto f = asio::post(mSocket.get_executor(), asio::use_future([this, ep = mEndpoint]
			{
				// Reset timers
				if (!mSocket.is_open())
					return;

				// Close
				mTimeout.cancel(); 
				std::error_code ec;
				mSocket.close(ec);
				if (ec)
				{
					nap::Logger::error("Close request failed (ec '%d'), projector endpoint : %s",
						ec.value(), ep.address().to_string().c_str());
					return;
				}
				nap::Logger::info("%s: Connection closed", ep.address().to_string().c_str());
			}
		));
		return f;
	}


	bool PJLinkConnection::authenticate(pjlink::EndPoint ep)
	{
		std::error_code ec;
		auto size = asio::read_until(mSocket, mAuthBuffer, pjlink::terminator, ec);
		if (ec)
		{
			nap::Logger::error("Failed (ec '%d') to authorize projector at endpoint: %s",
				ec.value(), ep.address().to_string().c_str());

			close(ep);
			return false;
		}

		// Commit to string
		nap::Logger::info("%s: Received %d authorization bytes", ep.address().to_string().c_str(), size);
		std::istream is(&mAuthBuffer); std::string response;
		std::getline(std::istream(&mAuthBuffer), response, pjlink::terminator);

		// Ensure it's an authentication header
		if (!utility::startsWith(response, pjlink::response::authenticate::header, false))
		{
			nap::Logger::error("Projector '%s' authentication failed, invalid response: %s",
				ep.address().to_string().c_str(), response.c_str());

			close(ep);
			return false;
		}

		// Ensure authentication is diabled
		if (!utility::startsWith(response, pjlink::response::authenticate::disabled, false))
		{
			nap::Logger::error("Projector authentication requested -> not supported, \
						disable authentication at endpoint: %s",
				ep.address().to_string().c_str());

			close(ep);
			return true;
		}

		// All good
		nap::Logger::info("%s: Authentication succeeded",
			mEndpoint.address().to_string().c_str());

		// Start connection timer
		return true;
	}


	void PJLinkConnection::send(PJLinkCommand&& command)
	{
		// Submit task for execution -> it is queued and called from the socket execution thread
		asio::post(mSocket.get_executor(), [this, ep = mEndpoint, cmd = std::move(command)]
			{
				bool writing = !mCmds.empty();
				mCmds.emplace(std::move(cmd));
				if (!writing)
				{
					write(ep);
				}
			}
		);
	}


	void PJLinkConnection::write(pjlink::EndPoint ep)
	{
		assert(!mCmds.empty()); 
		auto& cmd_ref = mCmds.front();
		auto write_buffer = asio::buffer(cmd_ref.data(), cmd_ref.size());

		nap::Logger::info("%s: Writing '%s'", ep.address().to_string().c_str(),
			cmd_ref.getCommand().substr(0, cmd_ref.size() - 1).c_str());

		assert(mSocket.is_open());
		asio::async_write(mSocket, write_buffer, [this, ep](std::error_code ec, std::size_t size)
			{
				// Writing failed
				if (ec)
				{
					nap::Logger::error("Writing failed (ec '%d'), projector endpoint: %s",
						ec.value(), ep.address().to_string().c_str());

					close(ep);
					return;
				}

				// Writing succeeded -> schedule a response read before attempting a new write
				nap::Logger::info("%s: Written %d byte(s)", ep.address().to_string().c_str(), size);
				auto current = mCmds.front(); mCmds.pop();
				read(ep, std::move(current));

				// Continue if there are more commands
				if (!mCmds.empty())
				{
					write(ep);
				}
			});
	}


	void PJLinkConnection::read(pjlink::EndPoint ep, PJLinkCommand&& source_cmd)
	{
		assert(mSocket.is_open());
		asio::async_read_until(mSocket, mRespBuffer, pjlink::terminator, [this, ep, src = std::move(source_cmd)] (std::error_code ec, std::size_t size)
			{
				if (ec)
				{
					nap::Logger::error("Reading failed (ec '%d'), projector endpoint : %s",
						ec.value(), ep.address().to_string().c_str());

					close(ep);
					return;
				}

				// Read succeeded
				nap::Logger::info("%s: Read %d byte(s)", ep.address().to_string().c_str(), size);

				// Commit response from buffer input to response 
				PJLinkCommand reply(src);
				std::istream is(&mRespBuffer);
				std::getline(std::istream(&mRespBuffer), reply.mResponse, pjlink::terminator);

				// All good
				nap::Logger::info("%s: Reply '%s', cmd: '%s'",
					mEndpoint.address().to_string().c_str(),
					reply.mResponse.substr(0, reply.mResponse.size()-1).c_str(),
					reply.mCommand.substr(0, reply.mCommand.size()-1).c_str());

				// Reset timer
				mTimeout.cancel();
				mTimeout = asio::steady_timer(mSocket.get_executor(), nap::Seconds(20));
				mTimeout.async_wait(std::bind(&PJLinkConnection::timeout, this, std::placeholders::_1));
			});
	}


	void PJLinkConnection::close(pjlink::EndPoint ep)
	{
		std::error_code ec;
		assert(mSocket.is_open());
		mSocket.close(ec);
		if (ec)
		{
			nap::Logger::error("Close request failed (ec '%d'), projector endpoint : %s",
				ec.value(), ep.address().to_string().c_str());
			return;
		}

		// Connection success -> verify authentification
		nap::Logger::info("%s: Connection closed",ep.address().to_string().c_str());
		mProjector.connectionClosed();
	}


	void PJLinkConnection::timeout(const std::error_code& ec)
	{
		if (!ec)
		{
			nap::Logger::info("%s: Connection timed out", mEndpoint.address().to_string().c_str());
			assert(mSocket.is_open());
			close(mEndpoint);
		}
	}


	PJLinkConnection::~PJLinkConnection()
	{ }
}
