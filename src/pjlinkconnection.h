/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local includes
#include "pjlinkprojectorpool.h"
#include "pjlinkcommand.h"

// External includes
#include <asio/ip/tcp.hpp>
#include <asio/streambuf.hpp>
#include <asio/steady_timer.hpp>
#include <utility/dllexport.h>
#include <nap/timer.h>
#include <queue>
#include <future>

namespace nap
{
	class PJLinkProjector;
	namespace pjlink
	{
		using Socket = asio::ip::tcp::socket;
		using StreamBuf = asio::streambuf;
	}

	/**
	 * PJLink client connection.
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

		// Disable move
		PJLinkConnection& operator=(PJLinkConnection&& other) = delete;
		PJLinkConnection(PJLinkConnection&& other) = delete;

		/**
		 * @return projector end-point
		 */
		const PJLinkProjector& getProjector() const		{ return mProjector; }

		/**
		 * Compare if they manage the same projector instance
		 */
		bool operator == (const PJLinkProjector& c)		{ return &c == &mProjector; }

		/**
		 * Compare if they manage the same projector instance
		 */
		bool operator != (const PJLinkProjector& c)		{ return &c != &mProjector; }

	private:
		friend class PJLinkProjector;

		pjlink::Socket		mSocket;					//< Communication socket
		pjlink::EndPoint	mEndpoint;					//< Endpoint description
		PJLinkProjector&	mProjector;					//< Projector end-point

		// Called from client thread
		std::future<bool> connect();
		std::future<void> disconnect();

		// Called from asio execution thread
		bool authenticate(pjlink::EndPoint ep);
		void send(PJLinkCommand&& cmd);
		void write(pjlink::EndPoint ep);
		void read(pjlink::EndPoint ep, PJLinkCommand&& cmd);
		void close(pjlink::EndPoint ep);
		void timeout(const std::error_code& ec);

		// A-sync objects -> accessed from socket execution context
		pjlink::StreamBuf mAuthBuffer;						//< Authentification buffer
		pjlink::StreamBuf  mRespBuffer;						//< Response buffer
		std::queue<PJLinkCommand> mCmds;					//< Commands to send
		asio::steady_timer mTimeout;							//< Timeout connection timer
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
