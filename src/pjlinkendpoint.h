/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local includes
#include "pjlinkprojector.h"

// External includes
#include <nap/device.h>
#include <nap/resourceptr.h>

namespace nap
{
	/**
	 * PJLink client pool.
	 * Manages multiple projector connections thread-safe.
	 */
	class PJLinkEndPoint : public Resource
	{
		RTTI_ENABLE(Resource)
    public:
		// Default constructor
		PJLinkEndPoint() = default;
    };
}