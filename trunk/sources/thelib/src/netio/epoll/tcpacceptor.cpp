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

#include "protocols/rtmp/amf0serializer.h"



#ifdef NET_EPOLL
#include "netio/epoll/tcpacceptor.h"
#include "netio/epoll/iohandlermanager.h"
#include "protocols/protocolfactorymanager.h"
#include "protocols/tcpprotocol.h"
#include "netio/epoll/tcpcarrier.h"
#include "application/baseclientapplication.h"

TCPAcceptor::TCPAcceptor(string ipAddress, uint16_t port, Variant parameters,
		vector<uint64_t>/*&*/ protocolChain)
: IOHandler(0, 0, IOHT_ACCEPTOR) {
	_pApplication = NULL;
	memset(&_address, 0, sizeof (sockaddr_in));

	_address.sin_family = PF_INET;
	_address.sin_addr.s_addr = inet_addr(STR(ipAddress));
	o_assert(_address.sin_addr.s_addr != INADDR_NONE);
	_address.sin_port = EHTONS(port); //----MARKED-SHORT----

	_protocolChain = protocolChain;
	_parameters = parameters;
	_enabled = false;
	_acceptedCount = 0;
	_droppedCount = 0;
	_ipAddress = ipAddress;
	_port = port;
}

TCPAcceptor::~TCPAcceptor() {
	CLOSE_SOCKET(_inboundFd);
}

bool TCPAcceptor::Bind() {
	_inboundFd = _outboundFd = (int) socket(PF_INET, SOCK_STREAM, 0); //NOINHERIT
	if (_inboundFd < 0) {
		int err = errno;
		FATAL("Unable to create socket: (%d) %s", err, strerror(err));
		return false;
	}

	if (!setFdOptions(_inboundFd, false)) {
		FATAL("Unable to set socket options");
		return false;
	}

	if (bind(_inboundFd, (sockaddr *) & _address, sizeof (sockaddr)) != 0) {
		int err = errno;
		FATAL("Unable to bind on address: tcp://%s:%hu; Error was: (%d) %s",
				inet_ntoa(((sockaddr_in *) & _address)->sin_addr),
				ENTOHS(((sockaddr_in *) & _address)->sin_port),
				err,
				strerror(err));
		return false;
	}

	if (_port == 0) {
		socklen_t tempSize = sizeof (sockaddr);
		if (getsockname(_inboundFd, (sockaddr *) & _address, &tempSize) != 0) {
			FATAL("Unable to extract the random port");
			return false;
		}
		_parameters[CONF_PORT] = (uint16_t) ENTOHS(_address.sin_port);
	}

	if (listen(_inboundFd, 100) != 0) {
		FATAL("Unable to put the socket in listening mode");
		return false;
	}

	_enabled = true;
	return true;
}

void TCPAcceptor::SetApplication(BaseClientApplication *pApplication) {
	o_assert(_pApplication == NULL);
	_pApplication = pApplication;
}

bool TCPAcceptor::StartAccept() {
	return IOHandlerManager::EnableAcceptConnections(this);
}

bool TCPAcceptor::SignalOutputData() {
	ASSERT("Operation not supported");
	return false;
}

bool TCPAcceptor::OnEvent(struct epoll_event &event) {
	//we should not return false here, because the acceptor will simply go down.
	//Instead, after the connection accepting routine failed, check to
	//see if the acceptor socket is stil in the business
	if (!OnConnectionAvailable(event))
		return IsAlive();
	else
		return true;
}

bool TCPAcceptor::OnConnectionAvailable(struct epoll_event &event) {
	if (_pApplication == NULL)
		return Accept();
	return _pApplication->AcceptTCPConnection(this);
}

