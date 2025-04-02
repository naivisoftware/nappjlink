/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local includes
#include "pjlinkprojectorpool.h"
#include "pjlinkconnection.h"
#include "pjlinkcommand.h"

// External includes
#include <nap/device.h>
#include <nap/resourceptr.h>

namespace nap
{
	/**
	 * PJLink projector communication interface.
	 * Acts as a client to control and operate a pjlink enabled projector on the network.
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
		 * Turns the projector on
		 */
		void powerOn()													{ set(pjlink::cmd::set::power, "1"); }

		/**
		 * Turns the projector off
		 */
		void powerOff()													{ set(pjlink::cmd::set::power, "0"); }

		/**
		 * Mute both projector audio and video output.
		 * @param value if audio and video output should be muted
		 */
		void avMute(bool value)											{ set(pjlink::cmd::set::avmute, value ? "31" : "30"); }

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

		std::string mIPAddress = "192.168.0.1";					//< Property: 'IP Address' ip address of the projector on the network
		nap::ResourcePtr<PJLinkProjectorPool> mPool;			//< Property: 'Pool' Interface that manages the connection

	private:
		std::unique_ptr<PJLinkConnection> mClient = nullptr;	//< Client connection
	};
}
