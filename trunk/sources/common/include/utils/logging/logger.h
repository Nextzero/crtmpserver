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


#ifndef _LOGGER_H
#define _LOGGER_H

#include "common.h"
#ifdef HAS_SAFE_LOGGER
#include <pthread.h>
#endif /* HAS_SAFE_LOGGER */

class BaseLogLocation;

class DLLEXP Version {
public:
	static string GetBuildNumber();
	static uint64_t GetBuildDate();
	static string GetBuildDateString();
	static string GetReleaseNumber();
	static string GetCodeName();
	static string GetBuilderOSName();
	static string GetBuilderOSVersion();
	static string GetBuilderOSArch();
	static string GetBuilderOSUname();
	static string GetBuilderOS();
	static string GetBanner();
	static Variant GetAll();
	static Variant GetBuilder();
};

class DLLEXP Logger {
private:
	static Logger *_pLogger; //! Pointer to the Logger class.
	vector<BaseLogLocation *> _logLocations; //! Vector that stores the location of the log file.
	bool _freeAppenders; //! Boolean that releases the logger.
#ifdef HAS_SAFE_LOGGER
public:
	static pthread_mutex_t *_pMutex;
#endif
public:
	Logger();
	virtual ~Logger();

	static void Init();
	static void Free(bool freeAppenders);
	static void Log(int32_t level, const char *pFileName, uint32_t lineNumber,
			const char *pFunctionName, const char *pFormatString, ...);
	static bool AddLogLocation(BaseLogLocation *pLogLocation);
	static void SignalFork();
	static void SetLevel(int32_t level);
};

#endif /* _LOGGER_H */