bool TCPAcceptor::Accept() {
	sockaddr address;
	memset(&address, 0, sizeof (sockaddr));
	socklen_t len = sizeof (sockaddr);
	int32_t fd;

	//1. Accept the connection
	fd = accept(_inboundFd, &address, &len);
	if ((fd < 0) || (!setFdCloseOnExec(fd))) {
		int err = errno;
		FATAL("Unable to accept client connection: (%d) %s", err, strerror(err));
		return false;
	}
	if (!_enabled) {
		CLOSE_SOCKET(fd);
		_droppedCount++;
		WARN("Acceptor is not enabled. Client dropped: %s:%"PRIu16" -> %s:%"PRIu16,
				inet_ntoa(((sockaddr_in *) & address)->sin_addr),
				ENTOHS(((sockaddr_in *) & address)->sin_port),
				STR(_ipAddress),
				_port);
		return true;
	}

	if (!setFdOptions(fd, false)) {
		FATAL("Unable to set socket options");
		CLOSE_SOCKET(fd);
		return false;
	}

	//4. Create the chain
	BaseProtocol *pProtocol = ProtocolFactoryManager::CreateProtocolChain(_protocolChain, _parameters);
	if (pProtocol == NULL) {
		FATAL("Unable to create protocol chain");
		CLOSE_SOCKET(fd);
		return false;
	}

	//5. Create the carrier and bind it
	TCPCarrier *pTCPCarrier = new TCPCarrier(fd);
	pTCPCarrier->SetProtocol(pProtocol->GetFarEndpoint());
	pProtocol->GetFarEndpoint()->SetIOHandler(pTCPCarrier);

	//6. Register the protocol stack with an application
	if (_pApplication != NULL) {
		pProtocol = pProtocol->GetNearEndpoint();
		pProtocol->SetApplication(_pApplication);

		//		EventLogger *pEvtLog = _pApplication->GetEventLogger();
		//		if (pEvtLog != NULL) {
		//			pEvtLog->LogInboundConnectionStart(_ipAddress, _port, STR(*(pProtocol->GetFarEndpoint())));
		//			pTCPCarrier->SetEventLogger(pEvtLog);
		//		}
	}

	if (pProtocol->GetNearEndpoint()->GetOutputBuffer() != NULL)
		pProtocol->GetNearEndpoint()->EnqueueForOutbound();

	_acceptedCount++;

	INFO("Inbound connection accepted: %s", STR(*(pProtocol->GetNearEndpoint())));

	//7. Done
	return true;
}

bool TCPAcceptor::Drop() {
	sockaddr address;
	memset(&address, 0, sizeof (sockaddr));
	socklen_t len = sizeof (sockaddr);


	//1. Accept the connection
	int32_t fd = accept(_inboundFd, &address, &len);
	if ((fd < 0) || (!setFdCloseOnExec(fd))) {
		int err = errno;
		if (err != EWOULDBLOCK)
			WARN("Accept failed. Error code was: (%d) %s", err, strerror(err));
		return false;
	}

	//2. Drop it now
	CLOSE_SOCKET(fd);
	_droppedCount++;

	INFO("Client explicitly dropped: %s:%"PRIu16" -> %s:%"PRIu16,
			inet_ntoa(((sockaddr_in *) & address)->sin_addr),
			ENTOHS(((sockaddr_in *) & address)->sin_port),
			STR(_ipAddress),
			_port);
	return true;
}

Variant & TCPAcceptor::GetParameters() {
	return _parameters;
}

BaseClientApplication *TCPAcceptor::GetApplication() {
	return _pApplication;
}

vector<uint64_t> &TCPAcceptor::GetProtocolChain() {
	return _protocolChain;
}

void TCPAcceptor::GetStats(Variant &info, uint32_t namespaceId) {
	info = _parameters;
	info["id"] = (((uint64_t) namespaceId) << 32) | GetId();
	info["enabled"] = (bool)_enabled;
	info["acceptedConnectionsCount"] = _acceptedCount;
	info["droppedConnectionsCount"] = _droppedCount;
	if (_pApplication != NULL) {
		info["appId"] = (((uint64_t) namespaceId) << 32) | _pApplication->GetId();
		info["appName"] = _pApplication->GetName();
	} else {
		info["appId"] = (((uint64_t) namespaceId) << 32);
		info["appName"] = "";
	}
}

bool TCPAcceptor::Enable() {
	return _enabled;
}

void TCPAcceptor::Enable(bool enabled) {
	_enabled = enabled;
}

bool TCPAcceptor::IsAlive() {
	//TODO: Implement this. It must return true
	//if this acceptor is operational
	NYI;
	return true;
}

#endif /* NET_EPOLL */

