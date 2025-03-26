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

RTTI_BEGIN_CLASS(nap::PJLinkProjectorPool)
RTTI_END_CLASS

using namespace asio::ip;

namespace nap
{
	bool PJLinkProjectorPool::init(utility::ErrorState& error)
	{
		try
		{
			mContext = std::make_unique<TCPContext>();
			return true;
		}
		catch (std::exception& e)
		{
			error.fail(e.what());
			return false;
		}
	}


	bool PJLinkProjectorPool::connect(const PJLinkProjector& projector, nap::utility::ErrorState& error)
	{
		// Bail if connection is already made
		assert(mContext != nullptr);
		if(mConnections.find(&projector) != mConnections.end())
			return true;

		// Asio can throw exceptions -> catch those
		try
		{
			// Create socket and connect to projector
			std::unique_ptr<TCPSocket> tcp_socket = std::make_unique<TCPSocket>(*mContext);
			auto end_point = tcp::endpoint(address::from_string(projector.mIPAddress), pjlink::port);
			tcp_socket->connect(end_point);

			// Add as valid connection on success
			mConnections.emplace(&projector, std::move(tcp_socket));
		}
		catch (asio::system_error& e)
		{
			error.fail("Connector '%s' connection '%s' failed", projector.mID.c_str(), projector.mIPAddress.c_str());
			error.fail(e.what());
			return false;
		}

		return true;
	}


	void PJLinkProjectorPool::disconnect(const PJLinkProjector& projector)
	{
		assert(mConnections.find(&projector) != mConnections.end());
		mConnections.erase(&projector);
	}


	void PJLinkProjectorPool::send(const PJLinkProjector& projector, const char* data, size_t size)
	{
		// Send msg
		auto it = mConnections.find(&projector); assert(it != mConnections.end());
		auto& tcp_socket = *it->second;
		auto result = asio::write(tcp_socket, asio::buffer(data, size));
		nap::Logger::info("Written %03d bytes", result);
	}
}
