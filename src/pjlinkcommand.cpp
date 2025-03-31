/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local includes
#include "pjlinkcommand.h"

// External includes
#include <assert.h>

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
}
