/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External includes
#include <nap/device.h>
#include <nap/resourceptr.h>
#include <unordered_map>
#include <asio/io_context.hpp>
#include <asio/executor_work_guard.hpp>
#include <asio/ip/tcp.hpp>

namespace nap
{
	class PJLinkProjector;
	namespace pjlink
	{
		using Context = asio::io_context;
		using Guard = asio::executor_work_guard<Context::executor_type>;
		using EndPoint = asio::ip::tcp::endpoint;
	}

	/**
	 * PJLink shared client context.
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
		 * Finish outstanding work and quit
		 */
		void onDestroy() override;

	private:
		friend class PJLinkProjector;

		// Attempt to disconnect
		pjlink::Context mContext;							//< Asio runtime context
		std::unique_ptr<pjlink::Guard> mGuard = nullptr;	//< Asio work guard
		std::unique_ptr<std::thread> mThread  = nullptr;	//< Asio runtime thread
		
    };
}
