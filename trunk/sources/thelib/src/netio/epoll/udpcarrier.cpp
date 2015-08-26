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


#ifdef NET_EPOLL
#include "netio/epoll/udpcarrier.h"
#include "netio/epoll/iohandlermanager.h"
#include "protocols/baseprotocol.h"

#define SOCKET_READ_CHUNK 65536
#define SOCKET_WRITE_CHUNK SOCKET_READ_CHUNK

UDPCarrier::UDPCarrier(int32_t fd)
: IOHandler(fd, fd, IOHT_UDP_CARRIER) {
	memset(&_peerAddress, 0, sizeof (sockaddr_in));
	memset(&_nearAddress, 0, sizeof (sockaddr_in));
	_nearIp = "";
	_nearPort = 0;
	_rx = 0;
	_tx = 0;
	_ioAmount = 0;

	Variant stats;
	GetStats(stats);
}

UDPCarrier::~UDPCarrier() {
	Variant stats;
	GetStats(stats);
	CLOSE_SOCKET(_inboundFd);
}

bool UDPCarrier::OnEvent(struct epoll_event &event) {
	//1. Read data
	if ((event.events & EPOLLIN) != 0) {
		IOBuffer *pInputBuffer = _pProtocol->GetInputBuffer();
		o_assert(pInputBuffer != NULL);
		if (!pInputBuffer->ReadFromUDPFd(_inboundFd, _ioAmount, _peerAddress)) {
			FATAL("Unable to read data");
			return false;
		}
		if (_ioAmount == 0) {
			FATAL("Connection closed");
			return false;
		}
		_rx += _ioAmount;
		ADD_IN_BYTES_MANAGED(_type, _ioAmount);
		if (!_pProtocol->SignalInputData(_ioAmount, &_peerAddress)) {
			FATAL("Unable to signal data available");
			return false;
		}
	}

	//2. Write data
	if ((event.events & EPOLLOUT) != 0) {
		_pProtocol->ReadyForSend();
		return true;
	}

	return true;
}

bool UDPCarrier::SignalOutputData() {
	NYIR;
}

void UDPCarrier::GetStats(Variant &info, uint32_t namespaceId) {
	if (!GetEndpointsInfo()) {
		FATAL("Unable to get endpoints info");
		info = "unable to get endpoints info";
		return;
	}
	info["type"] = "IOHT_UDP_CARRIER";
	info["nearIP"] = _nearIp;
	info["nearPort"] = _nearPort;
	info["rx"] = _rx;
}

Variant &UDPCarrier::GetParameters() {
	return _parameters;
}

void UDPCarrier::SetParameters(Variant parameters) {
	_parameters = parameters;
}

bool UDPCarrier::StartAccept() {
	return IOHandlerManager::EnableReadData(this);
}

string UDPCarrier::GetFarEndpointAddress() {
	ASSERT("Operation not supported");
	return "";
}

uint16_t UDPCarrier::GetFarEndpointPort() {
	ASSERT("Operation not supported");
	return 0;
}

string UDPCarrier::GetNearEndpointAddress() {
	if (_nearIp != "")
		return _nearIp;
	GetEndpointsInfo();
	return _nearIp;
}

uint16_t UDPCarrier::GetNearEndpointPort() {
	if (_nearPort != 0)
		return _nearPort;
	GetEndpointsInfo();
	return _nearPort;
}

