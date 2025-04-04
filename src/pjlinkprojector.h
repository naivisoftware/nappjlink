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
#include <mutex>
#include <nap/signalslot.h>

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
		 * Turns the projector on
		 */
		void powerOn()													{ send(PJLinkSetPowerCommand(true)); }

		/**
		 * Turns the projector off
		 */
		void powerOff()													{ send(PJLinkSetPowerCommand(false)); }

		/**
		 * Mute projector audio and video output.
		 * @param value if audio and video output should be muted
		 */
		void muteOn()													{ send(PJLinkSetAVMuteCommand(true)); }

		/**
		 * Don't mute projector audio and video output.
		 * @param value if audio and video output should be muted
		 */
		void muteOff()													{ send(PJLinkSetAVMuteCommand(false)); }

		/**
		 * Sends a set (control) command to the projector a-sync.
		 * This function returns immediately, the command is queued.
		 * @param cmd control command to send
		 */
		void set(PJLinkSetCommand&& cmd)								{ send(std::move(cmd)); }

		/**
		 * Sends a get (query) command to the projector a-sync.
		 * This function returns immediately, the command is queued.
		 * @param cmd query command to send
		 */
		void get(PJLinkGetCommand&& cmd)								{ send(std::move(cmd)); }

		/**
		 * Sends a PJLink command to the projector a-sync.
		 * This function returns immediately, the command is queued.
		 * @param cmd command to send
		 */
		void send(PJLinkCommand&& cmd);

		/**
		 * Creates and sends a PJLink command to the projector a-sync.
		 * This function returns immediately, the command is queued.
		 * @param body pjlink command body (see spec)
		 * @param value pjlink value (see spec)
		 */
		void send(const char* body, const char* value)					{ send(PJLinkCommand(body, value)); }

		/**
		 * Connects the projector if connect on startup is true.
		 * Called by core after initialization.
		 * @param errorState the error if connecting fails
		 */
		bool start(utility::ErrorState& errorState) override;

		/**
		 * Disconnect the projector.
		 * Called by core before destruction.
		 */
		void stop() override;

		bool mConnect = false;									//< Property: 'ConnectOnStartup' Connect to projector on startup, startup will fail if connection can't be established
		std::string mIPAddress = "192.168.0.1";					//< Property: 'IP Address' ip address of the projector on the network
		nap::ResourcePtr<PJLinkProjectorPool> mPool;			//< Property: 'Pool' Interface that manages the connection

		// Signal called when receiving a pj link response
		nap::Signal<const PJLinkCommand&> ResponseReceived;

	private:
		friend class PJLinkConnection;

		// Called by the PJLink client when connection is closed
		void connectionClosed();

		// Called by the PJLink client when it receives a message from the projector
		void response(PJLinkCommand&& message);

		// Creates a connection
		std::shared_ptr<PJLinkConnection> create(utility::ErrorState & error);

		// Get current connection handle
		std::shared_ptr<PJLinkConnection> getConnection(utility::ErrorState& error);

		std::mutex mConnectionMutex;
		std::shared_ptr<PJLinkConnection> mConnection = nullptr;	//< Client connection
	};
}
