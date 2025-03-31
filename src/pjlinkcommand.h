/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External includes
#include <string>
#include <utility/dllexport.h>
#include <rtti/rttiutilities.h>

namespace nap
{
	/**
	 * PJLink protocol message specifications
	 */
	namespace pjlink
	{
		constexpr const int port				= 4352;			//< PJ Link communication port number
		constexpr const char terminator			= '\r';			//< PJ link msg terminator

		namespace cmd
		{
			constexpr const size_t size			= 136;			//< PJ link max cmd size
			constexpr const char header			= '%';			//< PJ link cmd header
			constexpr const char version		= '1';			//< PJ link cmd version
			constexpr const char seperator		= ' ';			//< PJ link seperator
			constexpr const char query			= '?';			//< PJ link query parameter
			namespace set {
				constexpr const char* power		= "POWR";		//< turn projector on(1) or off(0)
			}
		}

		namespace response
		{
			constexpr const char header			= '%';			// PJ link response header
			namespace authenticate {
				constexpr const char* header	= "PJLINK";		//< projector authentication response header
				constexpr const char* disabled	= "PJLINK 0";	//< projector authentication disabled (required!)
			}
		}
	}


	/**
	 * Standard text based PJLink command including response.
	 * Can be copied and moved.
	 */
	class NAPAPI PJLinkCommand 
	{
		RTTI_ENABLE()
	public:
		// Construct cmd from body and value
		PJLinkCommand(const std::string& body, const std::string& value);

		// Default destructor
		virtual ~PJLinkCommand() = default;

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

	private:
		std::string mCommand;					//< Full PJLink command message
		std::string mResponse;					//< Full PJLink command response
	};
}
