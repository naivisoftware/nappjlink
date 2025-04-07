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
	 * Receives and forwards pjlink client messages on the main thread.
	 */
	class NAPAPI PJLinkComponent : public Component
	{
		RTTI_ENABLE(Component)
		DECLARE_COMPONENT(PJLinkComponent, PJLinkComponentInstance)
	public:
		nap::ResourcePtr<PJLinkProjector> mProjector;			///< Property: 'Projector' Projector client connection
	};


	/**
	 * Receives and forwards pjlink client messages on the main thread.
	 * 
	 * Register to the messageReceived signal to receive projector messages.
	 * The signal is invoked on the main (application) thread, on update() of this component.
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

		/**
		 * @return assigned projector
		 */
		PJLinkProjector& getProjector()					{ assert(mProjector != nullptr); return *mProjector; }

		/**
		 * @return assigned projector
		 */
		const PJLinkProjector& getProjector() const		{ assert(mProjector != nullptr); return *mProjector; }

		/**
		 * Called when the component receives a message from the assigned projector.
		 * The signal is invoked on the main (application) thread, on update() of this component.
		 */
		nap::Signal<const PJLinkComponentInstance&, const PJLinkCommand&> messageReceived;

	private:
		nap::PJLinkProjector* mProjector = nullptr;

		// Called from pjlink event thread
		void onResponse(const PJLinkCommand&);
		nap::Slot<const PJLinkCommand&> mResponseSlot = { this, &PJLinkComponentInstance::onResponse };
		std::queue<PJLinkCommandPtr> mrQueue;
		std::queue<PJLinkCommandPtr> mcQueue;
		std::mutex mMutex;
	};
}
