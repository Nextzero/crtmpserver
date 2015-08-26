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
#ifndef _OUTNETRTMP4TSSTREAM_H
#define	_OUTNETRTMP4TSSTREAM_H

#include "protocols/rtmp/streaming/baseoutnetrtmpstream.h"

class DLLEXP OutNetRTMP4TSStream
: public BaseOutNetRTMPStream {
public:
	OutNetRTMP4TSStream(BaseProtocol *pProtocol, string name, uint32_t rtmpStreamId,
			uint32_t chunkSize);
	virtual ~OutNetRTMP4TSStream();

	virtual bool IsCompatibleWithType(uint64_t type);
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
protected:
	virtual bool FinishInitialization(
			GenericProcessDataSetup *pGenericProcessDataSetup);
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts,
			bool isKeyFrame);
	virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
	virtual bool IsCodecSupported(uint64_t codec);
};


#endif	/* _OUTNETRTMP4TSSTREAM_H */

#endif /* HAS_PROTOCOL_RTMP */

