#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include <ESP8266WiFiMulti.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RTClib.h>
#include <NTPClient.h>
#include <WifiUdp.h>

RTC_DS3231 rtc;
ESP8266WiFiMulti wifiMulti;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

#define WIFI_SSID ""
#define WIFI_PASS ""

#define DEVICE "ESP8266"
#define INFLUXDB_URL ""
#define INFLUXDB_TOKEN ""
#define INFLUXDB_ORG ""
#define INFLUXDB_DB_NAME ""
#define INFLUXDB_USER ""
#define INFLUXDB_PASSWORD ""

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);
Point sensor("wifi status");

//Temperature Sensor
const int oneWireBus = 2; //D4
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

#define CLOCK_INTERRUPT_PIN 16 //D0

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void setup() {
  Serial.begin(9600);
  
  #ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
  #endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  pinMode(LED_BUILTIN, OUTPUT); 
  digitalWrite(LED_BUILTIN, HIGH);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to: ");
  Serial.print(WIFI_SSID);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.print(WiFi.localIP());

  //ntp stuff and set rtc
  timeClient.begin();
  timeClient.setTimeOffset(0);
  timeClient.update();
  rtc.adjust(DateTime(timeClient.getEpochTime()));

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

  rtc.disable32K(); // RTC
  pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), onAlarm, FALLING);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.writeSqwPinMode(DS3231_OFF);
  rtc.disableAlarm(2);
  if(!rtc.setAlarm1(rtc.now() + TimeSpan(10),DS3231_A1_Second)) {
    Serial.println("Error, alarm wasn't set!");
  }
  else {
    Serial.println("Alarm will happen in 10 seconds!");  
  }
}

void loop() {
  //digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
  // but actually the LED is on; this is because
  // it is active low on the ESP-01)
  //delay(1000);                      // Wait for a second
  //digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  //delay(2000);                      // Wait for two seconds (to demonstrate the active low LED)


  //RTC MAGIC
  DateTime now = rtc.now();
  Serial.println();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  Serial.print(" since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());
  Serial.print("s = ");
  Serial.print(now.unixtime() / 86400L);
  Serial.println("d");

  Serial.println();

  Serial.print("SQW: ");
  Serial.print(digitalRead(CLOCK_INTERRUPT_PIN));
  Serial.print(" Alarm1: ");
  Serial.print(rtc.alarmFired(1));
  Serial.println();
  
  if(rtc.alarmFired(1)) {
    rtc.clearAlarm(1);
    Serial.println("Alarm cleared!");

    //TEST
      timeClient.update();
  Serial.print("Formated Time: ");
  Serial.print(timeClient.getFormattedTime());
  Serial.println();
  sensors.requestTemperatures();
  Serial.println("Wait 1s for Temperature Sensor");
  delay(1000);
  Serial.println(sensors.getTempCByIndex(0));
  // Store measured value into point
  sensor.clearFields();
  // Report RSSI of currently connected network
  sensor.addField("rssi", WiFi.RSSI());
  sensor.addField("temperature", sensors.getTempCByIndex(0));
  // Print what are we exactly writing
  //Serial.print("Writing: ");
  //Serial.println(client.pointToLineProtocol(sensor));
  // If no Wifi signal, try to reconnect it
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }
  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  }
  
  delay(9000);
}

ICACHE_RAM_ATTR void onAlarm() {
  Serial.println("Alarm occured!");
}
