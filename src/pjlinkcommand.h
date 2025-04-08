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
	class PJLinkCommand;					//< Generic PJLink Command

	// Set commands
	class PJLinkSetPowerCommand;			//< Turn power on or off
	class PJLinkSetAVMuteCommand;			//< Turn avmute (audio & video) on or off
	class PJLinkSetInputCommand;			//< Select input

	// Get commands
	class PJLinkGetPowerCommand;			//< Get current power status 
	class PJLinkGetAVMuteCommand;			//< Get current audio visual mute status
	class PJLinkGetLampStatusCommand;		//< Get current lamp hours
	class PJLinkGetErrorStatusCommand;		//< Get error status

	/**
	 * PJLink protocol message specifications
	 */
	namespace pjlink
	{
		constexpr const int port = 4352;						//< PJ Link communication port number
		constexpr const char terminator = '\r';					//< PJ link msg terminator

		namespace cmd
		{
			constexpr const size_t size = 136;					//< PJ link max cmd size
			constexpr const char header = '%';					//< PJ link cmd header
			constexpr const char version = '1';					//< PJ link cmd version
			constexpr const char seperator = ' ';				//< PJ link seperator
			constexpr const char query = '?';					//< PJ link query parameter
			constexpr const char equals = '=';					//< PJ link equals parameter
			constexpr const char* error = "ERR";				//< PJ link error response parameter

			namespace set
			{
				constexpr const char* power = "POWR";			//< Turn projector on(1) or off(0)
				constexpr const char* avmute = "AVMT";			//< Turn video(10,11), audio(20,21) or both(30,31) on or off
				constexpr const char* input = "INPT";			//< Select input, RGB(1n), VIDEO(2n), DIGITAL(3n)
				constexpr const char* ok = "OK";				//< If the message has been received and processed by the projector
			}
			namespace get
			{
				constexpr const char* power = "POWR";			//< Power query -> 0(off), 1(on), 2(cooling), 3(warming)
				constexpr const char* avmute = "AVMT";			//< Mute query -> x1(on), x0(off)
				constexpr const char* error = "ERST";			//< Error status -> 1(fan), 2(lamp), 3(temp), 4(cover), 5(filter), 6(other)
				constexpr const char* hours = "LAMP";			//< Lamp hours -> x

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

	class PJLinkCommand;
	using PJLinkCommandPtr = std::unique_ptr<PJLinkCommand>;

	/**
	 * Standard text based PJLink command including response.
	 * Use this for custom pjlink commands without specialization.
	 * Can be copied and moved.
	 */
	class NAPAPI PJLinkCommand
	{
		RTTI_ENABLE()
	public:
		enum class EResponseCode : char
		{
			Ok					= '0',		//< Command accepted
			SupportError		= '1',		//< Unsupported command
			ParameterError		= '2',		//< Parameter out of bounds
			TimeError			= '3',		//< Time issue
			ProjectorError		= '4',		//< Projector display failure
			Invalid				= 0x00		//< No response
		};

		// Construct cmd from body and value
		PJLinkCommand(const std::string& body, const std::string& value);

		// Creates an invalid pjlink command
		PJLinkCommand() = default;

		// Default destructor
		virtual ~PJLinkCommand() = default;

		/**
		 * @return cmd characters
		 */
		const char* data()						{ return mCommand.data(); }

		/**
		 * @return cmd byte size
		 */
		size_t size()							{ return mCommand.size(); }

		/**
		 * Returns formatted command excluding header, response & terminator.
		 * Includes only the command body
		 * @return projector command
		 */
		std::string getCommand() const;

		/**
		 * @return if there is a response
		 */
		bool hasResponse() const				{ return !mResponse.empty(); }

		/**
		 * Returns formatted response excluding header, command & terminator.
		 * Includes only the received parameter body.
		 * @return projector response message
		 */
		std::string getResponse() const;

		/**
		 * Returns response error code
		 * @return Response error code
		 */
		EResponseCode getResponseCode() const;

		/**
		 * A clone of this command, including all derived properties
		 */
		PJLinkCommandPtr clone() const;

		std::string mCommand;					//< Full PJLink command message, including header & terminator
		std::string mResponse;					//< Full PJLink command response, including header & terminator
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

		PJLinkSetCommand() = default;

		/**
		 * @return if the projector received and processed the request
		 */
		bool success() const;
	};


	// Power on / off
	class NAPAPI PJLinkSetPowerCommand : public PJLinkSetCommand
	{
		RTTI_ENABLE(PJLinkSetCommand)
	public:
		PJLinkSetPowerCommand(bool value) :
			PJLinkSetCommand(pjlink::cmd::set::power, value ? "1" : "0")		{ }

		PJLinkSetPowerCommand() = default;
	};


	// Mute (av) on / off
	class NAPAPI PJLinkSetAVMuteCommand : public PJLinkSetCommand
	{
		RTTI_ENABLE(PJLinkSetCommand)
	public:
		PJLinkSetAVMuteCommand(bool value) :
			PJLinkSetCommand(pjlink::cmd::set::avmute, value ? "31" : "30")		{ }

		// Invalid set command
		PJLinkSetAVMuteCommand() = default;
	};


	// Input selection
	class NAPAPI PJLinkSetInputCommand : public PJLinkSetCommand
	{
		RTTI_ENABLE(PJLinkSetCommand)
	public:
		// Default constructor
		PJLinkSetInputCommand() = default;

		// Available input types
		enum class EType : char
		{
			RGB			= '1',
			Video		= '2',
			Digital		= '3',
			Storage		= '4',
			Network		= '5'
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
			PJLinkCommand(body, &pjlink::cmd::query)	{ }

		// Invalid get command
		PJLinkGetCommand() = default;
	};


	// Get power status
	class NAPAPI PJLinkGetPowerCommand : public PJLinkGetCommand
	{
		RTTI_ENABLE(PJLinkGetCommand)
	public:
		enum class EStatus : char
		{
			Off				= '0',		//< Projector is off
			On				= '1',		//< Projector is on
			Cooling			= '2',		//< Projector is cooling down
			WarmingUp		= '3',		//< Projector is warming up
			TimeError		= '4',		//< Projector is unavailable
			ProjectorError	= '5',		//< Projector power error
			Unknown			= 0x00		//< Response not available
		};

		PJLinkGetPowerCommand() :
			PJLinkGetCommand(pjlink::cmd::get::power)		{ }

		/**
		 * @return power status
		 */
		EStatus getStatus() const;
	};


	// Get mute status
	class NAPAPI PJLinkGetAVMuteCommand : public PJLinkGetCommand
	{
		RTTI_ENABLE(PJLinkGetCommand)
	public:
		enum class EStatus : char
		{
			Off				= '0',		
			On				= '1',		// 31! == (audio & video)
			TimeError		= '3',
			ProjectorError	= '4',
			Unknown			= 0x00
		};

		PJLinkGetAVMuteCommand() :
			PJLinkGetCommand(pjlink::cmd::get::avmute)		{ }

		/**
		 * @return av mute status
		 */
		EStatus getStatus() const;
	};


	// Get lamp status
	class NAPAPI PJLinkGetLampStatusCommand : public PJLinkGetCommand
	{
		RTTI_ENABLE(PJLinkGetCommand)
	public:
		PJLinkGetLampStatusCommand() :
			PJLinkGetCommand(pjlink::cmd::get::hours)		{ }

		/**
		 * Total number of hours, -1 if response is invalid
		 */
		int getHours() const;
	};


	// Get error status
	class NAPAPI PJLinkGetErrorStatusCommand : public PJLinkGetCommand
	{
		RTTI_ENABLE(PJLinkGetCommand)
	public:
		enum class EStatus : nap::uint16
		{
			None			= 0x000,		//< No issue detected
			Fan				= 0x001,		//< Fan issue
			Lamp			= 0x002,		//< Lamp issue
			Temerature		= 0x004,		//< Temperature issue
			Cover			= 0x008,		//< Cover issue
			Filter			= 0x010,		//< Filter issue
			Other			= 0x020,		//< Other issue
			TimeError		= 0x040,		//< Projector is unavailable
			ProjectorError	= 0x080,		//< Projector error
			Unknown			= 0x100			//< Response not available
		};

		PJLinkGetErrorStatusCommand() :
			PJLinkGetCommand(pjlink::cmd::get::error)		{ }

		/**
		 * Returns warning bits, 0 if no response is available 
		 * @return warning bits
		 */
		nap::uint16 getWarnings() const;

		/**
		 * @return all errors as ', ' separated string
		 */
		std::string warningsToString() const;

		/**
		 * Returns error bits, 0 if no response is available 
		 * @return error bits
		 */
		nap::uint16 getErrors() const;

		/**
		 * @return all errors as ', ' separated string
		 */
		std::string errorsToString() const;

		/**
		 * Turns error or warning bit-mask into ', ' separated string  
		 * @return if conversion succeeded
		 */
		static bool toString(nap::uint16 mask, std::string& outString, utility::ErrorState& error);

		/**
		 * @return if warning is present for given bit
		 */
		bool getWarning(EStatus status) const				{ return (getWarnings() & static_cast<nap::uint>(status)) > 0; }

		/**
		 * @return if error is present for given bit
		 */
		bool getError(EStatus status) const					{ return (getErrors() & static_cast<nap::uint>(status)) > 0; }
	};
}
