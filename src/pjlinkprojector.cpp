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
		if (mConnect)
		{
			if (connect(nap::Seconds(10), errorState) == nullptr)
				return false;
		}
		return true;
	}


	void PJLinkProjector::stop()
	{
		auto connection = getConnection();
		if (connection != nullptr)
		{
			auto cf = connection->disconnect();
			if (cf.wait_for(nap::Seconds(10)) != std::future_status::ready)
				nap:Logger::warn("Unable to gracefully shut down '%s' connection", mID.c_str());

			// Connection should be reset after a disconnect
			assert(mConnection == nullptr);
		}
	}


	void PJLinkProjector::send(PJLinkCommand&& cmd)
	{
		utility::ErrorState error;
		auto connection = connect(nap::Seconds(5), error);
		if (connection == nullptr)
		{
			nap::Logger::warn(error.toString());
			return;
		}
		mConnection->enqueue(std::move(cmd));
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


	std::shared_ptr<PJLinkConnection> PJLinkProjector::connect(nap::Milliseconds timeOut, utility::ErrorState& error)
	{
		// Already connected
		auto client = getConnection();
		if (client != nullptr)
			return client;

		// Make ip address
		std::error_code ec;
		auto ip_address = asio::ip::make_address(mIPAddress, ec);
		if (!error.check(!ec, "Invalid ip address: '%s'", mIPAddress.c_str()))
			return nullptr;

		// Instantiate connection
		client = PJLinkConnection::create(mPool->mContext, ip_address, *this);

		// Wait until established, including authorization
		// TODO: Give option not to block and continue execution
		client->connect();

		/*
		if (!error.check(cf.wait_for(timeOut) == std::future_status::ready,
			"Projector connection timed out"))
			return nullptr;

		// Check if connection is valid
		bool rvalue = cf.get();
		if (!error.check(rvalue, "Unable to connect to projector at '%s', port: %d",
			mIPAddress.c_str(), pjlink::port))
			return nullptr;
		*/

		// Store for future reference
		{
			std::lock_guard<std::mutex> lock(mConnectionMutex);
			mConnection = client;
		}
		return client;
	}


	std::shared_ptr<nap::PJLinkConnection> PJLinkProjector::getConnection()
	{
		std::lock_guard<std::mutex> lock(mConnectionMutex);
		return mConnection;
	}
}
