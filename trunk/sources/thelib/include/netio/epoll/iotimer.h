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
#ifndef _IOTIMER_H
#define	_IOTIMER_H

#include "netio/epoll/iohandler.h"

class IOTimer
: public IOHandler {
private:
#ifndef HAS_EPOLL_TIMERS
	static int32_t _idGenerator;
#else /*  */
	uint64_t _count;
#endif /* HAS_EPOLL_TIMERS */
public:
	IOTimer();
	virtual ~IOTimer();

	virtual bool SignalOutputData();
	virtual bool OnEvent(struct epoll_event &eventWrapper);
	bool EnqueueForTimeEvent(uint32_t seconds);
	bool EnqueueForHighGranularityTimeEvent(uint32_t milliseconds);
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
};

#endif	/* _TIMERIO_H */
#endif /* NET_EPOLL */


