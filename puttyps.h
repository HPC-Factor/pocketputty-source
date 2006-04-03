#ifndef PUTTY_PUTTYPS_H
#define PUTTY_PUTTYPS_H

#if defined(_WIN32_WCE) || defined(UNDER_CE)

#include "wcestuff.h"

#elif defined (_WINDOWS)

//#include "winstuff.h"

#elif defined(macintosh)

//#include "mac\macstuff.h"

#else

//#include "unix\unix.h"

#endif

#endif
