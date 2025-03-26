/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local includes
#include "pjlinkprojectorpool.h"

// External includes
#include <nap/device.h>
#include <nap/resourceptr.h>

namespace nap
{
	namespace pjlink
	{
		constexpr const int port = 4352;					//< default PJ Link port number
		namespace cmd
		{
			constexpr const char* header = "%1";			//< PJ link cmd header
			namespace set
			{
				constexpr const char* power = "POWR";		//< turn projector on(1) or off(0)
			}
		}
	}


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
		 * Sends a control command to the projector a-sync.
		 * @param cmd pjlink set command
		 * @param value pjlink parameter
		 */
		void set(const char* cmd, const char* value);

		/**
		 * Sends a query command to the projector a-sync.
		 * @param cmd pjlink query command
		 */
		void get(const char* cmd);

		std::string mIPAddress = "192.168.0.1";				//< Property: 'IP Address' ip address of the projector on the network
		nap::ResourcePtr<PJLinkProjectorPool> mPool;		//< Property: 'Pool' Interface that manages the connection

	private:
		bool mConnected = false;							
	};
}
