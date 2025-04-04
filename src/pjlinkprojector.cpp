/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local includes
#include "pjlinkprojector.h"

// External includes
#include <nap/logger.h>
#include <asio/ip/address.hpp>

RTTI_BEGIN_CLASS(nap::PJLinkProjector)
	RTTI_PROPERTY("IP Address", &nap::PJLinkProjector::mIPAddress, nap::rtti::EPropertyMetaData::Required, "IP address of the projector on the network")
	RTTI_PROPERTY("Pool", &nap::PJLinkProjector::mPool, nap::rtti::EPropertyMetaData::Required, "Interface that manages the connection")
	RTTI_PROPERTY("ConnectOnStartup", &nap::PJLinkProjector::mConnect, nap::rtti::EPropertyMetaData::Default, "Connect to projector on startup, init will fail if connection can't be established")
RTTI_END_CLASS

namespace nap
{
	bool PJLinkProjector::start(utility::ErrorState& errorState)
	{
		// If connection on startup is requested -> force
		if (mConnect)
		{
			// Create client connection
			auto client = getConnection(errorState);
			if (client == nullptr)
				return false;

			// Connect
			if (!errorState.check(client->connect().wait_for(nap::Seconds(10)) == std::future_status::ready,
				"Connection to endpoint '%s' timed out", mIPAddress.c_str()))
				return false;
		}
		return true;
	}


	void PJLinkProjector::stop()
	{
		utility::ErrorState error;
		auto client = getConnection(error);
		if (client != nullptr)
		{
			auto cf = client->disconnect();
			if (cf.wait_for(nap::Seconds(10)) != std::future_status::ready)
				nap:Logger::warn("Unable to gracefully shut down '%s' connection", mID.c_str());

			// Connection should be reset after a disconnect
			assert(mConnection == nullptr);
		}
	}


	void PJLinkProjector::send(PJLinkCommand&& cmd)
	{
		utility::ErrorState error;
		auto client = getConnection(error);
		if (client == nullptr)
		{
			nap::Logger::error(error.toString());
			return;
		}
		client->enqueue(std::move(cmd));
	}


	void PJLinkProjector::connectionClosed()
	{
		// Clear current connection
		std::lock_guard<std::mutex> lock(mConnectionMutex);
		mConnection = nullptr;
	}


	void PJLinkProjector::response(PJLinkCommand&& message)
	{
		// Notify listeners
		ResponseReceived(message);
	}


	std::shared_ptr<PJLinkConnection> PJLinkProjector::create(utility::ErrorState& error)
	{
		// Make ip address
		std::error_code ec;
		auto ip_address = asio::ip::make_address(mIPAddress, ec);
		if (!error.check(!ec, "Invalid ip address: '%s'", mIPAddress.c_str()))
			return nullptr;

		// Create client and connect
		auto client = PJLinkConnection::create(mPool->mContext, ip_address, *this);
		return client;
	}


	std::shared_ptr<nap::PJLinkConnection> PJLinkProjector::getConnection(utility::ErrorState& error)
	{
		std::lock_guard<std::mutex> lock(mConnectionMutex);
		if (mConnection == nullptr)
		{
			mConnection = create(error);
			if (mConnection == nullptr)
				return  nullptr;

			mConnection->connect();
		}
		return mConnection;
	}
}
