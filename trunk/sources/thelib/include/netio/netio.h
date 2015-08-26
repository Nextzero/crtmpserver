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


#ifndef _NETIO_H
#define	_NETIO_H

#include "netio/iohandlertype.h"
#include "netio/fdstats.h"

#ifdef NET_KQUEUE
#include "netio/kqueue/iohandler.h"
#include "netio/kqueue/iohandlermanager.h"
#include "netio/kqueue/iohandlermanagertoken.h"
#include "netio/kqueue/iotimer.h"
#include "netio/kqueue/tcpacceptor.h"
#include "netio/kqueue/tcpcarrier.h"
#include "netio/kqueue/udpcarrier.h"
#include "netio/kqueue/tcpconnector.h"
#ifdef HAS_KQUEUE_TIMERS
#define NETWORK_REACTOR "kqueue with EVFILT_TIMER support"
#else /* HAS_KQUEUE_TIMERS */
#define NETWORK_REACTOR "kqueue without EVFILT_TIMER support"
#endif /* HAS_KQUEUE_TIMERS */
#endif

#ifdef NET_EPOLL
#include "netio/epoll/iohandler.h"
#include "netio/epoll/iohandlermanager.h"
#include "netio/epoll/iohandlermanagertoken.h"
#include "netio/epoll/iotimer.h"
#include "netio/epoll/tcpacceptor.h"
#include "netio/epoll/tcpcarrier.h"
#include "netio/epoll/udpcarrier.h"
#include "netio/epoll/tcpconnector.h"
#ifdef HAS_EPOLL_TIMERS
#define NETWORK_REACTOR "epoll with timerfd_XXXX support"
#else /* HAS_EPOLL_TIMERS */
#define NETWORK_REACTOR "epoll without timerfd_XXXX support"
#endif /* HAS_EPOLL_TIMERS */
#endif

#ifdef NET_SELECT
#include "netio/select/iohandler.h"
#include "netio/select/iohandlermanager.h"
#include "netio/select/iotimer.h"
#include "netio/select/tcpacceptor.h"
#include "netio/select/tcpcarrier.h"
#include "netio/select/udpcarrier.h"
#include "netio/select/tcpconnector.h"
#define NETWORK_REACTOR "select"
#endif

#endif	/* _NETIO_H */


