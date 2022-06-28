#ifndef _TIME_H
#define _TIME_H 1

#ifdef __cpluplus
extern "C" {
#endif

struct tm {
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

typedef long clock_t;
typedef long long time_t;

#ifdef __cpluplus
}
#endif

#endif
