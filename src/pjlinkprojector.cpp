/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "pjlinkprojector.h"

RTTI_BEGIN_CLASS(nap::PJLinkProjector)
	RTTI_PROPERTY("IP Address",		&nap::PJLinkProjector::mIPAddress,  nap::rtti::EPropertyMetaData::Required, "IP address of the projector on the network")
	RTTI_PROPERTY("Pool",			&nap::PJLinkProjector::mPool,		nap::rtti::EPropertyMetaData::Required, "Interface that manages the connection")
RTTI_END_CLASS

namespace nap
{
	bool PJLinkProjector::start(utility::ErrorState& errorState)
	{
		return mPool->connect(*this, errorState);
	}


	void PJLinkProjector::stop()
	{
		mPool->disconnect(*this);
	}


	void PJLinkProjector::powerOn()
	{
		mPool->send(*this, PJLinkCommand(pjlink::cmd::set::power, "1"));
	}


	void PJLinkProjector::powerOff()
	{
		mPool->send(*this, PJLinkCommand(pjlink::cmd::set::power, "0"));
	}


	void PJLinkProjector::mute(bool value)
	{
		mPool->send(*this, PJLinkCommand(pjlink::cmd::set::avmute, value ? "31" : "30"));
	}


	void PJLinkProjector::set(const std::string& cmd, const std::string& value)
	{
		mPool->send(*this, PJLinkCommand(cmd, value));
	}


	void PJLinkProjector::get(const std::string& cmd)
	{
		mPool->send(*this, PJLinkCommand(cmd, &pjlink::cmd::query));
	}
}
