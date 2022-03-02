#ifndef _H_M5EPD
#define _H_M5EPD
#include <M5EPD.h>

static int t[] = { 306,337,0,31,61,92,122,153,184,214,245,275 };


uint64_t date2sec(uint32_t y, uint32_t m, uint32_t d, uint32_t hour, uint32_t min, uint32_t sec)
{
    const uint64_t base = 1969*365L + 1969/4 - 1969/100 + 1969/400 + 306 + 1;
    uint64_t days;
    if (m < 3) y--;
    days = y*365L + y/4 - y/100 + y/400 + t[m-1] + d - base;
    return days * (24*60*60) + ((hour * 60 + min) * 60 + sec);
}

void sec2date(rtc_date_t* date, rtc_time_t* time, uint64_t sec)
{
    uint32_t y, m, d, n, leap;
    const uint64_t BASE = 1969*365L + 1969/4 - 1969/100 + 1969/400 + 306;
	    // BASE は 0年3月1日から 1970年1月1日までの日数
	const uint32_t Y400 = 365L * 400 + 97;      // 400年間の日数、閏年は97回
	const uint32_t Y100 = 365L * 100 + 24;      // 100年間の日数、閏年は24回
	const uint32_t Y4   = 365L *   4 +  1;      //   4年間の日数、閏年は 1回
	const uint32_t Y1   = 365;                  //   1年間の日数

    sec += BASE * (24 * 60 * 60);           // sec は 0年3月1日からの秒数
    time->sec  = sec % 60; sec /= 60;       // sec は 0年3月1日からの分数
    time->min  = sec % 60; sec /= 60;       // sec は 0年3月1日からの時間数
    time->hour = sec % 24; d = sec / 24;    // d は 0年3月1日からの日数

    n = d / Y400; d %= Y400; y  = n * 400;  // y は 400年単位の年数、d は 400年内の日数
    n = d / Y100; d %= Y100; y += n * 100;  // y は 100年単位の年数を加えたもの、d は 100年内の日数
    leap = (n == 4);                        // 400年に一度の閏年が見つかったかどうか
    n = d / Y4;   d %= Y4;   y += n * 4;    // y は 4年単位の年数を加えたもの、d は 4年内の日数
    n = d / Y1;   d %= Y1;   y += n;        // y は 年数、d は 1年内の日数
#if 0
	const uint32_t M5 = 31 + 30 + 31 + 30 + 31; // 3月～7月、8月～12月の5か月ずつの日数
	const uint32_t M2 = 31 + 30;  // 3月4月、5月6月、8月9月、10月11月、1月2月 の 2か月ずつの日数
	const uint32_t M1 = 31;       // 3月、5月、7月、8月、10月、12月、1月 の 1か月の日数
    if (n == 4) leap = 1;                   // 4年に一度の閏年が見つかったかどうか
    n = d / M5;   d %= M5;   m  = n * 5;    // m は 5か月単位の月数、d は 5か月内の日数
	n = d / M2;   d %= M2;   m += n * 2;    // m は 2か月単位の月数を加えたもの、d は 2か月内の日数
    n = d / M1;   d %= M1;   m += n + 3;    // m は 3月から始まる月、d は 月内の日数
    if (leap) m = 2, d = 29;                // 閏年
    else if (m <= 12) d++;                  // 3月～12月は、日を 1 からに始まるようにする
    else y++, m -= 12, d++;                 // 13月14月は 1月2月しに、年を翌年にし、日を 1 からにする
#else
    if (leap || n == 4) m = 2, d = 29;      // 閏年
    else {
        m = (d * 5 + 2) / 153 + 3;          // 年内の日数から、月を求める式、3月から始まる
        if (m > 12) m -= 12, y++;           // 13月14月は 1月2月にし、年を翌年にする
        d -= t[m-1] - 1;                    // 年内の日数から、前月までの日数を引き、当月の日数にする
    }
#endif
    date->year = y;
    date->mon = m;
    date->day = d;
}

#endif	// _H_M5EPD