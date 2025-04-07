/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local includes
#include "pjlinkcommand.h"

// External includes
#include <assert.h>
#include <nap/logger.h>

// Base class
RTTI_BEGIN_CLASS(nap::PJLinkCommand)
	RTTI_CONSTRUCTOR(const std::string&, const std::string&)
	RTTI_PROPERTY("Command",	&nap::PJLinkCommand::mCommand,		nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Response",	&nap::PJLinkCommand::mResponse,		nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

// Set commands
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PJLinkSetCommand)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::PJLinkSetPowerCommand)
	RTTI_CONSTRUCTOR(bool)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::PJLinkSetAVMuteCommand)
	RTTI_CONSTRUCTOR(bool)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::PJLinkSetInputCommand)
	RTTI_CONSTRUCTOR(nap::PJLinkSetInputCommand::EType, nap::uint8)
RTTI_END_CLASS

// Get commands
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PJLinkGetCommand)
RTTI_END_CLASS

RTTI_DEFINE_CLASS(nap::PJLinkGetPowerCommand)
RTTI_DEFINE_CLASS(nap::PJLinkGetAVMuteCommand)
RTTI_DEFINE_CLASS(nap::PJLinkGetLampStatusCommand)
RTTI_DEFINE_CLASS(nap::PJLinkGetErrorStatusCommand)

RTTI_BEGIN_ENUM(nap::PJLinkSetInputCommand::EType)
	RTTI_ENUM_VALUE(nap::PJLinkSetInputCommand::EType::RGB,			"RGB"),
	RTTI_ENUM_VALUE(nap::PJLinkSetInputCommand::EType::Video,		"Video"),
	RTTI_ENUM_VALUE(nap::PJLinkSetInputCommand::EType::Digital,		"Video"),
	RTTI_ENUM_VALUE(nap::PJLinkSetInputCommand::EType::Storage,		"Storage"),
	RTTI_ENUM_VALUE(nap::PJLinkSetInputCommand::EType::Network,		"Network")
RTTI_END_ENUM


namespace nap
{
	static std::string createCmd(const std::string& cmd, const std::string& value)
	{
		std::string r;
		r.reserve(value.size() + 8);
		r += pjlink::cmd::header;
		r += pjlink::cmd::version;
		r += cmd;
		r += pjlink::cmd::seperator;
		r += value;
		r += pjlink::terminator;

		assert(r.size() < pjlink::cmd::size);
		assert(sizeof(std::string::value_type) == sizeof(char));
		return r;
	}


	PJLinkCommand::PJLinkCommand(const std::string& cmd, const std::string& value)
	{
		mCommand = createCmd(cmd, value);
	}


	std::string nap::PJLinkCommand::getResponse() const
	{
		if (mResponse.empty())
		{
			assert(false);
			return "";
		}

		// Get loc of response
		auto loc = mResponse.find_last_of(pjlink::cmd::equals);
		assert(loc != std::string::npos);

		// Return substring
		loc++; assert(loc < mResponse.size());
		return mResponse.substr(loc, mResponse.size() - loc);
	}


	std::string nap::PJLinkCommand::getCommand() const
	{
		if (mCommand.empty())
		{
			assert(false);
			return "";
		}

		auto count = mCommand.size() - sizeof(pjlink::terminator) - 2;
		assert(mCommand.back() == pjlink::terminator &&  count > 0);
		return mCommand.substr(2, count);
	}


	nap::PJLinkCommandPtr PJLinkCommand::clone() const
	{
		// Create clone
		assert(this->get_type().can_create_instance());
		auto clone = std::unique_ptr<PJLinkCommand>(get_type().create<PJLinkCommand>());
		if (clone == nullptr)
		{
			assert(false);
			nap::Logger::error("Failed to clone PJLink command of type: '%s'",
				this->get_type().get_name().data());
			return nullptr;
		}

		// Copy properties
		for (const rtti::Property& property : get_type().get_properties())
		{
			rtti::Variant new_value = property.get_value(*this);
			bool success = property.set_value(*clone, new_value);
			assert(success);
		}
		return clone;
	}


	PJLinkSetInputCommand::PJLinkSetInputCommand(EType type, nap::uint8 number)
	{
		assert(number > 0 && number < 10);
		std::string is;
		is += static_cast<char>(type);
		is += static_cast<char>(number + 0x30);
		mCommand = createCmd(pjlink::cmd::set::input, is);
	}


	nap::PJLinkGetPowerCommand::EStatus PJLinkGetPowerCommand::getPowerStatus() const
	{
		if (mResponse.empty())
			return EStatus::Invalid;

		return EStatus::Cooling;
	}

}
