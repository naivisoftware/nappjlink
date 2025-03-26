/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local includes
#include "pjlinkprojector.h"

// External includes
#include <nap/device.h>
#include <nap/resourceptr.h>
#include <unordered_map>

// External includes
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

namespace nap
{
	/**
	 * PJLink client pool.
	 * Manages multiple projector connections thread-safe.
	 */
	class PJLinkProjectorPool : public Resource
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
		virtual bool init(utility::ErrorState& error) override;

	private:
		friend class PJLinkProjector;

		// Attempt to connect
		bool connect(PJLinkProjector& projector, nap::utility::ErrorState& error);

		// Attempt to disconnect
		void disconnect(PJLinkProjector& projector);

		using TCPContext = asio::io_context;
		using TCPSocket = asio::ip::tcp::socket;
		std::unique_ptr<TCPContext> mContext;
		std::unordered_map<PJLinkProjector*, std::unique_ptr<TCPSocket>> mConnections;
    };
}
