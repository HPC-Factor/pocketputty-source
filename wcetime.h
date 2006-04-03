#ifndef WCETIME_H
#define WCETIME_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#if (UNDER_CE >= 400)
#include <time.h>
#endif


#if (UNDER_CE < 400)
    typedef struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
    };
    
    time_t time(time_t *timer);
#endif

struct tm *localtime(const time_t *timer);
size_t strftime(char *, size_t, const char *, const struct tm *);

#ifdef __cplusplus
}
#endif
#endif