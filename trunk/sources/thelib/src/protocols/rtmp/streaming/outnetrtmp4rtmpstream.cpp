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
#include "protocols/rtmp/streaming/outnetrtmp4rtmpstream.h"
#include "streaming/streamstypes.h"

OutNetRTMP4RTMPStream::OutNetRTMP4RTMPStream(BaseProtocol *pProtocol, string name,
		uint32_t rtmpStreamId, uint32_t chunkSize)
: BaseOutNetRTMPStream(pProtocol, ST_OUT_NET_RTMP_4_RTMP,
name, rtmpStreamId, chunkSize) {

}

OutNetRTMP4RTMPStream::~OutNetRTMP4RTMPStream() {
}

bool OutNetRTMP4RTMPStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_IN_NET_RTMP)
			|| TAG_KIND_OF(type, ST_IN_NET_LIVEFLV)
			|| TAG_KIND_OF(type, ST_IN_FILE_RTMP);
}

bool OutNetRTMP4RTMPStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	return BaseOutNetRTMPStream::InternalFeedData(pData, dataLength, processedLength,
			totalLength, dts, isAudio);
}

bool OutNetRTMP4RTMPStream::PushVideoData(IOBuffer &buffer, double pts, double dts,
		bool isKeyFrame) {
	ASSERT("Operation not supported");
	return false;
}

bool OutNetRTMP4RTMPStream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	ASSERT("Operation not supported");
	return false;
}

bool OutNetRTMP4RTMPStream::IsCodecSupported(uint64_t codec) {
	ASSERT("Operation not supported");
	return false;
}
#endif /* HAS_PROTOCOL_RTMP */

