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


	void PJLinkProjectorPool::send(PJLinkProjector& projector, const char* data, size_t size)
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
		auto write_size = asio::write(connection->getSocket(), asio::buffer(data, size));
		nap::Logger::info("Written %03d bytes", write_size);

		char read_buffer[1024];
		auto read_size = connection->getSocket().read_some(asio::buffer(read_buffer, 1024));
		auto msg = std::string(read_buffer, read_size-1);
		auto msgs = utility::splitString(msg, '\r');
		nap::Logger::info("Received %03d bytes", read_size);
		for (const auto& msg : msgs)
			nap::Logger::info("Message: %s", msg.c_str());

		//str += "\0\n";

		//auto read_size = asio::read_until(tcp_socket, stream, '\r');
		//auto read_buff = std::string();
		//read_buff.resize(some_size);
		//stream.sgetn(read_buff.data(), some_size);
		

		// Check if there's more
	}
}
