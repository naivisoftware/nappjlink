/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local includes
#include "pjlinkprojectorpool.h"
#include "pjlinkprojector.h"
#include "pjlinkcommand.h"

// External includes
#include <asio/write.hpp>
#include <asio/buffer.hpp>
#include <nap/logger.h>
#include <iterator>

RTTI_BEGIN_CLASS(nap::PJLinkProjectorPool)
RTTI_END_CLASS

using namespace asio::ip;

namespace nap
{
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
	}


	bool PJLinkProjectorPool::init(utility::ErrorState& error)
	{
		return true;
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
			pjlink::Socket proj_socket(mContext);
			auto end_point = tcp::endpoint(address::from_string(projector.mIPAddress), pjlink::port);
			proj_socket.connect(end_point);

			// Read authentication response
			PJLinkConnection connection(std::move(proj_socket), projector);
			std::string auth_response;
			readResponse(connection, auth_response);
			nap::Logger::info("%s: response: %s", projector.mID.c_str(), auth_response.c_str());

			// Ensure it's an authentication response
			if (!error.check(utility::startsWith(auth_response, pjlink::response::authenticate::header),
				"Invalid authentication response '%s' from projector '%s'", auth_response.c_str(), projector.mID.c_str()))
				return nullptr;

			// Make sure authentication is disabled
			if (!error.check(utility::startsWith(auth_response, pjlink::response::authenticate::disabled),
				"PJLink authentication procedure not supported, turn off authentication for '%s'", projector.mID.c_str()))
				return nullptr;

			// Add as valid connection on success
			auto it = mConnections.emplace(&projector, std::move(connection));
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


	void PJLinkProjectorPool::send(PJLinkProjector& projector, PJLinkCommand&& cmd)
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
		nap::Logger::info("%s: sending cmd: %s", projector.mID.c_str(), cmd.data());
		auto write_size = asio::write(connection->getSocket(), asio::buffer(cmd.data(), cmd.size()));
		nap::Logger::info("%s: written %d bytes", projector.mID.c_str(), write_size);

		// Get response
		std::string response;
		readResponse(*connection, response);
		assert(!response.empty());
		nap::Logger::info("%s: response: %s", projector.mID.c_str(), response.c_str());
	}
}
