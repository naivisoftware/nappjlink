/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External includes
#include <string>
#include <utility/dllexport.h>
#include <rtti/rttiutilities.h>
#include <nap/numeric.h>

namespace nap
{
	/**
	 * PJLink protocol message specifications
	 */
	namespace pjlink
	{
		constexpr const int port = 4352;					//< PJ Link communication port number
		constexpr const char terminator = '\r';				//< PJ link msg terminator

		namespace cmd
		{
			constexpr const size_t size = 136;				//< PJ link max cmd size
			constexpr const char header = '%';				//< PJ link cmd header
			constexpr const char version = '1';				//< PJ link cmd version
			constexpr const char seperator = ' ';			//< PJ link seperator
			constexpr const char query = '?';				//< PJ link query parameter
			namespace set
			{
				constexpr const char* power = "POWR";		//< Turn projector on(1) or off(0)
				constexpr const char* avmute = "AVMT";		//< Turn video(10,11), audio(20,21) or both(30,31) on or off
				constexpr const char* input = "INPT";		//< Select input, RGB(1n), VIDEO(2n), DIGITAL(3n)
			}
			namespace get
			{
				constexpr const char* power = "POWR";		//< Power query -> 0(off), 1(on), 2(cooling), 3(warming)
				constexpr const char* avmute = "AVMT";		//< Mute query -> x1(on), x0(off)
				constexpr const char* error = "ERST";		//< Error status -> 1(fan), 2(lamp), 3(temp), 4(cover), 5(filter), 6(other)
				constexpr const char* hours = "LAMP";		//< Lamp hours -> x

			}
		}

		namespace response
		{
			constexpr const char header = '%';					// PJ link response header
			namespace authenticate {
				constexpr const char* header = "PJLINK";		//< projector authentication response header
				constexpr const char* disabled = "PJLINK 0";	//< projector authentication disabled (required!)
			}
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// Commands
	//////////////////////////////////////////////////////////////////////////

	/**
	 * Standard text based PJLink command including response.
	 * Use this for custom pjlink commands without specialization.
	 * Can be copied and moved.
	 */
	class NAPAPI PJLinkCommand 
	{
		RTTI_ENABLE()
	public:
		// Construct cmd from body and value
		PJLinkCommand(const std::string& body, const std::string& value);

		/**
		 * @return projector command
		 */
		const std::string& getCommand()			{ return mCommand; }

		/**
		 * @return cmd characters
		 */
		const char* data()						{ return mCommand.data(); }

		/**
		 * @return cmd byte size
		 */
		size_t size()							{ return mCommand.size(); }

		/**
		 * @return projector response message
		 */
		const std::string& getResponse()		{ return mResponse; }

		std::string mCommand;					//< Full PJLink command message
		std::string mResponse;					//< Full PJLink command response

	protected:
		PJLinkCommand() = default;
	};


	//////////////////////////////////////////////////////////////////////////
	// Set commands
	//////////////////////////////////////////////////////////////////////////

	// Set cmd
	class NAPAPI PJLinkSetCommand : public PJLinkCommand
	{
		RTTI_ENABLE(PJLinkCommand)
	public:
		PJLinkSetCommand(const std::string& body, const std::string& value) :
			PJLinkCommand(body, value) { }

	protected:
		PJLinkSetCommand() = default;
	};

	// Power on / off
	class NAPAPI PJLinkSetPowerCommand : public PJLinkSetCommand
	{
		RTTI_ENABLE(PJLinkSetCommand)
	public:
		PJLinkSetPowerCommand(bool value) :
			PJLinkSetCommand(pjlink::cmd::set::power, value ? "1" : "0")		{ }
	};

	// Mute (av) on / off
	class NAPAPI PJLinkSetAVMuteCommand : public PJLinkSetCommand
	{
		RTTI_ENABLE(PJLinkSetCommand)
	public:
		PJLinkSetAVMuteCommand(bool value) :
			PJLinkSetCommand(pjlink::cmd::set::avmute, value ? "31" : "30")	{ }
	};

	// Input selection
	class NAPAPI PJLinkSetInputCommand : public PJLinkSetCommand
	{
		RTTI_ENABLE(PJLinkSetCommand)
	public:
		// Available input types
		enum class EType : char
		{
			RGB			= 1,
			Video		= 2,
			Digital		= 3,
			Storage		= 4,
			Network		= 5
		};

		/**
		 * @param type input type
		 * @param number input number (1-9)
		 */
		PJLinkSetInputCommand(EType type, nap::uint8 number);
	};


	//////////////////////////////////////////////////////////////////////////
	// Get commands
	//////////////////////////////////////////////////////////////////////////

	// Set cmd
	class NAPAPI PJLinkGetCommand : public PJLinkCommand
	{
		RTTI_ENABLE(PJLinkCommand)
	public:
		PJLinkGetCommand(const std::string& body) :
			PJLinkCommand(body, &pjlink::cmd::query) { }
	};

	// Get power status
	class NAPAPI PJLinkGetPowerCommand : public PJLinkGetCommand
	{
		RTTI_ENABLE(PJLinkGetCommand)
	public:
		PJLinkGetPowerCommand() :
			PJLinkGetCommand(pjlink::cmd::get::power)		{ }
	};

	// Get mute status
	class NAPAPI PJLinkGetAVMuteCommand : public PJLinkGetCommand
	{
		RTTI_ENABLE(PJLinkGetCommand)
	public:
		PJLinkGetAVMuteCommand() :
			PJLinkGetCommand(pjlink::cmd::get::avmute)	{ }
	};

	// Get lamp status
	class NAPAPI PJLinkGetLampStatusCommand : public PJLinkGetCommand
	{
		RTTI_ENABLE(PJLinkGetCommand)
	public:
		PJLinkGetLampStatusCommand() :
			PJLinkGetCommand(pjlink::cmd::get::hours)		{ }
	};

	// Get error status
	class NAPAPI PJLinkGetErrorStatusCommand : public PJLinkGetCommand
	{
		RTTI_ENABLE(PJLinkGetCommand)
	public:
		PJLinkGetErrorStatusCommand() :
			PJLinkGetCommand(pjlink::cmd::get::error)		{ }
	};
}
