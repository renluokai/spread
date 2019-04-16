#include <stdio.h>
#include <time.h>
#include "../include/helper.h"
void Time::FullTime(char*  buffer)
{
	struct timespec tv;
	struct tm ts;
	clock_gettime(CLOCK_REALTIME,&tv);
	localtime_r(&tv.tv_sec, &ts);
	sprintf(buffer, "%04d%02d%02d:%02d:%02d:%02d.%06d",
			ts.tm_year+1900,
			ts.tm_mon+1,
			ts.tm_mday,
			ts.tm_hour,
			ts.tm_min,
			ts.tm_sec,
			tv.tv_nsec);
}
