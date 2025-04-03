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
			if (connect(nap::Seconds(10), errorState) == nullptr)
				return false;
		}
		return true;
	}


	void PJLinkProjector::stop()
	{
		if (mConnection)
		{
			auto cf = mConnection->disconnect();
			if (cf.wait_for(nap::Seconds(10)) != std::future_status::ready)
			{
			nap:Logger::warn("Unable to gracefully shut down '%s' connection", mID.c_str());
			}
		}
	}


	void PJLinkProjector::set(const std::string& cmd, const std::string& value)
	{
		utility::ErrorState error;
		auto* connection = connect(nap::Seconds(5), error);
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
		auto* connection = connect(nap::Seconds(5), error);
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
		mConnected  = false;
	}



	void PJLinkProjector::messageReceived(PJLinkCommand&& message)
	{
		// TODO: Forward!
	}


	nap::PJLinkConnection* PJLinkProjector::connect(nap::Milliseconds timeOut, utility::ErrorState& error)
	{
		// Return active connection when connected
		std::lock_guard<std::mutex> lock(mConnectionMutex);
		{
			if (mConnected)
			{
				assert(mConnection != nullptr);
				return mConnection.get();
			}
		}

		// TODO: You 'could' return a connection that is about to be timed out -> Those requests will fail
		// TODO: It is also possible that the handler, of that message will segfault because read buffers are destroyed
		// TODO: Therefore we should make a copy here and wait for connection to be closed before dismissing it...
		mConnection = PJLinkConnection::create(mPool->mContext, *this);
		auto cf = mConnection->connect();

		// Wait until established
		if (!error.check(cf.wait_for(timeOut) == std::future_status::ready,
			"Projector connection timed out"))
		{
			mConnection = nullptr;
			return nullptr;
		}

		// Check if connection is valid
		bool rvalue = cf.get();
		if (!error.check(rvalue, "Unable to connect to projector at '%s', port: %d",
			mIPAddress.c_str(), pjlink::port))
		{
			mConnection = nullptr;
			return nullptr;
		}

		mConnected = true;
		return mConnection.get();
	}
}
