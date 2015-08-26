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

#ifdef NET_KQUEUE
#include "netio/kqueue/iohandlermanager.h"
#include "netio/kqueue/iohandler.h"

#define INITIAL_EVENTS_SIZE 1024

vector<IOHandlerManagerToken *> IOHandlerManager::_tokensVector1;
vector<IOHandlerManagerToken *> IOHandlerManager::_tokensVector2;
vector<IOHandlerManagerToken *> *IOHandlerManager::_pAvailableTokens;
vector<IOHandlerManagerToken *> *IOHandlerManager::_pRecycledTokens;
map<uint32_t, IOHandler *> IOHandlerManager::_activeIOHandlers;
map<uint32_t, IOHandler *> IOHandlerManager::_deadIOHandlers;
int32_t IOHandlerManager::_kq = 0;
struct kevent *IOHandlerManager::_pDetectedEvents = NULL;
struct kevent *IOHandlerManager::_pPendingEvents = NULL;
int32_t IOHandlerManager::_pendingEventsCount = 0;
int32_t IOHandlerManager::_eventsSize = 0;
FdStats IOHandlerManager::_fdStats;
#ifndef HAS_KQUEUE_TIMERS
struct timespec IOHandlerManager::_timeout = {1, 0};
TimersManager *IOHandlerManager::_pTimersManager = NULL;
struct kevent IOHandlerManager::_dummy = {0, EVFILT_TIMER, 0, 0, 0, NULL};
#endif /* HAS_KQUEUE_TIMERS */

void IOHandlerManager::SetupToken(IOHandler *pIOHandler) {
	IOHandlerManagerToken *pResult = NULL;
	if (_pAvailableTokens->size() == 0) {
		pResult = new IOHandlerManagerToken();
	} else {
		pResult = (*_pAvailableTokens)[0];
		_pAvailableTokens->erase(_pAvailableTokens->begin());
	}
	pResult->pPayload = pIOHandler;
	pResult->validPayload = true;
	pIOHandler->SetIOHandlerManagerToken(pResult);
}

void IOHandlerManager::FreeToken(IOHandler *pIOHandler) {
	IOHandlerManagerToken *pToken = pIOHandler->GetIOHandlerManagerToken();
	pIOHandler->SetIOHandlerManagerToken(NULL);
	pToken->pPayload = NULL;
	pToken->validPayload = false;
	ADD_VECTOR_END((*_pRecycledTokens), pToken);
}

bool IOHandlerManager::RegisterEvent(int32_t fd, int16_t filter, uint16_t flags,
		uint32_t fflags, uint32_t data, IOHandlerManagerToken *pToken, bool ignoreError) {
	if (_pendingEventsCount >= _eventsSize) {
		ResizeEvents();
	}
	EV_SET(&_pPendingEvents[_pendingEventsCount], fd, filter, flags, fflags,
			data, pToken);
	_pendingEventsCount++;
	return true;
}

map<uint32_t, IOHandler *> & IOHandlerManager::GetActiveHandlers() {
	return _activeIOHandlers;
}

map<uint32_t, IOHandler *> & IOHandlerManager::GetDeadHandlers() {
	return _deadIOHandlers;
}

FdStats &IOHandlerManager::GetStats(bool updateSpeeds) {
#ifdef GLOBALLY_ACCOUNT_BYTES
	if (updateSpeeds)
		_fdStats.UpdateSpeeds();
#endif /* GLOBALLY_ACCOUNT_BYTES */
	return _fdStats;
}

void IOHandlerManager::Initialize() {
	_fdStats.Reset();
	_kq = 0;
	_pAvailableTokens = &_tokensVector1;
	_pRecycledTokens = &_tokensVector2;
#ifndef HAS_KQUEUE_TIMERS
	_pTimersManager = new TimersManager(ProcessTimer);
#endif /* HAS_KQUEUE_TIMERS */
	ResizeEvents();
}

void IOHandlerManager::Start() {
	_kq = kqueue();
	o_assert(_kq > 0);
}

void IOHandlerManager::SignalShutdown() {
	close(_kq);
}

