/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "pjlinkprojector.h"

#include "nap/logger.h"

RTTI_BEGIN_CLASS(nap::PJLinkProjector)
	RTTI_PROPERTY("IP Address",		&nap::PJLinkProjector::mIPAddress,		nap::rtti::EPropertyMetaData::Required, "IP address of the projector on the network")
	RTTI_PROPERTY("Pool",			&nap::PJLinkProjector::mPool,			nap::rtti::EPropertyMetaData::Required, "Interface that manages the connection")
	RTTI_PROPERTY("Allow failure",	&nap::PJLinkProjector::mAllowFailure,	nap::rtti::EPropertyMetaData::Default,	"If the PJLinkProjector is allowed to fail on start")
RTTI_END_CLASS

namespace nap
{
	bool PJLinkProjector::start(utility::ErrorState& errorState)
	{
		assert(!connected());

		mConnected = mPool->connect(*this, errorState);

		if (mAllowFailure)
		{
			nap::Logger::error(*this, "Failed to connect to projector %s : %s", mID.c_str(), errorState.toString().c_str());
			return true;
		}

		return mConnected;
	}


	void PJLinkProjector::stop()
	{
		if(connected())
			mPool->disconnect(*this);
	}


	void PJLinkProjector::powerOn()
	{
		if (!connected())
			return;

		mPool->send(*this, PJLinkCommand(pjlink::cmd::set::power, "1"));
	}


	void PJLinkProjector::powerOff()
	{
		if (!connected())
			return;

		mPool->send(*this, PJLinkCommand(pjlink::cmd::set::power, "0"));
	}


	void PJLinkProjector::mute(bool value)
	{
		if (!connected())
			return;

		mPool->send(*this, PJLinkCommand(pjlink::cmd::set::avmute, value ? "31" : "30"));
	}


	void PJLinkProjector::set(const std::string& cmd, const std::string& value)
	{
		if (!connected())
			return;

		mPool->send(*this, PJLinkCommand(cmd, value));
	}


	void PJLinkProjector::get(const std::string& cmd)
	{
		if (!connected())
			return;

		mPool->send(*this, PJLinkCommand(cmd, &pjlink::cmd::query));
	}
}
