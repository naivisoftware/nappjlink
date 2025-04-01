/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local includes
#include "pjlinkprojectorpool.h"

// External includes
#include <asio/ip/tcp.hpp>
#include <utility/dllexport.h>
#include <nap/timer.h>

namespace nap
{
	class PJLinkProjector;
	namespace pjlink
	{
		using Socket = asio::ip::tcp::socket;
	}

	/**
	 * TCP projector connection, established and managed by the projector pool.
	 */
	class NAPAPI PJLinkConnection final
	{
	public:
		// tcp connection timeout in seconds
		static constexpr double sTimeout = 20.0;

		/**
		 * Constructor 
		 * @param context network runtime context
		 * @param projector end point
		 */
		PJLinkConnection(pjlink::Context& context, PJLinkProjector& projector);

		/**
		 * Closes connection to projector on destruction
		 */
		~PJLinkConnection();

		// Disable copy
		PJLinkConnection& operator=(const PJLinkConnection&) = delete;
		PJLinkConnection(PJLinkConnection&) = delete;

		// Explicitly enable move
		PJLinkConnection& operator=(PJLinkConnection&& other) noexcept;
		PJLinkConnection(PJLinkConnection&& other) noexcept;

		/**
		 * @return projector end-point
		 */
		const PJLinkProjector& getProjector() const		{ assert(mProjector != nullptr); return *mProjector; }

		/**
		 * @return communication socket
		 */
		pjlink::Socket& getSocket()						{ return mSocket; }

		/**
		 * @return communication socket
		 */
		const pjlink::Socket& getSocket() const			{ return mSocket; }

		/**
		 * Compare if they manage the same projector instance
		 */
		bool operator == (const PJLinkProjector& c)		{ return &c == mProjector; }

		/**
		 * Compare if they manage the same projector instance
		 */
		bool operator != (const PJLinkProjector& c)		{ return &c != mProjector; }

		/**
		 * Session length in seconds
		 */
		double session() const							{ return mTimer.getElapsedTime(); }

	private:
		pjlink::Socket		mSocket;					//< Communication socket
		PJLinkProjector*	mProjector = nullptr;		//< Projector end-point
		SteadyTimer			mTimer;						//< Connection timer

		void connect();
	};
}

//////////////////////////////////////////////////////////////////////////
// HASH
//////////////////////////////////////////////////////////////////////////

namespace std
{
	template<>
	struct hash<nap::PJLinkConnection>
	{
		size_t operator()(const nap::PJLinkConnection& v) const
		{
			return std::hash<const nap::PJLinkProjector*>{}( &(v.getProjector()) );
		}
	};
}
