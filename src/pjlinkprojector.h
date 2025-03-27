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
	/**
	 * PJLink protocol message specifications
	 */
	namespace pjlink
	{
		constexpr const int port				= 4352;			//< PJ Link communication port number
		constexpr const char terminator			= '\r';			//< PJ link msg terminator

		namespace cmd
		{
			constexpr const size_t size			= 136;			//< PJ link max cmd size
			constexpr const char header			= '%';			//< PJ link cmd header
			constexpr const char version		= '1';			//< PJ link cmd version
			constexpr const char seperator		= ' ';			//< PJ link seperator
			constexpr const char query			= '?';			//< PJ link query parameter
			namespace set {
				constexpr const char* power		= "POWR";		//< turn projector on(1) or off(0)
			}
		}

		namespace response
		{
			constexpr const char header			= '%';			// PJ link response header
			namespace authenticate {
				constexpr const char* header	= "PJLINK";		//< projector authentication response header
				constexpr const char* disabled	= "PJLINK 0";	//< projector authentication disabled (required!)
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
		 * Turns the projector on
		 */
		void powerOn();

		/**
		 * Turns the projector off
		 */
		void powerOff();

		/**
		 * Sends a control command to the projector a-sync.
		 * @param cmd pjlink set command
		 * @param value pjlink parameter
		 */
		void set(const char* cmd, const char* value);

		/**
		 * Sends a get command to the projector a-sync.
		 * @param cmd pjlink get command name
		 */
		void get(const char* cmd);

		std::string mIPAddress = "192.168.0.1";				//< Property: 'IP Address' ip address of the projector on the network
		nap::ResourcePtr<PJLinkProjectorPool> mPool;		//< Property: 'Pool' Interface that manages the connection

	private:
		bool mConnected = false;							
	};
}
