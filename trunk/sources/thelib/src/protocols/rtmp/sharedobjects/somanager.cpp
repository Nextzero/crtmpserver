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
#include "protocols/rtmp/sharedobjects/somanager.h"
#include "protocols/rtmp/sharedobjects/so.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/rtmp/messagefactories/messagefactories.h"

SOManager::SOManager() {

}

SOManager::~SOManager() {

	FOR_MAP(_sos, string, SO *, i) {
		delete MAP_VAL(i);
	}
	_sos.clear();
}

void SOManager::UnRegisterProtocol(BaseRTMPProtocol *pProtocol) {
	if (!MAP_HAS1(_protocolSos, pProtocol->GetId()))
		return;
	vector<SO *> sos = _protocolSos[pProtocol->GetId()];

	FOR_VECTOR_ITERATOR(SO *, sos, i) {
		SO *pSO = VECTOR_VAL(i);
		pSO->UnRegisterProtocol(pProtocol->GetId());
		if (pSO->GetSubscribersCount() == 0 && !pSO->IsPersistent()) {
			_sos.erase(pSO->GetName());
			delete pSO;
		}
	}

	_protocolSos.erase(pProtocol->GetId());
}

bool SOManager::Process(BaseRTMPProtocol *pFrom, Variant &request) {
	//TODO: This is a hack. We process both kinds of messages with ProcessSharedObject
	return ProcessSharedObject(pFrom, request);
}

SO * SOManager::GetSO(string name, bool persistent) {
	if (MAP_HAS1(_sos, name))
		return _sos[name];
	SO *pSO = new SO(name, persistent);
	_sos[name] = pSO;
	return pSO;
}

bool SOManager::ContainsSO(string name) {
	return MAP_HAS1(_sos, name);
}

bool SOManager::ProcessFlexSharedObject(BaseRTMPProtocol *pFrom, Variant &request) {
	FINEST("Message:\n%s", STR(request.ToString()));
	NYIR;
}

bool SOManager::ProcessSharedObject(BaseRTMPProtocol *pFrom, Variant &request) {
	//1. Get the name and the persistance property
	string name = M_SO_NAME(request);
	if (pFrom->GetType() == PT_OUTBOUND_RTMP)
		pFrom->SignalBeginSOProcess(name);

	SO *pSO = NULL;
	if (MAP_HAS1(_sos, name))
		pSO = _sos[name];

	//3. Hit the SO with the operations requested
	for (uint32_t i = 0; i < M_SO_PRIMITIVES(request).MapSize(); i++) {
		if (!ProcessSharedObjectPrimitive(pFrom, pSO, name, request, i)) {
			FATAL("Unable to process primitive %u from\n%s", i,
					STR(request.ToString()));
			if (pFrom->GetType() == PT_OUTBOUND_RTMP)
				pFrom->SignalEndSOProcess(name, M_SO_VER(request));
			return false;
		}
	}

	//4. Get the SO again, but ONLY is it is stil alive
	pSO = NULL;
	if (MAP_HAS1(_sos, name))
		pSO = _sos[name];

	//5. Track it if is alive
	if (pSO != NULL)
		pSO->Track();

	//6. Done
	if (pFrom->GetType() == PT_OUTBOUND_RTMP)
		pFrom->SignalEndSOProcess(name, M_SO_VER(request));
	return true;
}

bool SOManager::ProcessSharedObjectPrimitive(BaseRTMPProtocol *pFrom, SO *pSO,
		string name, Variant &request, uint32_t primitiveId) {
	Variant primitive = M_SO_PRIMITIVE(request, primitiveId);

	switch ((uint8_t) primitive[RM_SHAREDOBJECTPRIMITIVE_TYPE]) {
		case SOT_CS_CONNECT:
		{
			pSO = GetSO(name, M_SO_PERSISTANCE(request));
			pSO->RegisterProtocol(pFrom->GetId());
			ADD_VECTOR_END(_protocolSos[pFrom->GetId()], pSO);
			return true;
		}
		case SOT_CS_DISCONNECT:
		{
			UnRegisterProtocol(pFrom);
			return true;
		}
		case SOT_CS_DELETE_FIELD_REQUEST:
		{
			if (pSO == NULL) {
				FATAL("SO is null");
				return false;
			}

			FOR_MAP(primitive[RM_SHAREDOBJECTPRIMITIVE_PAYLOAD], string, Variant, i) {
				pSO->UnSet(MAP_VAL(i), M_SO_VER(request));
			}
			return true;
		}
		case SOT_CS_UPDATE_FIELD_REQUEST:
		{
			if (pSO == NULL) {
				FATAL("SO is null");
				return false;
			}

			FOR_MAP(primitive[RM_SHAREDOBJECTPRIMITIVE_PAYLOAD], string, Variant, i) {
				pSO->Set((string &) MAP_KEY(i), MAP_VAL(i), M_SO_VER(request),
						pFrom->GetId());
			}

			return true;
		}
		case SOT_BW_SEND_MESSAGE:
		{
			if (pFrom->GetType() == PT_OUTBOUND_RTMP) {
				return pFrom->HandleSOPrimitive(name, primitive);
			} else {
				if (pSO == NULL) {
					FATAL("SO is null");
					return false;
				}
				return pSO->SendMessage(request);
			}
		}
		case SOT_SC_INITIAL_DATA:
		case SOT_SC_CLEAR_DATA:
		case SOT_CS_UPDATE_FIELD:
		case SOT_SC_DELETE_FIELD:
		case SOT_CS_UPDATE_FIELD_ACK:
		{
			return pFrom->HandleSOPrimitive(name, primitive);
		}
		default:
		{
			FATAL("SO primitive not allowed here:\n%s", STR(primitive.ToString()));
			return false;
		}
	}
}

#endif /* HAS_PROTOCOL_RTMP */

