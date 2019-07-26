#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#else
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#endif
#include <AutoConnect.h>

#include <time.h>

#define ENABLE_ACTIVITY_LED
//#define PIN_ACTIVITY_LED LED_BUILTIN
#define PIN_ACTIVITY_LED 10
#define ENABLE_MDNS_SERVICE
#define ENABLE_ARDUINO_OTA

#if !defined(ESP8266)
//# define ENABLE_SERIAL2_OUTPUT
#endif

#if defined(ESP8266)
typedef ESP8266WebServer WebServer;
#endif

WebServer Server;
//AutoConnectConfig Config;
AutoConnect Portal(Server);

char g_ntp_server[64] = "pool.ntp.org";

#if defined(ENABLE_ACTIVITY_LED)
bool g_led_state = true;
#endif

//HardwareSerial SerialOut(2);
#if defined(ESP8266)
bool getLocalTime(struct tm *info, uint32_t ms = 5000)
{
  uint32_t start = millis();
  time_t now;
  while ((millis() - start) <= ms)
  {
    time(&now);
    localtime_r(&now, info);
    if (info->tm_year > (2016 - 1900))
    {
      return true;
    }
    delay(10);
  }
  return false;
}
#endif

bool printLocalTime()
{
#if !defined(ESP8266)
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return false;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
#endif
  return true;
}

void rootPage()
{
  char content[] = "Hello, world";
  Server.send(200, "text/plain", content);
}

// the setup function runs once when you press reset or power the board
void setup()
{
  // delay for debug purpose
  delay(3000);

  // initialize Serial
  //  Serial.begin(115200);
  Serial.begin(4800, SERIAL_8N1);
  Serial.print("Serial begun");

  // setup AutoConnect
  Server.on("/", rootPage);
//  Config.autoReset = true;
//  Portal.config(Config);
  if (Portal.begin())
  {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
#if defined(ENABLE_MDNS_SERVICE)
    // setup mDNS service, so that you can access the device from the same wifi network for in case.
    if (MDNS.begin("esp8266ntp"))
    {
      MDNS.addService("http", "tcp", 80);
    }
#endif
  }

#if defined(ENABLE_ARDUINO_OTA)
  // setup OTA
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
#endif

#if defined(ENABLE_ACTIVITY_LED)
  pinMode(PIN_ACTIVITY_LED, OUTPUT);
#endif

  // configTime Server
  configTime(0, 0, g_ntp_server); // handle as UTC time.

#if defined(ENABLE_SERIAL2_OUTPUT)
  // if enabled, output pseudo GPS data to Serial2 at the same time.
  Serial2.begin(4800, SERIAL_8N1);
  Serial2.println("Serial2 begun");
#endif
}

// the loop function runs over and over again forever
void loop()
{
  // for AutoConnect Portal
  Portal.handleClient();

  // for OTA
#if defined(ENABLE_ARDIONO_OTA)
  ArduinoOTA.handle();
#endif

  // for blinking activity LED
#if defined(ENABLE_ACTIVITY_LED)
  g_led_state = !g_led_state;
  char buf[256];
  sprintf(buf, "turn LED %s", g_led_state ? "on" : "off");
  Serial.println(buf);
  digitalWrite(PIN_ACTIVITY_LED, g_led_state ? HIGH : LOW); // turn the LED on (HIGH is the voltage level)
#endif

  // dump current time
//  printLocalTime();

  // send time as GPS format
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
  }
  else
  {
    char buf[256];
    sprintf(buf, "$GPRMC,%02d%02d%02d.000,A,0000.00,N,0000.00,W,000.0,000.0,%02d%02d%02d,,E*",
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
            timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year % 100);
    char *ptr = buf + 1;
    uint8_t check = 0;
    while (*ptr != '*')
    {
      check ^= *ptr;
      ptr++;
    }
    sprintf(buf + strlen(buf), "%02X\x0d\x0a", check);
//    Serial.print("sending : ");
    Serial.write(buf);
#if defined(ENABLE_SERIAL2_OUTPUT)
    Serial2.write(buf);
#endif
  }
  // wait for a second if wifi status is okay.  The cycle slows down otherwise, so that user can see the status.
  delay(WiFi.status() != WL_CONNECTED ? 2500 : 1000);
//  delay(1000); // wait for a second
}
