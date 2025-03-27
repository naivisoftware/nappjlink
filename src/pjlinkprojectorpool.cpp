/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local includes
#include "pjlinkprojectorpool.h"
#include "pjlinkprojector.h"

// External includes
#include <asio/write.hpp>
#include <asio/buffer.hpp>
#include <nap/logger.h>
#include <asio/streambuf.hpp>
#include <asio/read_until.hpp>
#include <iterator>
#include <nap/assert.h>

RTTI_BEGIN_CLASS(nap::PJLinkProjectorPool)
RTTI_END_CLASS

using namespace asio::ip;

namespace nap
{
	bool PJLinkProjectorPool::init(utility::ErrorState& error)
	{
		try
		{
			mContext = std::make_unique<pjlink::Context>();
			return true;
		}
		catch (std::exception& e)
		{
			error.fail(e.what());
			return false;
		}
	}


	PJLinkConnection* PJLinkProjectorPool::connect(PJLinkProjector& projector, nap::utility::ErrorState& error)
	{
		// Check if there's an open connection
		auto it = mConnections.find(&projector);
		if (it != mConnections.end())
		{
			// -> Within time window
			if (it->second.session() < PJLinkConnection::sTimeout)
				return &it->second;

			// -> Connection timed out
			mConnections.erase(&projector);
		}

		// Asio can throw exceptions -> catch those
		try
		{
			// Create socket and connect to projector
			assert(mContext != nullptr);
			pjlink::Socket proj_socket(*mContext);
			auto end_point = tcp::endpoint(address::from_string(projector.mIPAddress), pjlink::port);
			proj_socket.connect(end_point);

			// Add as valid connection on success
			auto it = mConnections.emplace(&projector, PJLinkConnection(std::move(proj_socket), projector));
			assert(it.second);
			return &it.first->second;
		}
		catch (asio::system_error& e)
		{
			error.fail(e.what());
			return nullptr;
		}
	}


	void PJLinkProjectorPool::disconnect(PJLinkProjector& projector)
	{
		// Delete connection -> shuts down active transfers
		assert(mConnections.find(&projector) != mConnections.end());
		mConnections.erase(&projector);
	}


	void readResponse(PJLinkConnection& connection, std::string& ioResponse)
	{
		// Read from socket
		assert(connection.getSocket().is_open());
		char buffer[512];
		auto size = connection.getSocket().read_some(asio::buffer(buffer, 512));
		nap::Logger::info("%s: received %d bytes", connection.getProjector().mID.c_str(), size);

		// Add to response
		assert(size > 0);
		ioResponse += std::string(buffer, size);

		// Incomplete -> read more
		if (ioResponse.back() != pjlink::terminator)
			readResponse(connection, ioResponse);

		// Could contain more than 1 response message, split based on msg terminator
		auto parts = utility::splitString(ioResponse, pjlink::terminator);
		assert(parts.size() <= 2);
		ioResponse = parts.size() > 1 ? parts[1] : ioResponse;

		// Valid response (not authentication)
		if (utility::startsWith(ioResponse, &pjlink::response::header))
			return;

		// Handle authentication
		if (utility::startsWith(ioResponse, pjlink::response::authenticate::header))
		{
			if (utility::startsWith(ioResponse, pjlink::response::authenticate::disabled))
			{
				ioResponse.clear();
				readResponse(connection, ioResponse);
				return;
			}

			// Authentication is not supported..
			// TODO: Support PJLink authentication
			nap::Logger::error("PJLink authentication request received, this is not supported!");
			nap::Logger::error("Turn off authentication for projector: %s", connection.getProjector().mID.c_str());
		}

		// Unknown response!
		NAP_ASSERT_MSG(false, utility::stringFormat("PJLink: unknown response: %s", ioResponse.c_str()).c_str());
	}


	void PJLinkProjectorPool::send(PJLinkProjector& projector, std::string&& msg)
	{
		utility::ErrorState error;
		auto* connection = connect(projector, error);
		if (connection == nullptr)
		{
			nap::Logger::error("Get connect to projector '%s', error: ",
				projector.mID.c_str(), error.toString().c_str());
			return;
		}

		// Send msg
		nap::Logger::info("%s: sending cmd: %s", projector.mID.c_str(), msg.c_str());
		auto write_size = asio::write(connection->getSocket(), asio::buffer(msg.data(), msg.size()));
		nap::Logger::info("%s: written %d bytes", projector.mID.c_str(), write_size);

		// Get response
		std::string response;
		readResponse(*connection, response);
		assert(!response.empty());
		nap::Logger::info("%s: response: %s", projector.mID.c_str(), response.c_str());
	}
}
