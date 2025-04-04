#include "pjlinkcomponent.h"

// External Includes
#include <entity.h>

// nap::pjlinkcomponent run time class definition 
RTTI_BEGIN_CLASS(nap::PJLinkComponent)
	RTTI_PROPERTY("Projector", &nap::PJLinkComponent::mProjector,  nap::rtti::EPropertyMetaData::Required, "Projector Client Connection")
RTTI_END_CLASS

// nap::pjlinkcomponentInstance run time class definition 
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PJLinkComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////


namespace nap
{
	bool PJLinkComponentInstance::init(utility::ErrorState& errorState)
	{
		// Listen for changes
		auto resource = getComponent<PJLinkComponent>();
		mProjector = resource->mProjector.get();
		mProjector->ResponseReceived.connect(mResponseSlot);

		return true;
	}


	void PJLinkComponentInstance::onResponse(const PJLinkCommand& cmd)
	{
		auto clone = cmd.clone();
		assert(clone != nullptr);
		std::lock_guard<std::mutex> lock(mMutex);
		mrQueue.emplace(std::move(clone));
	}


	void PJLinkComponentInstance::update(double deltaTime)
	{
		// Swap messages thread-safe
		{
			std::lock_guard<std::mutex> lock(mMutex);
			assert(mcQueue.empty());
			mrQueue.swap(mcQueue);
		}

		// Consume messages
		while (!mcQueue.empty())
		{
			messageReceived(*mcQueue.front());
			mcQueue.pop();
		}
	}
}
