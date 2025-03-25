/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External includes
#include <nap/device.h>
#include <nap/resourceptr.h>

namespace nap
{
	// The projector pool
	class PJLinkEndPoint;

	/**
	 * PJLink projector communication interface.
	 * Acts as a client to control and operate a pjlink enabled projector on the network.
	 * This object purely acts as an interface and does not manage its own connection.
	 */
	class PJLinkProjector : public Device
	{
		RTTI_ENABLE(Device)
	public:
		PJLinkProjector() = default;
		std::string mIPAddress = "192.168.0.1";				//< Property: 'IP Address' ip address of the projector on the network
		nap::ResourcePtr<PJLinkEndPoint> mEndPoint;			//< Property: 'End Point' Interface that manages the connection
	};
}
