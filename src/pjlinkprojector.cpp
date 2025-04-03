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
		if (mConnection != nullptr)
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
		if (connect(nap::Seconds(5), error) == nullptr)
		{
			nap::Logger::warn(error.toString());
			return;
		}
		mConnection->send(PJLinkCommand(cmd, value));
	}


	void PJLinkProjector::get(const std::string& cmd)
	{
		utility::ErrorState error;
		if (connect(nap::Seconds(5), error) == nullptr)
		{
			nap::Logger::warn(error.toString());
			return;
		}
		mConnection->send(PJLinkCommand(cmd, &pjlink::cmd::query));
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
		//mConnection = nullptr;
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

		// TODO: You 'could' (in theory) return a connection that is about to be timed out
		// TODO: Those messages will most likely fail -> we should counter that
		std::error_code ec;
		auto ip_address = asio::ip::make_address(mIPAddress, ec);
		if (!error.check(!ec, "Invalid ip address: '%s'", mIPAddress.c_str()))
			return nullptr;

		// Instantiate connection
		mConnection = PJLinkConnection::create(mPool->mContext, ip_address, *this);
		auto cf = mConnection->connect();

		// Wait until established, including authorization
		// TODO: Give option not to block and continue execution
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
