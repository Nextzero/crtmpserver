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

#ifndef _ECHOAPPPROTOCOLHANDLER_H
#define	_ECHOAPPPROTOCOLHANDLER_H

#include "application/baseappprotocolhandler.h"

namespace app_samplefactory {

	class EchoAppProtocolHandler
	: public BaseAppProtocolHandler {
	public:
		EchoAppProtocolHandler(Variant &configuration);
		virtual ~EchoAppProtocolHandler();

		virtual void RegisterProtocol(BaseProtocol *pProtocol);
		virtual void UnRegisterProtocol(BaseProtocol *pProtocol);
	};
}

#endif	/* _ECHOAPPPROTOCOLHANDLER_H */

