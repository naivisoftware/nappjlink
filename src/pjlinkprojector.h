/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External includes
#include <nap/device.h>
#include <nap/resourceptr.h>

namespace nap
{
	namespace pjlink
	{
		constexpr const int port = 4352;		//< pjlink tcp communication port
	}

	// The projector pool
	class PJLinkProjectorPool;

	/**
	 * PJLink projector communication interface.
	 * Acts as a client to control and operate a pjlink enabled projector on the network.
	 * This object purely acts as an interface and does not manage its own connection.
	 */
	class PJLinkProjector : public Device
	{
		RTTI_ENABLE(Device)
	public:
		/**
		 * Connect the projector
		 * @param errorState the error if connecting fails
		 */
		virtual bool start(utility::ErrorState& errorState) override;

		/**
		 * Disconnect the projector
		 */
		void stop() override;

		std::string mIPAddress = "192.168.0.1";				//< Property: 'IP Address' ip address of the projector on the network
		nap::ResourcePtr<PJLinkProjectorPool> mPool;		//< Property: 'Pool' Interface that manages the connection

	};
}