UDPCarrier* UDPCarrier::Create(string bindIp, uint16_t bindPort, uint16_t ttl,
		uint16_t tos, string ssmIp) {

	//1. Create the socket
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if ((sock < 0) || (!setFdCloseOnExec(sock))) {
		int err = errno;
		FATAL("Unable to create socket: (%d) %s", err, strerror(err));
		return NULL;
	}

	//2. fd options
	if (!setFdOptions(sock, true)) {
		FATAL("Unable to set fd options");
		CLOSE_SOCKET(sock);
		return NULL;
	}

	if (tos <= 255) {
		if (!setFdTOS(sock, (uint8_t) tos)) {
			FATAL("Unable to set tos");
			CLOSE_SOCKET(sock);
			return NULL;
		}
	}

	//3. Bind
	if (bindIp == "") {
		bindIp = "0.0.0.0";
		bindPort = 0;
	}

	sockaddr_in bindAddress;
	memset(&bindAddress, 0, sizeof (bindAddress));
	bindAddress.sin_family = PF_INET;
	bindAddress.sin_addr.s_addr = inet_addr(STR(bindIp));
	bindAddress.sin_port = EHTONS(bindPort); //----MARKED-SHORT----
	if (bindAddress.sin_addr.s_addr == INADDR_NONE) {
		FATAL("Unable to bind on address %s:%hu", STR(bindIp), bindPort);
		CLOSE_SOCKET(sock);
		return NULL;
	}
	uint32_t testVal = EHTONL(bindAddress.sin_addr.s_addr);
	if ((testVal > 0xe0000000) && (testVal < 0xefffffff)) {
		INFO("Subscribe to multicast %s:%"PRIu16, STR(bindIp), bindPort);
		int activateBroadcast = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &activateBroadcast,
				sizeof (activateBroadcast)) != 0) {
			int err = errno;
			FATAL("Unable to activate SO_BROADCAST on the socket: (%d) %s", err, strerror(err));
			return NULL;
		}
		if (ttl <= 255) {
			if (!setFdMulticastTTL(sock, (uint8_t) ttl)) {
				FATAL("Unable to set ttl");
				CLOSE_SOCKET(sock);
				return NULL;
			}
		}
	} else {
		if (ttl <= 255) {
			if (!setFdTTL(sock, (uint8_t) ttl)) {
				FATAL("Unable to set ttl");
				CLOSE_SOCKET(sock);
				return NULL;
			}
		}
	}
	if (bind(sock, (sockaddr *) & bindAddress, sizeof (sockaddr)) != 0) {
		int err = errno;
		FATAL("Unable to bind on address: udp://%s:%"PRIu16"; Error was: (%d) %s",
				STR(bindIp), bindPort, err, strerror(err));
		CLOSE_SOCKET(sock);
		return NULL;
	}
	if ((testVal > 0xe0000000) && (testVal < 0xefffffff)) {
		if (!setFdJoinMulticast(sock, bindIp, bindPort, ssmIp)) {
			FATAL("Adding multicast failed");
			CLOSE_SOCKET(sock);
			return NULL;
		}
	}


	//4. Create the carrier
	UDPCarrier *pResult = new UDPCarrier(sock);
	pResult->_nearAddress = bindAddress;

	return pResult;
}

UDPCarrier* UDPCarrier::Create(string bindIp, uint16_t bindPort,
		BaseProtocol *pProtocol, uint16_t ttl, uint16_t tos, string ssmIp) {
	if (pProtocol == NULL) {
		FATAL("Protocol can't be null");
		return NULL;
	}

	UDPCarrier *pResult = Create(bindIp, bindPort, ttl, tos, ssmIp);
	if (pResult == NULL) {
		FATAL("Unable to create UDP carrier");
		return NULL;
	}

	pResult->SetProtocol(pProtocol->GetFarEndpoint());
	pProtocol->GetFarEndpoint()->SetIOHandler(pResult);

	return pResult;
}

bool UDPCarrier::Setup(Variant &settings) {
	NYIR;
}

bool UDPCarrier::GetEndpointsInfo() {
	socklen_t len = sizeof (sockaddr);
	if (getsockname(_inboundFd, (sockaddr *) & _nearAddress, &len) != 0) {
		FATAL("Unable to get peer's address");
		return false;
	}
	_nearIp = format("%s", inet_ntoa(((sockaddr_in *) & _nearAddress)->sin_addr));
	_nearPort = ENTOHS(((sockaddr_in *) & _nearAddress)->sin_port); //----MARKED-SHORT----
	return true;
}

#endif /* NET_EPOLL */


