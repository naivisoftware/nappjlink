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
	 * 
	 * The projector establishes a connection (a-sync) when a message is sent, or on startup when 'ConnectOnStartup' is set to true.
	 * Initialization will fail if the connection can't be established when 'ConnectOnStartup' is set to true (defaults to false).
	 * 
	 * A connection remains available for 20 seconds after receiving the last response from the projector.
	 * Subsequent messages will establish a new connection, as outlined in the pjlink protocol document.
	 * You as a user don't have to worry about the state of the connection, that is done here for you.
	 * 
	 * All communication is a-synchronous: all calls to send() will return immediately -> the command is queued for write.
	 * On success, the response message from the projector is forwarded to the nap::PJLinkComponent that listens to this projector.
	 * If no component is listening the response is simply discarded.
	 *
	 * You must assign a nap::PJLinkProjectorPool to every projector.
	 * 
	 * The pool runs all queued I/O network requests a-synchronous on it's assigned worker thread.
	 */
	class NAPAPI PJLinkProjector : public Device
	{
		RTTI_ENABLE(Device)
	public:
		/**
		 * Turns the projector on
		 */
		void powerOn()													{ send<PJLinkSetPowerCommand>(true); }

		/**
		 * Turns the projector off
		 */
		void powerOff()													{ send<PJLinkSetPowerCommand>(false); }

		/**
		 * Mute projector audio and video output.
		 * @param value if audio and video output should be muted
		 */
		void muteOn()													{ send<PJLinkSetAVMuteCommand>(true); }

		/**
		 * Don't mute projector audio and video output.
		 * @param value if audio and video output should be muted
		 */
		void muteOff()													{ send<PJLinkSetAVMuteCommand>(false); }

		/**
		 * Sends a PJLink command to the projector a-sync.
		 * This function returns immediately, the command is queued.
		 * @param cmd command to send
		 */
		void send(PJLinkCommandPtr cmd);

		/**
		 * Sends a PJLink command of type CMD to the projector a-sync
		 * This function returns immediately, the command is queued.
		 * 
		 * ~~~~~{.cpp}
		 * projector->send<PJLinkSetPowerCommand>(true)
		 * ~~~~~
		 * 
		 * @param args optional PJLink command arguments
		 */
		template<typename CMD, typename ... Args>
		void send(Args&& ... args)										{ send(std::make_unique<CMD>(std::forward<Args>(args)...)); }

		/**
		 * Creates and sends a PJLink command to the projector a-sync.
		 * This function returns immediately, the command is queued.
		 * @param body pjlink command body (see spec)
		 * @param value pjlink value (see spec)
		 */
		void send(const char* body, const char* value)					{ send(std::make_unique<PJLinkCommand>(body, value)); }

		/**
		 * Connects the projector if connect on startup is true.
		 * Called by core after initialization.
		 * @param errorState the error if connecting fails
		 */
		bool start(utility::ErrorState& errorState) override;

		/**
		 * Disconnects the projector.
		 * Called by core before destruction.
		 */
		void stop() override;

		bool mConnect = false;									//< Property: 'ConnectOnStartup' Connect to projector on startup, startup will fail if connection can't be established
		std::string mIPAddress = "192.168.0.1";					//< Property: 'IP Address' ip address of the projector on the network
		nap::ResourcePtr<PJLinkProjectorPool> mPool;			//< Property: 'Pool' Interface that manages the connection

		/**
		 * Called by the **network processing thread** after receiving a response.
		 * Use PJLinkComponent::messageReceived to receive this message on the application thread.
		 */
		nap::Signal<const PJLinkCommand&> responseReceived;

	private:
		friend class PJLinkConnection;

		// Called by the PJLink client when connection is closed
		void connectionClosed();

		// Called by the PJLink client when it receives a message from the projector
		void response(const PJLinkCommand& message);

		// Creates a connection
		std::shared_ptr<PJLinkConnection> create(utility::ErrorState & error);

		// Get current connection handle
		std::shared_ptr<PJLinkConnection> getConnection(bool make, utility::ErrorState& error);

		std::mutex mConnectionMutex;
		std::shared_ptr<PJLinkConnection> mConnection = nullptr;	//< Client connection
	};
}
