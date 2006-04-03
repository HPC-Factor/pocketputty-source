#include "wcetime.h"
#include "windows.h"
#include "puttymem.h"

time_t time(time_t *timer) {
    return (0); // Not supported
}

struct tm *localtime(const time_t *timer) {
    SYSTEMTIME time;
    struct tm *result = snew(struct tm);
    GetLocalTime(&time);
    result->tm_hour = time.wHour;
    result->tm_min = time.wMinute;
    result->tm_sec = time.wSecond;
    result->tm_year = time.wYear;
    result->tm_mon = time.wMonth;
    result->tm_mday = time.wDay;
    result->tm_wday = time.wDayOfWeek;
    result->tm_isdst = -1;
    result->tm_yday = -1; // Not supported
    
    return (NULL);
};

size_t strftime(char *buffer, size_t size, const char *format, const struct tm *time) {
    size_t pos = 0;
    char num[16] = "";

    buffer[0] = '\0';
    while (format[pos]) {
	if (format[pos] == '%' && format[pos + 1]) {
	    switch (format[pos + 1]) {

	    case 'a':
	    case 'A':
	    case 'b':
	    case 'B':
	    case 'c':
	    case 'j':
	    case 'U':
	    case 'W':
	    case 'x':
	    case 'X':
	    case 'z':
	    case 'Z':
		num[0] = '\0';
		// Unsupported
		break;

	    case '%d':
		_itoa(time->tm_mday, num, 10);
		break;

	    case '%H':
		_itoa(time->tm_hour, num, 10);
		break;

	    case '%I':
		_itoa((time->tm_hour % 12) + 1, num, 10);
		break;

	    case '%m':
		_itoa(time->tm_mon, num, 10);
		break;

	    case '%M':
		_itoa(time->tm_min, num, 10);
		break;

	    case '%p':
		strcpy(num, ((time->tm_hour / 12) ? "A.M." : "P.M."));
		break;

	    case '%S':
		_itoa(time->tm_sec, num, 10);
		break;

	    case '%w':
		_itoa(time->tm_wday, num, 10);
		break;

	    case '%y':
		_itoa(time->tm_year % 100, num, 10);
		break;

	    case '%Y':
		_itoa(time->tm_year, num, 10);
		break;

	    case '%%':
		strcpy(num, "%");
		break;

	    default:
		num[0] = format[pos + 1];
		num[1] = '\0';
	    }

	    if (num)
		strncat(buffer, num, size);
	
	    pos++;

	} else {
	    size_t len = strlen(buffer);
	    if (len + 1 < size) {
	    	buffer[len++] = format[pos];
		buffer[len] = '\0';
	    } else 
		return (strlen(buffer) + 1);
	}
	pos++;
    }
    return (strlen(buffer) + 1);

}