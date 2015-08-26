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

#ifdef HAS_PROTOCOL_RTP
#ifndef _OUTBOUNDCONNECTIVITY_H
#define	_OUTBOUNDCONNECTIVITY_H

#include "protocols/rtp/connectivity/baseconnectivity.h"

class BaseOutNetRTPUDPStream;
class RTSPProtocol;

struct RTPClient {
	uint32_t protocolId;
	bool isUdp;

	bool hasAudio;
	sockaddr_in audioDataAddress;
	sockaddr_in audioRtcpAddress;
	uint32_t audioPacketsCount;
	uint32_t audioBytesCount;
	uint8_t audioDataChannel;
	uint8_t audioRtcpChannel;

	bool hasVideo;
	sockaddr_in videoDataAddress;
	sockaddr_in videoRtcpAddress;
	uint32_t videoPacketsCount;
	uint32_t videoBytesCount;
	uint8_t videoDataChannel;
	uint8_t videoRtcpChannel;

	RTPClient() {
		protocolId = 0;
		isUdp = false;

		hasAudio = false;
		memset(&audioDataAddress, 0, sizeof (audioDataAddress));
		memset(&audioRtcpAddress, 0, sizeof (audioRtcpAddress));
		audioPacketsCount = 0;
		audioBytesCount = 0;
		audioDataChannel = 0xff;
		audioRtcpChannel = 0xff;

		hasVideo = false;
		memset(&videoDataAddress, 0, sizeof (videoDataAddress));
		memset(&videoRtcpAddress, 0, sizeof (videoRtcpAddress));
		videoPacketsCount = 0;
		videoBytesCount = 0;
		videoDataChannel = 0xff;
		videoRtcpChannel = 0xff;
	}
};

class DLLEXP OutboundConnectivity
: public BaseConnectivity {
private:
	bool _forceTcp;
	RTSPProtocol *_pRTSPProtocol;
	BaseOutNetRTPUDPStream *_pOutStream;
	MSGHDR _dataMessage;
	MSGHDR _rtcpMessage;
	uint8_t *_pRTCPNTP;
	uint8_t *_pRTCPRTP;
	uint8_t *_pRTCPSPC;
	uint8_t *_pRTCPSOC;
	uint64_t _startupTime;
	RTPClient _rtpClient;

	bool _hasVideo;
	SOCKET _videoDataFd;
	uint16_t _videoDataPort;
	SOCKET _videoRTCPFd;
	uint16_t _videoRTCPPort;
	uint32_t _videoNATDataId;
	uint32_t _videoNATRTCPId;
	double _videoSampleRate;

	bool _hasAudio;
	SOCKET _audioDataFd;
	uint16_t _audioDataPort;
	SOCKET _audioRTCPFd;
	uint16_t _audioRTCPPort;
	uint32_t _audioNATDataId;
	uint32_t _audioNATRTCPId;
	double _audioSampleRate;

	int32_t _amountSent;
	uint32_t _dummyValue;
public:
	OutboundConnectivity(bool forceTcp, RTSPProtocol *pRTSPProtocol);
	virtual ~OutboundConnectivity();
	bool Initialize();
	void Enable(bool value);
	void SetOutStream(BaseOutNetRTPUDPStream *pOutStream);
	string GetVideoPorts();
	string GetAudioPorts();
	string GetVideoChannels();
	string GetAudioChannels();
	uint32_t GetAudioSSRC();
	uint32_t GetVideoSSRC();
	uint16_t GetLastVideoSequence();
	uint16_t GetLastAudioSequence();
	void HasAudio(bool value);
	void HasVideo(bool value);
	bool RegisterUDPVideoClient(uint32_t rtspProtocolId, sockaddr_in &data,
			sockaddr_in &rtcp);
	bool RegisterUDPAudioClient(uint32_t rtspProtocolId, sockaddr_in &data,
			sockaddr_in &rtcp);
	bool RegisterTCPVideoClient(uint32_t rtspProtocolId, uint8_t data, uint8_t rtcp);
	bool RegisterTCPAudioClient(uint32_t rtspProtocolId, uint8_t data, uint8_t rtcp);
	void SignalDetachedFromInStream();
	bool FeedVideoData(MSGHDR &message, double pts, double dts);
	bool FeedAudioData(MSGHDR &message, double pts, double dts);
	void ReadyForSend();
private:
	bool InitializePorts(SOCKET &dataFd, uint16_t &dataPort,
			uint32_t &natDataId, SOCKET &RTCPFd, uint16_t &RTCPPort,
			uint32_t &natRTCPId);
	bool FeedData(MSGHDR &message, double pts, double dts, bool isAudio);
};


#endif	/* _OUTBOUNDCONNECTIVITY_H */
#endif /* HAS_PROTOCOL_RTP */

