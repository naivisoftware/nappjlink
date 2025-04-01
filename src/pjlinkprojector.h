/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local includes
#include "pjlinkprojectorpool.h"
#include "pjlinkcommand.h"

// External includes
#include <nap/device.h>
#include <nap/resourceptr.h>

namespace nap
{
	/**
	 * PJLink projector communication interface.
	 * Acts as a client to control and operate a pjlink enabled projector on the network.
	 * This object purely acts as an interface and does not manage its own connection.
	 */
	class NAPAPI PJLinkProjector : public Device
	{
		RTTI_ENABLE(Device)
	public:
		/**
		 * Connect the projector
		 * @param errorState the error if connecting fails
		 */
		bool start(utility::ErrorState& errorState) override;

		/**
		 * Disconnect the projector
		 */
		void stop() override;

		/**
		 * @return if there is an active projector connection
		 */
		bool connected() const								{ return mConnected; }

		/**
		 * Turns the projector on
		 */
		void powerOn();

		/**
		 * Turns the projector off
		 */
		void powerOff();

		/**
		 * Mute both projector audio and video output.
		 * @param value if audio and video output should be muted
		 */
		void mute(bool value);

		/**
		 * Sends a control command to the projector a-sync.
		 * @param cmd pjlink set command
		 * @param value pjlink parameter
		 */
		void set(const std::string& cmd, const std::string& value);

		/**
		 * Sends a get command to the projector a-sync.
		 * @param cmd pjlink get command name
		 */
		void get(const std::string& cmd);

		std::string mIPAddress = "192.168.0.1";				//< Property: 'IP Address' ip address of the projector on the network
		nap::ResourcePtr<PJLinkProjectorPool> mPool;		//< Property: 'Pool' Interface that manages the connection

	private:
		bool mConnected = false;							
	};
}
