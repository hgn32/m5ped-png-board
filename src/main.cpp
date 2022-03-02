#ifndef _H_M5EPD
#define _H_M5EPD
#include <M5EPD.h>
#include <WiFi.h>
#include <Preferences.h>
#include <HTTPClient.h>
#endif	// _H_M5EPD
#include "./header.h"
#include "./unixtime.cpp"
#include "./secret.h"

/********************/
// Const
/********************/
//secret.h
//const TypeAccessPoint AccessPoints[] {
//    {"ssid1", "pwd1"},
//    {"ssid2", "pwd2"},
//};
//const char* url_png = "http://**/**.png";
//const char* url_api = "http://**/**";

/********************/
// Global
/********************/
M5EPD_Canvas canvas(&M5.EPD);
rtc_time_t RTCTime;
rtc_date_t RTCDate;
struct tm NTPDatetime;
Preferences preferences;
char lastsync_buff[64];
char today_buff[64];


float get_battery_voltage()
{
  uint32_t vol = M5.getBatteryVoltage();
  if(vol < 3300)
    {
        vol = 3300;
    }
    else if(vol > 4350)
    {
        vol = 4350;
    }
    float battery = (float)(vol - 3300) / (float)(4350 - 3300);
    if(battery <= 0.01)
    {
        battery = 0.01;
    }
    if(battery > 1)
    {
        battery = 1;
    }

    return battery;
}

void setup()
{
    //初期化
    M5.begin();
    float battery_voltage = get_battery_voltage();    // RTC.beginの前にやる
    M5.RTC.begin();

    //wifi 接続
    int try_count = 10;
    int i = 0;
    while(i < sizeof(AccessPoints) / sizeof(TypeAccessPoint)){
        WiFi.begin(AccessPoints[i].ssid, AccessPoints[i].key);
        Serial.print(AccessPoints[i].ssid);
        try_count = 10;
        while (WiFi.status() != WL_CONNECTED && try_count-- > 0) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        if(WiFi.status() == WL_CONNECTED){
            break;
        }
        i++;
    }
    //EEPROMがデータ取得
    preferences.begin("preferences", false);
    preferences.getString("lastsync", lastsync_buff, 64);
    uint16_t boot_count = preferences.getUShort("boot_count", 0);
    if(boot_count > 288 || M5.BtnP.isPressed()){
        boot_count = 0;
        log_i("Clear boot_count");
    }else{
        boot_count++;
        log_i("boot_count = %d", boot_count);
    }
    preferences.putUShort("boot_count", boot_count);
    preferences.end();
    

    //wifi接続してたらの処理
    if(WiFi.status() == WL_CONNECTED){
        //キャンパス設定
        M5.EPD.SetRotation(90); 
        canvas.createCanvas(540, 960);
        canvas.createRender(2, 256);
        canvas.setTextSize(2);

        //battery情報の取得と描画
        Serial.println("update battery_level");
        char battery_buf[20];
        int  battery_level = battery_voltage * 100;
        sprintf(battery_buf, "%d%%", battery_level);

        //battery情報のサーバ送信
        Serial.println("update status");
        HTTPClient http;
        char jsonbuf[40];
        http.begin(url_api);
        http.addHeader("Content-Type", "application/json");
        sprintf(jsonbuf, "{\"level\":%d,\"voltage\":%.5f}", battery_level, battery_voltage);
        http.POST(jsonbuf);
        delay(15);
        //Serial.println(http.getString());

        //png取得と描画
        Serial.println("draw png");
        canvas.drawPngUrl(url_png);
        canvas.drawString(battery_buf, 5, 940);
    
        //NTPで接続
        M5.RTC.getTime(&RTCTime);
        M5.RTC.getDate(&RTCDate);
        sprintf(today_buff, "%04d%02d%02d", RTCDate.year, RTCDate.mon, RTCDate.day);
        if(strcmp(today_buff, lastsync_buff) != 0){
            //init NTP
            configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
            getLocalTime(&NTPDatetime);
            RTCDate.year = NTPDatetime.tm_year + 1900;
            RTCDate.mon =  NTPDatetime.tm_mon + 1;
            RTCDate.day = NTPDatetime.tm_mday;
            RTCTime.hour = NTPDatetime.tm_hour;
            RTCTime.min =  NTPDatetime.tm_min;
            RTCTime.sec = NTPDatetime.tm_sec;
            M5.RTC.setTime(&RTCTime);
            M5.RTC.setDate(&RTCDate);
            Serial.println("sync NTP");
            //save to NVS
            preferences.begin("preferences", false);
            sprintf(today_buff, "%04d%02d%02d", RTCDate.year, RTCDate.mon, RTCDate.day);
            preferences.putString("lastsync", today_buff);
            preferences.end();
        }else{
            Serial.print("today : ");
            Serial.println(today_buff);
        }

        //更新時刻の描画
        Serial.print("update time : ");
        M5.RTC.getTime(&RTCTime);
        M5.RTC.getDate(&RTCDate);
        sprintf(today_buff, "%02d/%02d %02d:%02d:%02d", RTCDate.mon, RTCDate.day, RTCTime.hour, RTCTime.min, RTCTime.sec);
        canvas.drawString(today_buff, 60, 940);
        Serial.println(today_buff);

        //2回に一回だけフル書き換え
        /*
        if(boot_count % 5 == 0){
            Serial.println("full clear");
            M5.EPD.Clear();
            canvas.pushCanvas(0, 0, UPDATE_MODE_INIT);
        }
        */
        M5.EPD.Clear();
        canvas.pushCanvas(0, 0, UPDATE_MODE_INIT);


        //画面更新
        canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
        delay(2 * 1000);
    }else{
        Serial.println("wifi conection error");
    }
    
    //sleep時間を決める
    M5.RTC.getTime(&RTCTime);
    M5.RTC.getDate(&RTCDate);
    uint64_t unix = date2sec(RTCDate.year,RTCDate.mon, RTCDate.day, RTCTime.hour, RTCTime.min, RTCTime.sec);
    uint64_t sleep_seconds = (unix - (unix % (60 * 60)) + (60 * 60)) - unix;
    if(RTCTime.hour >=20 || RTCTime.hour <= 7){
        sleep_seconds = (unix - (unix % (60 * 60)) + (60 * 60 * 3)) - unix;
    }
    if(RTCTime.min >= 55 && RTCTime.min <= 59){
        sleep_seconds = sleep_seconds + 60 * 60 * 1; //few min + a hour
    }

    //Power OFF
    Serial.print("sleep : ");
    Serial.println(sleep_seconds);
    M5.disableEPDPower();
    M5.disableEXTPower();
    
    //deep sleep(秒)
    //パッテリー駆動時のみ動きここで止まる
    M5.shutdown(sleep_seconds);

    //USB駆動時のみ働く(u秒)
    esp_sleep_enable_timer_wakeup(sleep_seconds * 1000 * 1000);
    esp_deep_sleep_start();  
    delay(5 * 1000);
}
 
void loop()
{

}