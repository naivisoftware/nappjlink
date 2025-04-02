/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "pjlinkprojector.h"
#include <nap/logger.h>

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
			if (getOrCreateConnection(nap::Milliseconds(10000), errorState) == nullptr)
				return false;
		}
		return true;
	}


	void PJLinkProjector::stop()
	{
		// If there's still a connection, close it
		std::lock_guard<std::mutex> lock(mConnectionMutex);
		if (mConnection != nullptr)
		{
			auto cf = mConnection->disconnect();
			if (cf.wait_for(nap::Milliseconds(10000)) != std::future_status::ready)
				nap:Logger::warn("Unable to gracefully shut down '%s' connection", mID.c_str());

			mConnection.reset(nullptr);
		}
	}


	void PJLinkProjector::set(const std::string& cmd, const std::string& value)
	{
		utility::ErrorState error;
		auto* connection = getOrCreateConnection(nap::Milliseconds(5000), error);
		if (connection != nullptr)
		{
			mConnection->send(PJLinkCommand(cmd, value));
			return;
		}
		nap::Logger::warn(error.toString());
	}


	void PJLinkProjector::get(const std::string& cmd)
	{
		utility::ErrorState error;
		auto* connection = getOrCreateConnection(nap::Milliseconds(5000), error);
		if (connection != nullptr)
		{
			mConnection->send(PJLinkCommand(cmd, &pjlink::cmd::query));
			return;
		}
		nap::Logger::warn(error.toString());
	}


	void PJLinkProjector::connectionOpened()
	{
		std::lock_guard<std::mutex> lock(mConnectionMutex);
		mConnected = true;
	}


	void PJLinkProjector::connectionClosed()
	{
		std::lock_guard<std::mutex> lock(mConnectionMutex);
		mConnection = nullptr;
		mConnected  = false;
	}



	void PJLinkProjector::messageReceived(PJLinkCommand&& message)
	{
		// TODO: Forward!
	}


	nap::PJLinkConnection* PJLinkProjector::getOrCreateConnection(nap::Milliseconds timeOut, utility::ErrorState& error)
	{
		// Client active
		if (mConnection != nullptr)
			return mConnection.get();

		// Create client and try to connect
		mConnection = std::make_unique<PJLinkConnection>(mPool->mContext, *this);
		auto cf = mConnection->connect();

		// Wait until established
		if (!error.check(cf.wait_for(timeOut) == std::future_status::ready,
			"Projector connection timed out"))
		{
			mConnection.reset(nullptr);
			return nullptr;
		}

		// Check if connection is valid
		bool rvalue = cf.get();
		if (!error.check(rvalue, "Unable to connect to projector at '%s', port: %d",
			mIPAddress.c_str(), pjlink::port))
		{
			mConnection.reset(nullptr);
			return nullptr;
		}

		return mConnection.get();
	}
}
