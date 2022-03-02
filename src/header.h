
typedef struct { 
  const char* ssid;
  const char* key;
} TypeAccessPoint;

uint64_t date2sec(uint32_t year,uint32_t month,uint32_t date, uint32_t hour,uint32_t minute,uint32_t second);
void sec2date(rtc_date_t* result_date,rtc_time_t* result_time, uint64_t unixTime);