void IOHandlerManager::ShutdownIOHandlers() {

	FOR_MAP(_activeIOHandlers, uint32_t, IOHandler *, i) {
		EnqueueForDelete(MAP_VAL(i));
	}
}

void IOHandlerManager::Shutdown() {
	close(_kq);

	for (uint32_t i = 0; i < _tokensVector1.size(); i++)
		delete _tokensVector1[i];
	_tokensVector1.clear();
	_pAvailableTokens = &_tokensVector1;

	for (uint32_t i = 0; i < _tokensVector2.size(); i++)
		delete _tokensVector2[i];
	_tokensVector2.clear();
	_pRecycledTokens = &_tokensVector2;
#ifndef HAS_KQUEUE_TIMERS
	delete _pTimersManager;
	_pTimersManager = NULL;
#endif /* HAS_KQUEUE_TIMERS */

	free(_pPendingEvents);
	_pPendingEvents = NULL;
	free(_pDetectedEvents);
	_pDetectedEvents = NULL;

	_pendingEventsCount = 0;
	_eventsSize = 0;

	if (_activeIOHandlers.size() != 0 || _deadIOHandlers.size() != 0) {
		FATAL("Incomplete shutdown!");
	}
}

void IOHandlerManager::RegisterIOHandler(IOHandler* pIOHandler) {
	if (MAP_HAS1(_activeIOHandlers, pIOHandler->GetId())) {
		ASSERT("IOHandler already registered");
	}
	size_t before = _activeIOHandlers.size();
	_activeIOHandlers[pIOHandler->GetId()] = pIOHandler;
	SetupToken(pIOHandler);
	_fdStats.RegisterManaged(pIOHandler->GetType());
	DEBUG("Handlers count changed: %"PRIz"u->%"PRIz"u %s", before, before + 1,
			STR(IOHandler::IOHTToString(pIOHandler->GetType())));
}

void IOHandlerManager::UnRegisterIOHandler(IOHandler *pIOHandler) {
	if (MAP_HAS1(_activeIOHandlers, pIOHandler->GetId())) {
		_fdStats.UnRegisterManaged(pIOHandler->GetType());
		FreeToken(pIOHandler);
		size_t before = _activeIOHandlers.size();
		_activeIOHandlers.erase(pIOHandler->GetId());
		DEBUG("Handlers count changed: %"PRIz"u->%"PRIz"u %s", before, before - 1,
				STR(IOHandler::IOHTToString(pIOHandler->GetType())));
	}
}

int IOHandlerManager::CreateRawUDPSocket() {
	int result = socket(AF_INET, SOCK_DGRAM, 0);
	if ((result >= 0)&&(setFdCloseOnExec(result))) {
		_fdStats.RegisterRawUdp();
	} else {
		int err = errno;
		FATAL("Unable to create raw udp socket. Error code was: (%d) %s",
				err, strerror(err));
	}
	return result;
}

void IOHandlerManager::CloseRawUDPSocket(int socket) {
	if (socket > 0) {
		_fdStats.UnRegisterRawUdp();
	}
	CLOSE_SOCKET(socket);
}
#ifdef GLOBALLY_ACCOUNT_BYTES

void IOHandlerManager::AddInBytesManaged(IOHandlerType type, uint64_t bytes) {
	_fdStats.AddInBytesManaged(type, bytes);
}

void IOHandlerManager::AddOutBytesManaged(IOHandlerType type, uint64_t bytes) {
	_fdStats.AddOutBytesManaged(type, bytes);
}

void IOHandlerManager::AddInBytesRawUdp(uint64_t bytes) {
	_fdStats.AddInBytesRawUdp(bytes);
}

void IOHandlerManager::AddOutBytesRawUdp(uint64_t bytes) {
	_fdStats.AddOutBytesRawUdp(bytes);
}
#endif /* GLOBALLY_ACCOUNT_BYTES */

bool IOHandlerManager::EnableReadData(IOHandler *pIOHandler) {
	return RegisterEvent(pIOHandler->GetInboundFd(), EVFILT_READ,
			EV_ADD | EV_ENABLE, 0, 0,
			pIOHandler->GetIOHandlerManagerToken());
}

