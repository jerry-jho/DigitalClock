
#include <Wire.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "ssid.private.h"
// put a 'ssid.private.h' in /include with content:
//const char *ssid     = "<SSID>";
//const char *password = "<Password>";

const long utcOffsetInSeconds = 3600 * 8;

const char *ntpServer = "asia.pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;

#define WIRE Wire



int minute_of_the_day = 0;
int odd_day = 0;
int second = 0;
unsigned long mm = 0;

void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.printf("%d-%d-%d %d:%d:%d", timeinfo.tm_year + 1900, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec); // 格式化输出
}

#define L0 0x20
#define L1 0x21
#define L2 0x23
#define L3 0x22

#define S7A 0x01
#define S7B 0x02
#define S7C 0x04
#define S7D 0x08
#define S7E 0x10
#define S7F 0x20
#define S7G 0x40
#define S7P 0x80

/*
   
   A
   -
F | |   B
   -    G
E | |   C
   -
   D
*/
constexpr unsigned char seg7[] = {
    S7G,                      //0
    (S7A|S7D|S7E|S7F|S7G),    //1
    (S7F|S7C),                //2
    (S7F|S7E),                //3
    (S7A|S7D|S7E),            //4
    (S7B|S7E),                //5
    S7B,                      //6
    (S7G|S7F|S7E|S7D),        //7
    0,                        //8
    S7E,                      //9
    0xFF                      //off                     
    };

void write_dig(int index, int val, int b = 0) {
  if (val > 9) {
    val = 10;
  }
  WIRE.beginTransmission(index);
  if (b) {
    WIRE.write(seg7[val]);
  } else {
    WIRE.write(seg7[val] | 0x80);
  }
  WIRE.endTransmission();
}

void write_dig4(int val, int b = 0) {
  write_dig(L0, val / 1000);
  val = val % 1000;
  write_dig(L1, val / 100);
  val = val % 100;
  write_dig(L2, val / 10);
  val = val % 10;
  write_dig(L3, val, b);
}

void setup() {
  WIRE.begin();
  pinMode(16, OUTPUT);
  pinMode(14, OUTPUT);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 100 );
    digitalWrite(16, 0);
    digitalWrite(14, 0);
    delay ( 100 );
    digitalWrite(16, 1);
    digitalWrite(14, 1); 
  }
  Serial.print ( "Connected\n" );
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  int year = timeinfo.tm_year + 1900;
  write_dig4(year);
  delay(5000);
  write_dig4((timeinfo.tm_mon + 1) * 100 + timeinfo.tm_mday);
  delay(5000);
  minute_of_the_day = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  second = timeinfo.tm_sec;
  mm = millis();
  odd_day = timeinfo.tm_yday % 2;
  write_dig4(timeinfo.tm_hour*100+timeinfo.tm_min, 1 - odd_day);
  // timeClient.begin();
  // int h = timeClient.getHours();
  // int m = timeClient.getMinutes();
  // second = timeClient.getSeconds();
  // Serial.printf ( "h=%d, m=%d, s=%d\n", h, m,second);
}



void loop() {
  auto the_mm = millis();
  if (the_mm - mm > 1000) {
    mm = the_mm;
    int pinVal = second % 2;
    digitalWrite(16, pinVal);
    digitalWrite(14, pinVal);
    second += 1;
    if (second == 60) {
      second = 0;
      minute_of_the_day += 1;
      // if (minute_of_the_day == 24*60) {
      //   minute_of_the_day = 0;
      // }
      if (minute_of_the_day % 60 == 0) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
          minute_of_the_day = timeinfo.tm_hour * 60 + timeinfo.tm_min;
          second = timeinfo.tm_sec;
          odd_day = timeinfo.tm_yday % 2;
          mm = millis();
        }      
      }
      int h = minute_of_the_day / 60;
      int m = minute_of_the_day % 60;
      write_dig4(h*100+m, 1 - odd_day);
    }
  }
  delay(1);
}