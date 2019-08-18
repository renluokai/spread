#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/helper.h"
void Time::FullTime(char*  buffer)
{
	struct timespec tv;
	struct tm ts;
	clock_gettime(CLOCK_REALTIME,&tv);
	localtime_r(&tv.tv_sec, &ts);
	sprintf(buffer, "%04d%02d%02d_%02d%02d%02d.%06d",
			ts.tm_year+1900,
			ts.tm_mon+1,
			ts.tm_mday,
			ts.tm_hour,
			ts.tm_min,
			ts.tm_sec,
			tv.tv_nsec);
	memset(buffer+22,0,1);
}
int Time::Hhmmss2sec(const char* timeStr)
{
	int h=0;
	int m=0;
	int s=0;
	h=atoi(timeStr);
	h%=24;

	m=atoi(timeStr+3);
	m%=60;

	s=atoi(timeStr+6);
	s%=60;
	return h*3600+m*60+s;
}
