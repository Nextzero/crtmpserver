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
#include "protocols/rtmp/streaming/rtmpstream.h"
#include "streaming/streamstypes.h"

RTMPStream::RTMPStream(BaseProtocol *pProtocol, uint32_t rtmpStreamId)
: BaseStream(pProtocol, ST_NEUTRAL_RTMP, "") {
	_rtmpStreamId = rtmpStreamId;
	_clientSideBufer = 0;
}

RTMPStream::~RTMPStream() {
}

StreamCapabilities * RTMPStream::GetCapabilities() {
	return NULL;
}

void RTMPStream::SetClientSideBuffer(uint32_t value) {
	_clientSideBufer = value;
}

uint32_t RTMPStream::GetClientSideBuffer() {
	return _clientSideBufer;
}

bool RTMPStream::Play(double dts, double length) {
	ASSERT("Operation not supported");
	return false;
}

bool RTMPStream::Pause() {
	ASSERT("Operation not supported");
	return false;
}

bool RTMPStream::Resume() {
	ASSERT("Operation not supported");
	return false;
}

bool RTMPStream::Seek(double dts) {
	ASSERT("Operation not supported");
	return false;
}

bool RTMPStream::Stop() {
	ASSERT("Operation not supported");
	return false;
}

bool RTMPStream::SignalPlay(double &dts, double &length) {
	ASSERT("Operation not supported");
	return false;
}

bool RTMPStream::SignalPause() {
	ASSERT("Operation not supported");
	return false;
}

bool RTMPStream::SignalResume() {
	ASSERT("Operation not supported");
	return false;
}

bool RTMPStream::SignalSeek(double &dts) {
	ASSERT("Operation not supported");
	return false;
}

bool RTMPStream::SignalStop() {
	ASSERT("Operation not supported");
	return false;
}

bool RTMPStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	ASSERT("Operation not supported");
	return false;
}

bool RTMPStream::IsCompatibleWithType(uint64_t type) {
	return false;
}

void RTMPStream::ReadyForSend() {
	ASSERT("Operation not supported");
}


#endif /* HAS_PROTOCOL_RTMP */

