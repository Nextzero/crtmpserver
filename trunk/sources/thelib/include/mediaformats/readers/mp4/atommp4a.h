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
#ifndef _ATOMMP4A_H
#define _ATOMMP4A_H

class AtomESDS;
class AtomWAVE;
class AtomCHAN;

#include "mediaformats/readers/mp4/boxatom.h"

class AtomMP4A
: public BoxAtom {
protected:
	AtomESDS *_pESDS;
	AtomWAVE *_pWAVE;
	AtomCHAN *_pCHAN;
public:
	AtomMP4A(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~AtomMP4A();
protected:
	virtual bool Read();
	virtual bool AtomCreated(BaseAtom *pAtom);
};

#endif	/* _ATOMMP4A_H */


#endif /* HAS_MEDIA_MP4 */
