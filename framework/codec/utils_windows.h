#pragma once

#ifndef WINAPI
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#endif

#include <wingdi.h>

namespace Cicada {
	bool checkDxInteropAvailable();
}// namespace Cicada