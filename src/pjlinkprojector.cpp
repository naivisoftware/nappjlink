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
	static std::string createCmd(const char* cmd, const char* value)
	{
		std::string r;
		r.reserve(strlen(value) + 8);
		r += pjlink::cmd::header;
		r += pjlink::cmd::version;
		r += cmd;
		r += pjlink::cmd::seperator;
		r += value;
		r += pjlink::terminator;

		assert(r.length() < pjlink::cmd::size);
		return r;
	}


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


	void PJLinkProjector::powerOn()
	{
		mPool->send(*this, createCmd(pjlink::cmd::set::power, "1"));
	}


	void PJLinkProjector::powerOff()
	{
		mPool->send(*this, createCmd(pjlink::cmd::set::power, "0"));
	}


	void PJLinkProjector::set(const char* cmd, const char* value)
	{
		mPool->send(*this, createCmd(cmd, value));
	}


	void PJLinkProjector::get(const char* cmd)
	{
		mPool->send(*this, createCmd(cmd, &pjlink::cmd::query));
	}
}