bool IOHandlerManager::DisableReadData(IOHandler *pIOHandler, bool ignoreError) {
	return RegisterEvent(pIOHandler->GetInboundFd(), EVFILT_READ,
			EV_ADD | EV_DISABLE, 0, 0,
			pIOHandler->GetIOHandlerManagerToken(), ignoreError);
}

bool IOHandlerManager::EnableWriteData(IOHandler *pIOHandler) {
	return RegisterEvent(pIOHandler->GetOutboundFd(), EVFILT_WRITE,
			EV_ADD | EV_ENABLE, 0, 0,
			pIOHandler->GetIOHandlerManagerToken());
}

bool IOHandlerManager::DisableWriteData(IOHandler *pIOHandler, bool ignoreError) {
	return RegisterEvent(pIOHandler->GetOutboundFd(), EVFILT_WRITE,
			EV_ADD | EV_DISABLE, 0, 0,
			pIOHandler->GetIOHandlerManagerToken(), ignoreError);
}

bool IOHandlerManager::EnableAcceptConnections(IOHandler *pIOHandler) {
	return RegisterEvent(pIOHandler->GetInboundFd(), EVFILT_READ,
			EV_ADD | EV_ENABLE, 0, 0,
			pIOHandler->GetIOHandlerManagerToken());
}

bool IOHandlerManager::DisableAcceptConnections(IOHandler *pIOHandler, bool ignoreError) {
	return RegisterEvent(pIOHandler->GetInboundFd(), EVFILT_READ,
			EV_ADD | EV_DISABLE, 0, 0,
			pIOHandler->GetIOHandlerManagerToken(), ignoreError);
}

bool IOHandlerManager::EnableTimer(IOHandler *pIOHandler, uint32_t seconds) {
#ifdef HAS_KQUEUE_TIMERS
	return RegisterEvent(pIOHandler->GetId(), EVFILT_TIMER,
			EV_ADD | EV_ENABLE, NOTE_USECONDS,
			seconds*KQUEUE_TIMER_MULTIPLIER, pIOHandler->GetIOHandlerManagerToken());
#else /* HAS_KQUEUE_TIMERS */
	TimerEvent event = {0, 0, 0, 0};
	event.id = pIOHandler->GetId();
	event.period = seconds * 1000;
	event.pUserData = pIOHandler->GetIOHandlerManagerToken();
	_pTimersManager->AddTimer(event);
	return true;
#endif /* HAS_KQUEUE_TIMERS */
}

bool IOHandlerManager::EnableHighGranularityTimer(IOHandler *pIOHandler, uint32_t milliseconds) {
#ifdef HAS_KQUEUE_TIMERS
	return RegisterEvent(pIOHandler->GetId(), EVFILT_TIMER,
			EV_ADD | EV_ENABLE, NOTE_USECONDS,
			milliseconds * (KQUEUE_TIMER_MULTIPLIER / 1000), pIOHandler->GetIOHandlerManagerToken());
#else /* HAS_KQUEUE_TIMERS */
	TimerEvent event = {0, 0, 0, 0};
	event.id = pIOHandler->GetId();
	event.period = milliseconds;
	event.pUserData = pIOHandler->GetIOHandlerManagerToken();
	_pTimersManager->AddTimer(event);
	return true;
#endif /* HAS_KQUEUE_TIMERS */
}

bool IOHandlerManager::DisableTimer(IOHandler *pIOHandler, bool ignoreError) {
#ifdef HAS_KQUEUE_TIMERS
	return RegisterEvent(pIOHandler->GetId(), EVFILT_TIMER,
			EV_DELETE, 0, 0,
			pIOHandler->GetIOHandlerManagerToken(), ignoreError);
#else /* HAS_KQUEUE_TIMERS */
	_pTimersManager->RemoveTimer(pIOHandler->GetId());
	return true;
#endif /* HAS_KQUEUE_TIMERS */
}

