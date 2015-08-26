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


#include "streaming/baseinstream.h"
#include "streaming/baseoutstream.h"
#include "streaming/streamstypes.h"
#include "streaming/streamsmanager.h"
#include "streaming/codectypes.h"
#include "protocols/baseprotocol.h"

BaseInStream::BaseInStream(BaseProtocol *pProtocol, uint64_t type, string name)
: BaseStream(pProtocol, type, name) {
	if (!TAG_KIND_OF(type, ST_IN)) {
		ASSERT("Incorrect stream type. Wanted a stream type in class %s and got %s",
				STR(tagToString(ST_IN)), STR(tagToString(type)));
	}
	_pOutStreams = NULL;
	_canCallOutStreamDetached = true;
}

BaseInStream::~BaseInStream() {
	_canCallOutStreamDetached = false;
	while (_linkedStreams.size() > 0) {
		UnLink(MAP_VAL(_linkedStreams.begin()), true);
	}
}

vector<BaseOutStream *> BaseInStream::GetOutStreams() {
	vector<BaseOutStream *> result;
	LinkedListNode<BaseOutStream *> *pIterator = _pOutStreams;
	LinkedListNode<BaseOutStream *> *pCurrent = NULL;
	while (pIterator != NULL) {
		pCurrent = pIterator;
		pIterator = pIterator->pPrev;
		ADD_VECTOR_END(result, pCurrent->info);
	}
	return result;
}

void BaseInStream::GetStats(Variant &info, uint32_t namespaceId) {
	BaseStream::GetStats(info, namespaceId);
	info["outStreamsUniqueIds"] = Variant();
	LinkedListNode<BaseOutStream *> *pIterator = _pOutStreams;
	LinkedListNode<BaseOutStream *> *pCurrent = NULL;
	while (pIterator != NULL) {
		pCurrent = pIterator;
		pIterator = pIterator->pPrev;
		info["outStreamsUniqueIds"].PushToArray(
				((((uint64_t) namespaceId) << 32) | pCurrent->info->GetUniqueId()));
	}
	StreamCapabilities *pCapabilities = GetCapabilities();
	if (pCapabilities != NULL)
		info["bandwidth"] = (uint32_t) (pCapabilities->GetTransferRate() / 1024.0);
	else
		info["bandwidth"] = (uint32_t) 0;
}

bool BaseInStream::Link(BaseOutStream *pOutStream, bool reverseLink) {
	if ((!pOutStream->IsCompatibleWithType(GetType()))
			|| (!IsCompatibleWithType(pOutStream->GetType()))) {
		FATAL("stream type %s not compatible with stream type %s",
				STR(tagToString(GetType())),
				STR(tagToString(pOutStream->GetType())));
		return false;
	}
	if (MAP_HAS1(_linkedStreams, pOutStream->GetUniqueId())) {
		WARN("BaseInStream::Link: This stream is already linked");
		return true;
	}
	_pOutStreams = AddLinkedList(_pOutStreams, pOutStream, true);
	_linkedStreams[pOutStream->GetUniqueId()] = pOutStream;

	if (reverseLink) {
		if (!pOutStream->Link(this, false)) {
			FATAL("BaseInStream::Link: Unable to reverse link");
			//TODO: here we must remove the link from _pOutStreams and _linkedStreams
			NYIA;
		}
	}
	SignalOutStreamAttached(pOutStream);
	return true;
}

bool BaseInStream::UnLink(BaseOutStream *pOutStream, bool reverseUnLink) {
	if (!MAP_HAS1(_linkedStreams, pOutStream->GetUniqueId())) {
		//WARN("BaseInStream::UnLink: This stream is not linked");
		return true;
	}
	_pStreamsManager->SignalUnLinkingStreams(this, pOutStream);

	_linkedStreams.erase(pOutStream->GetUniqueId());
	LinkedListNode<BaseOutStream *> *pIterator = _pOutStreams;
	LinkedListNode<BaseOutStream *> *pCurrent = NULL;
	while (pIterator != NULL) {
		pCurrent = pIterator;
		pIterator = pIterator->pPrev;
		if (pCurrent->info->GetUniqueId() == pOutStream->GetUniqueId()) {
			_pOutStreams = LastLinkedList<BaseOutStream *>(RemoveLinkedList<BaseOutStream *>(pCurrent));
			break;
		}
	}

	if (reverseUnLink) {
		if (!pOutStream->UnLink(false)) {
			FATAL("BaseInStream::UnLink: Unable to reverse unLink");
			//TODO: what are we going to do here???
			NYIA;
		}
	}
	if (_canCallOutStreamDetached) {
		SignalOutStreamDetached(pOutStream);
	}
	return true;
}

