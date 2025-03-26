/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local includes
 #include "pjlinkprojectorpool.h"

RTTI_BEGIN_CLASS(nap::PJLinkProjectorPool)
RTTI_END_CLASS

using namespace asio::ip;

namespace nap
{
	bool PJLinkProjectorPool::init(utility::ErrorState& error)
	{
		// Asio can throw exceptions -> catch those
		try
		{
			mContext = std::make_unique<TCPContext>();
			return true;
		}
		catch (asio::system_error& e)
		{
			error.fail(e.what());
			return false;
		}
	}


	bool PJLinkProjectorPool::connect(PJLinkProjector& projector, nap::utility::ErrorState& error)
	{
		// Bail if connection is already made
		if(mConnections.find(&projector) != mConnections.end())
			return true;

		// Asio can throw exceptions -> catch those
		try
		{
			// Create socket
			assert(mContext != nullptr);
			std::unique_ptr<TCPSocket> tcp_socket = std::make_unique<TCPSocket>(*mContext);

			// Connect to endpoint (projector)
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


	void PJLinkProjectorPool::disconnect(PJLinkProjector& projector)
	{
		assert(mConnections.find(&projector) != mConnections.end());
		mConnections.erase(&projector);
	}
}
