/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local includes
#include "pjlinkconnection.h"
#include "pjlinkprojector.h"
#include "pjlinkcommand.h"

// External includes
#include <asio/connect.hpp>
#include <asio/read.hpp>
#include <nap/logger.h>
#include <asio/read_until.hpp>
#include <string.h>

using namespace asio::ip;

namespace nap
{
	PJLinkConnection::PJLinkConnection(pjlink::Context& context, PJLinkProjector& projector) :
		mSocket(context), mProjector(&projector)
	{
		connect();
	}


	void PJLinkConnection::connect()
	{
		mEndpoint = tcp::endpoint(address::from_string(mProjector->mIPAddress), pjlink::port);
		mSocket.async_connect(mEndpoint, [this, ep = mEndpoint, id = mProjector->mID](std::error_code ec)
			{
				if (ec)
				{
					nap::Logger::error("Failed (ec '%d') to connect to projector '%s', endpoint: %s, port: %d",
						ec.value(), id.c_str(), ep.address().to_string().c_str(), ep.port());

					// TODO: CLOSE
				}

				// Connection success -> verify authentification
				nap::Logger::info("%s: Connected to projector '%s', port: %d",
					ep.address().to_string().c_str(), id.c_str(), ep.port());

				// Authenticate
				authenticate(ep);
			});
	}


	void PJLinkConnection::authenticate(pjlink::EndPoint ep)
	{
		asio::async_read_until(mSocket, mAuthBuffer, pjlink::terminator, [this, ep](std::error_code ec,  std::size_t size)
			{
				if (ec)
				{
					nap::Logger::error("Failed (ec '%d') to authorize projector at endpoint: %s",
						ec.value(), ep.address().to_string().c_str());

					// TODO: CLOSE
				}

				// Commit to string
				nap::Logger::info("%s: Received %d authorization bytes", ep.address().to_string().c_str(), size);
				std::istream is(&mAuthBuffer); std::string response;
				std::getline(std::istream(&mAuthBuffer), response, pjlink::terminator);

				// Ensure it's an authentication header
				if (!utility::startsWith(response, pjlink::response::authenticate::header, false))
				{
					nap::Logger::error("Projector '%s' authentication failed, invalid response: %s",
						ep.address().to_string().c_str(), response.c_str());

					// TODO: Close
				}

				// Ensure authentication is diabled
				if (!utility::startsWith(response, pjlink::response::authenticate::disabled, false))
				{
					nap::Logger::error("Projector authentication requested -> not supported, \
						disable authentication at endpoint: %s",
						ep.address().to_string().c_str());

					// TODO: Close
				}

				// All good
				nap::Logger::info("%s: Authentication succeeded",
					mEndpoint.address().to_string().c_str());
			});
	}


	nap::PJLinkConnection& PJLinkConnection::operator=(PJLinkConnection&& other) noexcept
	{
		mSocket = std::move(other.mSocket);
		mProjector = other.mProjector;
		other.mProjector = nullptr;
		return *this;
	}


	PJLinkConnection::PJLinkConnection(PJLinkConnection&& other) noexcept:
		mSocket(std::move(other.mSocket)),
		mProjector(other.mProjector)
	{
		other.mProjector = nullptr;
	}


	PJLinkConnection::~PJLinkConnection()
	{
		if (mSocket.is_open())
		{
			mSocket.cancel();
			mSocket.close();
		}
	}
}
