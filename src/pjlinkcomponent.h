/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local includes
#include "pjlinkprojector.h"

// External includes
#include <component.h>
#include <mutex>

namespace nap
{
	class PJLinkComponentInstance;

	/**
	 * Receives and forwards pjlink client messages.
	 */
	class NAPAPI PJLinkComponent : public Component
	{
		RTTI_ENABLE(Component)
		DECLARE_COMPONENT(PJLinkComponent, PJLinkComponentInstance)
	public:
		nap::ResourcePtr<PJLinkProjector> mProjector;			///< Property: 'Projector' Projector client connection
	};


	/**
	 * Receives and forwards pjlink client messages.
	 * Register to the ResponseReceived signal to receive projector messages.
	 */
	class NAPAPI PJLinkComponentInstance : public ComponentInstance
	{
		RTTI_ENABLE(ComponentInstance)
	public:
		PJLinkComponentInstance(EntityInstance& entity, Component& resource) :
			ComponentInstance(entity, resource)									{ }

		// Initialize
		bool init(utility::ErrorState& errorState) override;

		/**
		 * Consumes all received pjlink events and forwards them to potential listeners
		 */
		void update(double deltaTime) override;

	private:
		nap::PJLinkProjector* mProjector = nullptr;

		void onResponse(const PJLinkCommand&);
		nap::Slot<const PJLinkCommand&> mResponseSlot = { this, &PJLinkComponentInstance::onResponse };
		std::queue<PJLinkCommand> mrQueue;
		std::queue<PJLinkCommand> mcQueue;
		std::mutex mMutex;
	};
}
