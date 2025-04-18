/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local includes
#include "pjlinkconnection.h"
#include "pjlinkprojector.h"
#include "pjlinkcommand.h"

// External includes
#include <nap/logger.h>
#include <asio/connect.hpp>
#include <asio/read_until.hpp>
#include <asio/write.hpp>
#include <asio/use_future.hpp>
#include <asio/defer.hpp>

using namespace asio::ip;

namespace nap
{
	// Windows abort error code
#ifdef WIN32
	constexpr int abortec = 1236;
#else
	constexpr int abortec = 125;
#endif

	PJLinkConnection::PJLinkConnection(pjlink::Context& context, const asio::ip::address& address, PJLinkProjector& projector) :
		mSocket(context),
		mProjector(projector),
		mAddress(address)
	{ }


	std::shared_ptr<nap::PJLinkConnection> PJLinkConnection::create(pjlink::Context& context, const asio::ip::address& address, PJLinkProjector& projector)
	{
		return std::shared_ptr<PJLinkConnection>(
			new PJLinkConnection(context, address, projector)
		);
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
					nap::Logger::error("Failed (ec '%d') to connect to endpoint: %s, port: %d",
						ec.value(),
						handle->mAddress.to_string().c_str(),
						handle->mEndpoint.port());

					// Notify listeners explicitly here -> otherwise on close
					handle->mProjector.connectionClosed();
					return false;
				}

				// Connection success -> verify authentification
				nap::Logger::debug("%s: Connected, port: %d",
					handle->mAddress.to_string().c_str(),
					handle->mEndpoint.port());

				// Authentication failed
				if (!handle->authenticate())
					return false;

				// Write enqueued cmd
				handle->setTimer();
				if (!handle->mCmds.empty())
					handle->write(*(handle->mCmds.front()));

				// Start reading callback
				handle->read();

				// Return reference to self as future
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
				handle->close();
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
		nap::Logger::debug("%s: Received %d authorization bytes", mAddress.to_string().c_str(), size);
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
		nap::Logger::debug("%s: Authentication succeeded",
			mAddress.to_string().c_str());

		mReady = true;
		return true;
	}


	void PJLinkConnection::enqueue(PJLinkCommandPtr command)
	{
		// Submit task for execution -> it is queued and called from the socket execution thread
		auto handle = shared_from_this();
		asio::post(mSocket.get_executor(), [handle, cmd = std::move(command)]() mutable
			{
				// We only write if the queue is empty -> when all cmds have been processed.
				// PJLink requires cmds to be sent in order, one by one, after a valid response.
				// The recursive read callback handles further cmd processing, after a response.
				bool queue_empty = handle->mCmds.empty();
				handle->mCmds.emplace(std::move(cmd));
				if (handle->mReady && queue_empty)
				{
					handle->write(*(handle->mCmds.front()));
				}
			}
		);
	}


	void PJLinkConnection::write(PJLinkCommand& cmd)
	{
		assert(mSocket.is_open() && mReady);
		auto write_buffer = asio::buffer(cmd.data(), cmd.size());
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
				nap::Logger::debug("%s: Written %d byte(s)", handle->mAddress.to_string().c_str(), size);
			});
	}


	void PJLinkConnection::read()
	{
		assert(mSocket.is_open());
		auto handle = shared_from_this();
		asio::async_read_until(mSocket, mRespBuffer, pjlink::terminator, [handle] (std::error_code ec, std::size_t size)
			{
				if (ec)
				{
					if (ec.value() != abortec)
					{
						nap::Logger::error("Reading failed (ec '%d'), projector endpoint: %s,\nmsg: %s",
							ec.value(),
							handle->mAddress.to_string().c_str(),
							ec.message().c_str());
					}
					handle->close();
					return;
				}

				// Read succeeded
				nap::Logger::debug("%s: Read %d byte(s)", handle->mAddress.to_string().c_str(), size);

				// Commit response from buffer input to response
				assert(!handle->mCmds.empty());
				auto& reply = *handle->mCmds.front();

				std::istream is(&handle->mRespBuffer);
				std::getline(std::istream(&handle->mRespBuffer), reply.mResponse, pjlink::terminator);

				// All good
				nap::Logger::debug("%s: Reply '%s', cmd: '%s'",
					handle->mAddress.to_string().c_str(),
					reply.mResponse.substr(0, reply.mResponse.size()-1).c_str(),
					reply.mCommand.substr(0, reply.mCommand.size()-1).c_str());

				// Forward response and set timer
				handle->mProjector.response(reply);
				handle->setTimer();

				// After receiving a response, we're ready to send a subsequent request
				// PJLink requires the response to be sent before attempting a new write..
				handle->mCmds.pop();
				if (!handle->mCmds.empty())
					handle->write(*(handle->mCmds.front()));

				// Keep reading until there's a new response
				handle->read();
			});
	}


	void PJLinkConnection::close()
	{
		// Delete timer -> bail if closed
		mTimeout.reset(nullptr);

		// Close -> must be open when called deferred
		if (!mSocket.is_open())
			return;

		std::error_code ec;
		mSocket.close(ec);
		if (ec)
		{
			nap::Logger::error("Close request failed (ec '%d'), projector endpoint : %s",
				ec.value(), mAddress.to_string().c_str());
			return;
		}

		// Cancel outstanding timing operations
		nap::Logger::debug("%s: Connection closed", mAddress.to_string().c_str());

		// Notify listeners
		mProjector.connectionClosed();
	}


	void PJLinkConnection::timeout(const std::error_code& ec)
	{
		if (!ec)
		{
			nap::Logger::debug("%s: Connection timed out", mAddress.to_string().c_str());
			assert(mSocket.is_open());
			close();
		}
	}


	void PJLinkConnection::setTimer()
	{
		mTimeout = std::make_unique<asio::steady_timer>(mSocket.get_executor(), nap::Seconds(sTimeout));
		mTimeout->async_wait(
			std::bind(&PJLinkConnection::timeout, shared_from_this(), std::placeholders::_1)
		);
	}
}