void IOHandlerManager::EnqueueForDelete(IOHandler *pIOHandler) {
	DisableAcceptConnections(pIOHandler, true);
	DisableReadData(pIOHandler, true);
	DisableWriteData(pIOHandler, true);
	DisableTimer(pIOHandler, true);
	if (!MAP_HAS1(_deadIOHandlers, pIOHandler->GetId()))
		_deadIOHandlers[pIOHandler->GetId()] = pIOHandler;
}

uint32_t IOHandlerManager::DeleteDeadHandlers() {
	uint32_t result = 0;
	while (_deadIOHandlers.size() > 0) {
		IOHandler *pIOHandler = MAP_VAL(_deadIOHandlers.begin());
		_deadIOHandlers.erase(pIOHandler->GetId());
		delete pIOHandler;
		result++;
	}
	return result;
}

bool IOHandlerManager::Pulse() {
	int32_t result = 0;
#ifdef HAS_KQUEUE_TIMERS
	result = kevent(_kq, _pPendingEvents, _pendingEventsCount,
			_pDetectedEvents, _eventsSize, NULL);
#else /* HAS_KQUEUE_TIMERS */
	result = kevent(_kq, _pPendingEvents, _pendingEventsCount,
			_pDetectedEvents, _eventsSize, &_timeout);
	int32_t nextVal = _pTimersManager->TimeElapsed();
	_timeout.tv_sec = nextVal / 1000;
	_timeout.tv_nsec = (nextVal * 1000000) % 1000000000;
#endif /* HAS_KQUEUE_TIMERS */

	if (result < 0) {
		int err = errno;
		if (err == EINTR)
			return true;
		FATAL("kevent failed: (%d) %s", err, strerror(err));
		return false;
	}
	_pendingEventsCount = 0;
	for (int32_t i = 0; i < result; i++) {
		IOHandlerManagerToken *pToken =
				(IOHandlerManagerToken *) _pDetectedEvents[i].udata;
		if ((_pDetectedEvents[i].flags & EV_ERROR) != 0) {
			if (pToken->validPayload) {
				//DEBUG("***Event handler ERROR: %p", (IOHandler *) pToken->pPayload);
				IOHandlerManager::EnqueueForDelete((IOHandler *) pToken->pPayload);
			}
			continue;
		}

		if (pToken->validPayload) {
			if (!((IOHandler *) pToken->pPayload)->OnEvent(_pDetectedEvents[i])) {
				EnqueueForDelete((IOHandler *) pToken->pPayload);
			}
			if ((_pDetectedEvents[i].flags & EV_EOF) != 0) {
				//DEBUG("***Event handler EOF: %p", (IOHandler *) pToken->pPayload);
				IOHandlerManager::EnqueueForDelete((IOHandler *) pToken->pPayload);
			}
		}
		//		else {
		//			FATAL("Invalid token");
		//		}
	}

	if (_tokensVector1.size() > _tokensVector2.size()) {
		_pAvailableTokens = &_tokensVector1;
		_pRecycledTokens = &_tokensVector2;
	} else {
		_pAvailableTokens = &_tokensVector2;
		_pRecycledTokens = &_tokensVector1;
	}

	return true;
}

#ifndef HAS_KQUEUE_TIMERS

bool IOHandlerManager::ProcessTimer(TimerEvent &event) {
	IOHandlerManagerToken *pToken =
			(IOHandlerManagerToken *) event.pUserData;
	if (pToken->validPayload) {
		if (!((IOHandler *) pToken->pPayload)->OnEvent(_dummy)) {
			EnqueueForDelete((IOHandler *) pToken->pPayload);
			return false;
		}
		return true;
	} else {
		FATAL("Invalid token");
		return false;
	}
}
#endif /* HAS_KQUEUE_TIMERS */

inline void IOHandlerManager::ResizeEvents() {
	_eventsSize += INITIAL_EVENTS_SIZE;
	_pPendingEvents = (struct kevent *) realloc(_pPendingEvents,
			_eventsSize * sizeof (struct kevent));
	_pDetectedEvents = (struct kevent *) realloc(_pDetectedEvents,
			_eventsSize * sizeof (struct kevent));
	WARN("Event size resized: %d->%d", _eventsSize - INITIAL_EVENTS_SIZE,
			_eventsSize);
}

#endif /* NET_KQUEUE */


