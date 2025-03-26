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
		assert(!connected());
		if (!mPool->connect(*this, errorState))
			return false;

		mConnected = true;
		return true;
	}


	void PJLinkProjector::stop()
	{
		if(connected())
			mPool->disconnect(*this);
	}


	void PJLinkProjector::set(const char* cmd, const char* value)
	{
		if (connected())
		{
			auto cmd_str = utility::stringFormat("%s%s %s\r", pjlink::cmd::header, cmd, value);
			mPool->send(*this, cmd_str.c_str(), cmd_str.size());
		}
	}


	void PJLinkProjector::get(const char* cmd)
	{

	}
}
