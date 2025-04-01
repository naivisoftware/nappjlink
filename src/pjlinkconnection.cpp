/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "pjlinkconnection.h"
#include "pjlinkprojector.h"

using namespace asio::ip;

namespace nap
{
	PJLinkConnection::PJLinkConnection(pjlink::Context& context, PJLinkProjector& projector) :
		mSocket(context), mProjector(&projector)
	{
		connect();
	}


	void PJLinkConnection::connect()
	{
		assert(mProjector != nullptr);
		auto end_point = tcp::endpoint(address::from_string(mProjector->mIPAddress), pjlink::port);
		mSocket.connect(end_point);
	}


	nap::PJLinkConnection& PJLinkConnection::operator=(PJLinkConnection&& other) noexcept
	{
		mSocket = std::move(other.mSocket);
		mProjector = other.mProjector;
		other.mProjector = nullptr;
		return *this;
	}


	PJLinkConnection::PJLinkConnection(PJLinkConnection&& other) noexcept:
		mSocket(std::move(other.mSocket)),
		mProjector(other.mProjector)
	{
		other.mProjector = nullptr;
	}


	PJLinkConnection::~PJLinkConnection()
	{
		if (mSocket.is_open())
		{
			mSocket.cancel();
			mSocket.close();
		}
	}
}
