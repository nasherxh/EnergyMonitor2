//setup RTC module. DS3231 or DS1307. Uses I2C communciation
  /* SDA to Arduino A4 / ESP32 IO21
     SCL to Arduino A5 / ESP32 IO22
     */
#include "RTClib.h"
  RTC_DS3231 rtc;
//RTC_DS1307 rtc;


void setup() {

  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) {
  ; // wait for serial port to connect. Needed for native USB port only
  }
  setUpRTC();
}

void loop() {
//      char dateTime[] = "YY-MM-DD, hh:mm:ss";
//      rtc.now().toString(dateTime);
//      Serial.println(dateTime);
//      delay(1000);
    char dateStamp[] = "YYYY-MM-DD";
    char timeStamp[] = "hh:mm:ss";
    rtc.now().toString(dateStamp);
    rtc.now().toString(timeStamp);
    Serial.println(dateStamp);
    Serial.println(timeStamp);

    delay(1000);

  }


void setUpRTC(){
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

//  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
//  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
}
