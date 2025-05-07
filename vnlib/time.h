#ifndef TIME_H
#define TIME_H

struct tm {
    unsigned tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year,
        tm_wday, tm_yday;
    int tm_isdst;
};

struct time_t {
    unsigned sec, min, day, year, isdst;
};

typedef struct time_t time_t;

static time_t *_t;
static struct tm *tm;

static void _time(int sec, int min, int day, int year, int isdst) {
    _t->sec = sec;
    _t->min = min;
    _t->day = day;
    _t->year = year;
    _t->isdst = isdst;
}

time_t *time(time_t *t) {
    _t = t;
    #dw 0xc009
    void (*__time)() = (void*)_time;
    __time();
}

static int days[] = {
    31,28,31,30,31,30,31,31,30,31,30,31,
};

static int mcode[] = {
    0,3,3,6,1,4,6,2,5,0,3,5,
};

static int ccode[] = { 4,2,0,6, };

struct tm *localtime(time_t *t) {
    tm->tm_sec = t->sec;
    tm->tm_min = t->min%60;
    tm->tm_hour = t->min/60;
    tm->tm_year = t->year;
    int y = t->year-1900;
    if(y%4 == 0 && y%100 != 0)
        days[1] = 29;
    printf("%d\n", days[1]);
    int i, j, d = 0;
    for(i = 0; d < t->day; i++)
        for(j = 0; j < days[i]; j++) d++;
    tm->tm_mday = j;
    tm->tm_mon = i;
    tm->tm_yday = t->day;
    tm->tm_isdst = t->isdst;
    d = mcode[i] + ((y%100)+((y%100)/4))%4 + j;
    if(y >= 1700)
        d += ccode[((y-1700)/100)%4];
    if(days[1] == 29 && i < 2) d--;
    tm->tm_wday = d%7;
    days[1] = 28;
    return tm;
}

#endif
