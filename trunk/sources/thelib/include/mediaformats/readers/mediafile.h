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

#ifndef _MEDIAFILE_H
#define	_MEDIAFILE_H

#include "common.h"

//#define MediaFile File

#ifdef HAS_MMAP
#define MediaFile MmapFile
#else
#define MediaFile File
#endif /* HAS_MMAP */

MediaFile* GetFile(string filePath, uint32_t windowSize);
void ReleaseFile(MediaFile *pFile);
bool GetFile(string filePath, uint32_t windowSize, MediaFile &mediaFile);

#endif	/* _MEDIAFILE_H */
