/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "pjlinkconnection.h"

namespace nap
{
	nap::PJLinkConnection& PJLinkConnection::operator=(PJLinkConnection&& other) noexcept
	{
		mSocket = std::move(other.mSocket);
		mProjector = other.mProjector;
		other.mProjector = nullptr;
		mTimer.reset();
		return *this;
	}


	PJLinkConnection::PJLinkConnection(PJLinkConnection&& other) noexcept:
		mSocket(std::move(other.mSocket)),
		mProjector(other.mProjector)
	{
		other.mProjector = nullptr;
		mTimer.reset();
	}
}
