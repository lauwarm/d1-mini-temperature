#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RTClib.h>
#include <NTPClient.h>
#include <WifiUdp.h>

const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

const char* DEVICE = "ESP8266";
const char* SENSOR = "DS18B20";
const char* INFLUXDB_URL = ""; 
//const char* INFLUXDB_TOKEN = ""
//const char* INFLUXDB_ORG = ""
const char* INFLUXDB_DB_NAME = "";
const char* INFLUXDB_USER = "";
const char* INFLUXDB_PASSWORD = "";

const char daysOfTheWeek[7][12] = 
  {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; // ---TESTING---

// Realtime Clock
RTC_DS3231 rtc;

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// InfluxDB Client
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);
Point sensor("wifi status");

// Temperature Sensor
const int oneWireBus = 2; //D4
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

const int CLOCK_INTERRUPT_PIN = 16; // ---TESTING---

//---

void setup() {
  Serial.begin(9600); // ---TESTING---
  
  #ifndef ESP8266 // ---TESTING---
  while (!Serial); // wait for serial port to connect. Needed for native USB // ---TESTING---
  #endif // ---TESTING---

  checkRTC();

  setWifi();
  
  debugMessages(1); // ---TESTING---

  setInfluxDB();

  setRTCTime();

  setRTCAlarm();
  pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP); // ---TESTING---
  attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), onAlarm, FALLING); // ---TESTING---
}

void loop() {
  //debugMessages(5); // ---TESTING---
  //debugMessages(2); // ---TESTING---
  
  // Activate Sensor and wait a Second before reading Values
  sensors.requestTemperatures();
  delay(1000);
  
  // Store measured value into point
  sensor.clearFields();
  sensor.addField("rssi", WiFi.RSSI());
  sensor.addField("temperature", sensors.getTempCByIndex(0));
  
  debugMessages(3); // ---TESTING---
    
  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  Serial.println("Clearing Alarm 1"); // ---TESTING---
  // Clear Alarm 1 to Shutdown ESP8266
  rtc.clearAlarm(1);
  Serial.println("Alarm 1 cleared"); // ---TESTING---
  
  //while(1);
  delay(15000);
}

ICACHE_RAM_ATTR void onAlarm() { // ---TESTING---
  Serial.println("Alarm occured!"); // ---TESTING---
} // ---TESTING---

void checkRTC() {
    if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
}

void setRTCTime() {
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!"); // ---TESTING---
    timeClient.begin();
    timeClient.setTimeOffset(0);
    timeClient.update();
    rtc.adjust(DateTime(timeClient.getEpochTime()));
  }
}

void setRTCAlarm() {
  // Disable 32K
  rtc.disable32K();

  // Clear Alarm 1 and 2
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  // Disable SQW Pin
  rtc.writeSqwPinMode(DS3231_OFF);

  // Disable Alarm 2
  rtc.disableAlarm(2);

  // Set Alarm 1 to current Time + a Timespan in Seconds
  if(!rtc.setAlarm1(rtc.now() + TimeSpan(600),DS3231_A1_Second)) {
    Serial.println("Error, alarm wasn't set!");
  }
  else {
    Serial.println("Alarm will happen in 600 seconds!");  
  }
}

void setWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  debugMessages(4);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void setInfluxDB() {
  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void debugMessages(int choose) { // ---TESTING---
  switch (choose) {
    case 1:
    {
      Serial.println(); // ---TESTING---
      Serial.print("Connected! IP: "); // ---TESTING---
      Serial.print(WiFi.localIP()); // ---TESTING---
      Serial.println();
      break;
    }
    case 2:
    {
      //RTC MAGIC
      DateTime now = rtc.now(); // ---TESTING---
      Serial.print("Formated Time: "); // ---TESTING---
      Serial.println(); // ---TESTING---
      Serial.print(now.year(), DEC); // ---TESTING---
      Serial.print('/'); // ---TESTING---
      Serial.print(now.month(), DEC); // ---TESTING---
      Serial.print('/'); // ---TESTING---
      Serial.print(now.day(), DEC); // ---TESTING---
      Serial.print(" ("); // ---TESTING---
      Serial.print(daysOfTheWeek[now.dayOfTheWeek()]); // ---TESTING---
      Serial.print(") "); // ---TESTING---
      Serial.print(now.hour(), DEC);// ---TESTING---
      Serial.print(':'); // ---TESTING---
      Serial.print(now.minute(), DEC); // ---TESTING---
      Serial.print(':'); // ---TESTING---
      Serial.print(now.second(), DEC); // ---TESTING---
      Serial.println(); // ---TESTING---
      Serial.print(" since midnight 1/1/1970 = "); // ---TESTING---
      Serial.print(now.unixtime()); // ---TESTING---
      Serial.print("s = "); // ---TESTING---
      Serial.print(now.unixtime() / 86400L); // ---TESTING---
      Serial.println("d"); // ---TESTING---
      Serial.println(); // ---TESTING---
      break;
    }
    case 3:
    {
      // Print what are we exactly writing
      Serial.print("Writing: "); // ---TESTING---
      Serial.println(client.pointToLineProtocol(sensor)); // ---TESTING---  
      break;
    }
    case 4:
    {
      Serial.print("Connecting to: "); // ---TESTING---
      Serial.print(WIFI_SSID); // ---TESTING---
      break;
    }
    case 5:
    {
      Serial.print("Lost Power? - "); // ---TESTING---
      Serial.println(rtc.lostPower()); // ---TESTING---

      Serial.print("SQW: "); // ---TESTING---
      Serial.print(digitalRead(CLOCK_INTERRUPT_PIN)); // ---TESTING---
      Serial.print(" Alarm1: "); // ---TESTING---
      Serial.print(rtc.alarmFired(1)); // ---TESTING---
      Serial.println(); // ---TESTING---
      break;
    }
  }
}
