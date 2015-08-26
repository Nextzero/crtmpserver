/*
 *  Copyright (c) 2010,
 *  Gavriloaie Eugen-Andrei (shiretu@gmail.com)
 *
 *  This file is part of crtmpserver.
 *  crtmpserver is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  crtmpserver is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with crtmpserver.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAS_PROTOCOL_CLI
#include "protocols/cli/basecliappprotocolhandler.h"
#include "protocols/cli/inboundbasecliprotocol.h"

BaseCLIAppProtocolHandler::BaseCLIAppProtocolHandler(Variant &configuration)
: BaseAppProtocolHandler(configuration) {

}

BaseCLIAppProtocolHandler::~BaseCLIAppProtocolHandler() {
}

void BaseCLIAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {

}

void BaseCLIAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {

}

bool BaseCLIAppProtocolHandler::SendFail(BaseProtocol *pTo, string description) {
	Variant dummy;
	return Send(pTo, "FAIL", description, dummy);
}

bool BaseCLIAppProtocolHandler::SendSuccess(BaseProtocol *pTo, string description, Variant &data) {
	return Send(pTo, "SUCCESS", description, data);
}

bool BaseCLIAppProtocolHandler::Send(BaseProtocol *pTo, string status, string description, Variant &data) {
	if (pTo == NULL)
		return true;
	//1. Prepare the final message
	Variant message;
	message["status"] = status;
	message["description"] = description;
	message["data"] = data;

	//3. Send it
	bool result;
	switch (pTo->GetType()) {
		case PT_INBOUND_JSONCLI:
			result = ((InboundBaseCLIProtocol *) pTo)->SendMessage(message);
			if ((pTo->GetFarProtocol())->GetType() == PT_HTTP_4_CLI) {
				// If this is an http based CLI, enqueue it for delete
				pTo->GracefullyEnqueueForDelete();
			}
			return result;
		default:
			WARN("Protocol %s not supported yet", STR(tagToString(pTo->GetType())));
			return false;
	}
}
#endif /* HAS_PROTOCOL_CLI */
