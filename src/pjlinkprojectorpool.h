/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External includes
#include <nap/device.h>
#include <nap/resourceptr.h>
#include <unordered_map>
#include "pjlinkconnection.h"

// External includes
#include <asio/io_context.hpp>

namespace nap
{
	class PJLinkProjector;
	namespace pjlink
	{
		using Context = asio::io_context;
	}

	/**
	 * PJLink client pool.
	 * Manages multiple projector connections thread-safe.
	 */
	class NAPAPI PJLinkProjectorPool : public Resource
	{
		RTTI_ENABLE(Resource)
    public:
		// Default constructor
		PJLinkProjectorPool() = default;

		/**
		 * Creates the network context
		 * @param error error if initialization fails
		 * @return if initialization succeeded
		 */
		bool init(utility::ErrorState& error) override;

		/**
		 * Send a message to a projector.
		 * Establishes a connection if no connection is available.
		 * A connection remains available for 30 seconds after receiving a reply from the projector.
		 * @param projector the projector to send the message 
		 */
		void send(PJLinkProjector& projector, const char* data, size_t size);

	private:
		friend class PJLinkProjector;

		// Attempt to connect
		PJLinkConnection* connect(PJLinkProjector& projector, nap::utility::ErrorState& error);

		// Attempt to disconnect
		void disconnect(PJLinkProjector& projector);

		std::unique_ptr<pjlink::Context> mContext = nullptr;
		std::unordered_map<PJLinkProjector*, PJLinkConnection> mConnections;
    };
}
