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
	 * PJLink client connection instance, instantiated by the PJLinkProjector.
	 * Handles all PJLink TCP/IP IO a-synchronous.
	 */
	class NAPAPI PJLinkConnection : public std::enable_shared_from_this<PJLinkConnection>
	{
	public:
		// tcp connection timeout in seconds
		static constexpr int sTimeout = 20;

		/**
		 * Creates a pjlink connection.
		 * @param context pjlink runtime context
		 * @param address endpoint address
		 * @param projector interface
		 * @return pjlink connection handle
		 */
		static std::shared_ptr<PJLinkConnection> create(pjlink::Context& context, const asio::ip::address& address, PJLinkProjector& projector);

		// Disable copy
		PJLinkConnection& operator=(const PJLinkConnection&) = delete;
		PJLinkConnection(PJLinkConnection&) = delete;

		// Disable move
		PJLinkConnection& operator=(PJLinkConnection&& other) = delete;
		PJLinkConnection(PJLinkConnection&& other) = delete;

		/**
		 * @return if client connection is established and active
		 */
		bool connected()								{ return mReady; }

		/**
		 * Compare if they manage the same projector instance
		 */
		bool operator == (const PJLinkProjector& c)		{ return &c == &mProjector; }

		/**
		 * Compare if they manage the same projector instance
		 */
		bool operator != (const PJLinkProjector& c)		{ return &c != &mProjector; }

		// Future connection -> available after establishing connection successful authorization
		using Future = std::future<std::shared_ptr<PJLinkConnection>>;

	private:
		friend class PJLinkProjector;

		pjlink::Socket		mSocket;					//< Communication socket
		pjlink::Address		mAddress;					//< Endpoint address description
		pjlink::EndPoint	mEndpoint;					//< Endpoint description
		PJLinkProjector&	mProjector;					//< Projector end-point

		// Called from client thread
		std::future<bool> connect();
		std::future<void> disconnect();
		void enqueue(PJLinkCommandPtr cmd);

		// Called from asio execution thread
		bool authenticate();
		void write(PJLinkCommand& cmd);
		void read();
		void close();
		void timeout(const std::error_code& ec);
		void setTimer();

		// A-sync objects -> accessed from socket execution context
		pjlink::StreamBuf mAuthBuffer;					//< Authentication buffer
		pjlink::StreamBuf  mRespBuffer;					//< Response buffer
		std::queue<PJLinkCommandPtr> mCmds;				//< Commands to send
		std::unique_ptr<asio::steady_timer> mTimeout;	//< Timeout connection timer
		std::atomic<bool> mReady = { false };			//< If io connection is active

		// Constructor -> private, use create()
		PJLinkConnection(pjlink::Context& context, const asio::ip::address& address, PJLinkProjector& projector);
	};
}
