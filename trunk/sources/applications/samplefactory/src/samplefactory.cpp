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

#include "application/baseclientapplication.h"
#include "samplefactory.h"
#include "samplefactoryapplication.h"
#include "protocolfactory.h"
using namespace app_samplefactory;

extern "C" BaseClientApplication *GetApplication_samplefactory(Variant configuration) {
	return new SampleFactoryApplication(
			configuration);
}

extern "C" DLLEXP BaseProtocolFactory *GetFactory_samplefactory(Variant configuration) {
	return new ProtocolFactory();
}

