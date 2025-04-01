/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local includes
#include "pjlinkprojectorpool.h"
#include "pjlinkprojector.h"
#include "pjlinkcommand.h"

// External includes
#include <asio/write.hpp>
#include <asio/buffer.hpp>
#include <nap/logger.h>
#include <iterator>

RTTI_BEGIN_CLASS(nap::PJLinkProjectorPool)
RTTI_END_CLASS

namespace nap
{
	bool PJLinkProjectorPool::init(utility::ErrorState& error)
	{
		assert(mThread == nullptr);
		assert(mGuard  == nullptr);
		mGuard  = std::make_unique<pjlink::Guard>(asio::make_work_guard(mContext));
		mThread = std::make_unique<std::thread>([this]
			{
				// Run until guard is reset or context is stopped.
				// All handlers will be called from within this thread.
				this->mContext.run();
			}
		);
		return true;
	}


	void PJLinkProjectorPool::onDestroy()
	{
		if (mThread != nullptr)
		{
			assert(mGuard != nullptr);
			mGuard->reset();;
			mThread->join();

			mThread.reset(nullptr);
			mGuard.reset(nullptr);

		}
	}
}
