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
#include "protocols/rtmp/streaming/outfilertmpflvstream.h"
#include "application/baseclientapplication.h"
#include "streaming/streamstypes.h"
#include "protocols/baseprotocol.h"

OutFileRTMPFLVStream::OutFileRTMPFLVStream(BaseProtocol *pProtocol, string name,
		string filename)
: BaseOutFileStream(pProtocol, ST_OUT_FILE_RTMP_FLV, name) {
	_timeBase = -1;
	_prevTagSize = 0;
	_filename = filename;
}

OutFileRTMPFLVStream::~OutFileRTMPFLVStream() {
	if (_file.IsOpen()) {
		_file.Close();
	}
}

void OutFileRTMPFLVStream::Initialize() {
	if (!_file.Initialize(_filename, FILE_OPEN_MODE_TRUNCATE)) {
		FATAL("Unable to initialize file %s", STR(_filename));
		_pProtocol->EnqueueForDelete();
	}

	//REFERENCE: video_file_format_spec_v10.pdf page 8/48

	//2. Write FLV header
	string flv = "FLV";
	if (!_file.WriteString(flv)) {
		FATAL("Unable to write FLV signature");
		_pProtocol->EnqueueForDelete();
		return;
	}

	//3. Write FLV version
	if (!_file.WriteUI8(1)) {
		FATAL("Unable to write FLV version");
		_pProtocol->EnqueueForDelete();
		return;
	}

	//4. Write FLV flags
	if (!_file.WriteUI8(5)) {
		FATAL("Unable to write flags");
		_pProtocol->EnqueueForDelete();
		return;
	}

	//5. Write FLV offset
	if (!_file.WriteUI32(9)) {
		FATAL("Unable to write data offset");
		_pProtocol->EnqueueForDelete();
		return;
	}

	//6. Write first dummy audio
	if (!FeedData(NULL, 0, 0, 0, 0, 0, true)) {
		FATAL("Unable to write dummy audio packet");
		_pProtocol->EnqueueForDelete();
		return;
	}

	//7. Write first dummy video
	if (!FeedData(NULL, 0, 0, 0, 0, 0, false)) {
		FATAL("Unable to write dummy audio packet");
		_pProtocol->EnqueueForDelete();
		return;
	}

	//8. Set the timebase to unknown value
	_timeBase = -1;
	return;
}

bool OutFileRTMPFLVStream::SignalPlay(double &dts, double &length) {
	NYIR;
}

bool OutFileRTMPFLVStream::SignalPause() {
	NYIR;
}

bool OutFileRTMPFLVStream::SignalResume() {
	NYIR;
}

bool OutFileRTMPFLVStream::SignalSeek(double &dts) {
	NYIR;
}

bool OutFileRTMPFLVStream::SignalStop() {
	NYIR;
}

bool OutFileRTMPFLVStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	if (!_file.IsOpen()) {
		Initialize();
	}

	if (_timeBase < 0)
		_timeBase = dts;

	IOBuffer &buffer = isAudio ? _audioBuffer : _videoBuffer;

	if (!buffer.ReadFromBuffer(pData, dataLength)) {
		FATAL("Unable to save data");
		return false;
	}

	if (GETAVAILABLEBYTESCOUNT(buffer) > totalLength) {
		FATAL("Invalid video input");
		return false;
	}

	if (GETAVAILABLEBYTESCOUNT(buffer) < totalLength) {
		return true;
	}

	if (!_file.WriteUI32(_prevTagSize)) {
		FATAL("Unable to write prev tag size");
		return false;
	}

	if (!_file.WriteUI8(isAudio ? 8 : 9)) {
		FATAL("Unable to write marker");
		return false;
	}

	if (!_file.WriteUI24(totalLength)) {
		FATAL("Unable to write data size");
		return false;
	}

	if (!_file.WriteSUI32((uint32_t) dts - (uint32_t) _timeBase)) {
		FATAL("Unable to timestamp");
		return false;
	}

	if (!_file.WriteUI24(0)) {
		FATAL("Unable to write streamId");
		return false;
	}

	if (!_file.WriteBuffer(GETIBPOINTER(buffer),
			GETAVAILABLEBYTESCOUNT(buffer))) {
		FATAL("Unable to write packet data");
		return false;
	}

	_prevTagSize = GETAVAILABLEBYTESCOUNT(buffer) + 11;

	buffer.IgnoreAll();

	return true;
}

bool OutFileRTMPFLVStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_IN_NET_RTMP);
}

void OutFileRTMPFLVStream::SignalAttachedToInStream() {

}

void OutFileRTMPFLVStream::SignalDetachedFromInStream() {
	_file.Close();
}

void OutFileRTMPFLVStream::SignalStreamCompleted() {
}

void OutFileRTMPFLVStream::SignalAudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	if ((pOld != NULL)&&(pNew != NULL))
		EnqueueForDelete();
}

void OutFileRTMPFLVStream::SignalVideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	if ((pOld != NULL)&&(pNew != NULL))
		EnqueueForDelete();
}

bool OutFileRTMPFLVStream::PushVideoData(IOBuffer &buffer, double pts, double dts,
		bool isKeyFrame) {
	ASSERT("Operation not supported");
	return false;
}

bool OutFileRTMPFLVStream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	ASSERT("Operation not supported");
	return false;
}

bool OutFileRTMPFLVStream::IsCodecSupported(uint64_t codec) {
	ASSERT("Operation not supported");
	return false;
}

#endif /* HAS_PROTOCOL_RTMP */