bool BaseInStream::Play(double dts, double length) {
	if (!SignalPlay(dts, length)) {
		FATAL("Unable to signal play");
		return false;
	}
	LinkedListNode<BaseOutStream *> *pIterator = _pOutStreams;
	LinkedListNode<BaseOutStream *> *pCurrent = NULL;
	while (pIterator != NULL) {
		pCurrent = pIterator;
		pIterator = pIterator->pPrev;
		if (!pCurrent->info->SignalPlay(dts, length)) {
			WARN("Unable to signal play on an outbound stream");
		}
	}
	return true;
}

bool BaseInStream::Pause() {
	if (!SignalPause()) {
		FATAL("Unable to signal pause");
		return false;
	}
	LinkedListNode<BaseOutStream *> *pIterator = _pOutStreams;
	LinkedListNode<BaseOutStream *> *pCurrent = NULL;
	while (pIterator != NULL) {
		pCurrent = pIterator;
		pIterator = pIterator->pPrev;
		if (!pCurrent->info->SignalPause()) {
			WARN("Unable to signal pause on an outbound stream");
		}
	}
	return true;
}

bool BaseInStream::Resume() {
	if (!SignalResume()) {
		FATAL("Unable to signal resume");
		return false;
	}
	LinkedListNode<BaseOutStream *> *pIterator = _pOutStreams;
	LinkedListNode<BaseOutStream *> *pCurrent = NULL;
	while (pIterator != NULL) {
		pCurrent = pIterator;
		pIterator = pIterator->pPrev;
		if (!pCurrent->info->SignalResume()) {
			WARN("Unable to signal resume on an outbound stream");
		}
	}
	return true;
}

bool BaseInStream::Seek(double dts) {
	LinkedListNode<BaseOutStream *> *pIterator = _pOutStreams;
	LinkedListNode<BaseOutStream *> *pCurrent = NULL;
	while (pIterator != NULL) {
		pCurrent = pIterator;
		pIterator = pIterator->pPrev;
		if (!pCurrent->info->SignalSeek(dts)) {
			WARN("Unable to signal seek on an outbound stream");
		}
	}

	if (!SignalSeek(dts)) {
		FATAL("Unable to signal seek");
		return false;
	}

	return true;
}

bool BaseInStream::Stop() {
	if (!SignalStop()) {
		FATAL("Unable to signal stop");
		return false;
	}
	LinkedListNode<BaseOutStream *> *pIterator = _pOutStreams;
	LinkedListNode<BaseOutStream *> *pCurrent = NULL;
	while (pIterator != NULL) {
		pCurrent = pIterator;
		pIterator = pIterator->pPrev;
		if (!pCurrent->info->SignalStop()) {
			WARN("Unable to signal stop on an outbound stream");
		}
	}
	return true;
}

void BaseInStream::AudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	LinkedListNode<BaseOutStream *> *pIterator = _pOutStreams;
	LinkedListNode<BaseOutStream *> *pCurrent = NULL;
	while (pIterator != NULL) {
		pCurrent = pIterator;
		pIterator = pIterator->pPrev;
		pCurrent->info->SignalAudioStreamCapabilitiesChanged(pCapabilities, pOld,
				pNew);
		if (IsEnqueueForDelete())
			return;
	}
}

void BaseInStream::VideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	LinkedListNode<BaseOutStream *> *pIterator = _pOutStreams;
	LinkedListNode<BaseOutStream *> *pCurrent = NULL;
	while (pIterator != NULL) {
		pCurrent = pIterator;
		pIterator = pIterator->pPrev;
		pCurrent->info->SignalVideoStreamCapabilitiesChanged(pCapabilities, pOld,
				pNew);
		if (IsEnqueueForDelete())
			return;
	}
}

StreamCapabilities * BaseInStream::GetCapabilities() {
	return NULL;
}

uint32_t BaseInStream::GetInputVideoTimescale() {
	StreamCapabilities *pCapabilities = NULL;
	VideoCodecInfo *pCodecInfo = NULL;
	if (((pCapabilities = GetCapabilities()) == NULL)
			|| ((pCodecInfo = pCapabilities->GetVideoCodec()) == NULL))
		return 1;
	return pCodecInfo->_samplingRate;
}

uint32_t BaseInStream::GetInputAudioTimescale() {
	StreamCapabilities *pCapabilities = NULL;
	AudioCodecInfo *pCodecInfo = NULL;
	if (((pCapabilities = GetCapabilities()) == NULL)
			|| ((pCodecInfo = pCapabilities->GetAudioCodec()) == NULL))
		return 1;
	return pCodecInfo->_samplingRate;
}
