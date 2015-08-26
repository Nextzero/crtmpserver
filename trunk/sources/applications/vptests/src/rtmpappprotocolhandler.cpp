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


#ifdef HAS_PROTOCOL_RTMP
#include "rtmpappprotocolhandler.h"
#include "variantappprotocolhandler.h"
#include "protocols/protocoltypes.h"
using namespace app_vptests;

RTMPAppProtocolHandler::RTMPAppProtocolHandler(Variant &configuration)
: BaseRTMPAppProtocolHandler(configuration) {

}

RTMPAppProtocolHandler::~RTMPAppProtocolHandler() {
}

bool RTMPAppProtocolHandler::ProcessInvokeConnect(BaseRTMPProtocol *pFrom, Variant &request) {
	request["Buggy_node"] = "<map>&some other xml stuff</map>";
	if (!Send("http://localhost/~shiretu/phpframework/input.php", request)) {
		FATAL("Unable to send the variant request");
		return false;
	}
	return true;
}

VariantAppProtocolHandler *RTMPAppProtocolHandler::GetVariantHandler(
		VariantSerializer serializer) {
#ifdef HAS_PROTOCOL_VAR
	switch (serializer) {
		case VariantSerializer_BIN:
			return (VariantAppProtocolHandler *) GetProtocolHandler(PT_BIN_VAR);
		case VariantSerializer_XML:
			return (VariantAppProtocolHandler *) GetProtocolHandler(PT_XML_VAR);
		case VariantSerializer_JSON:
			return (VariantAppProtocolHandler *) GetProtocolHandler(PT_JSON_VAR);
		default:
		{
			ASSERT("Invalid variant serializer type: %d", serializer);
			return NULL;
		}
	}
#else
	FATAL("Variant protocol not available");
	return NULL;
#endif /* HAS_PROTOCOL_VAR */
}

bool RTMPAppProtocolHandler::Send(string ip, uint16_t port, Variant &variant,
		VariantSerializer serializer) {
#ifdef HAS_PROTOCOL_VAR
	VariantAppProtocolHandler *pHandler = GetVariantHandler(serializer);
	if (pHandler == NULL) {
		FATAL("Unable to get the protocol handler");
		return false;
	}
	return pHandler->Send(ip, port, variant, serializer);
#else
	FATAL("Variant protocol not available");
	return false;
#endif /* HAS_PROTOCOL_VAR */
}

bool RTMPAppProtocolHandler::Send(string url, Variant &variant,
		VariantSerializer serializer) {
#ifdef HAS_PROTOCOL_VAR
	VariantAppProtocolHandler *pHandler = GetVariantHandler(serializer);
	if (pHandler == NULL) {
		FATAL("Unable to get the protocol handler");
		return false;
	}
	return pHandler->Send(url, variant, serializer);
#else
	FATAL("Variant protocol not available");
	return false;
#endif /* HAS_PROTOCOL_VAR */
}
#endif /* HAS_PROTOCOL_RTMP */

