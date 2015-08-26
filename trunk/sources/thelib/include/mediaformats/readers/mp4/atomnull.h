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

#ifdef HAS_MEDIA_MP4
#ifndef _ATOMNULL_H
#define _ATOMNULL_H

#include "mediaformats/readers/mp4/baseatom.h"

class AtomNULL
: public BaseAtom {
public:
	AtomNULL(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~AtomNULL();

	virtual bool IsIgnored();

	virtual bool Read();
	string Hierarchy(uint32_t indent);
};

#endif	/* _ATOMNULL_H */


#endif /* HAS_MEDIA_MP4 */
