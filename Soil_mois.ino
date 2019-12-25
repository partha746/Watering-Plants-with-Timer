#include "HTTPSRedirect.h"
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <BlynkSimpleEsp8266.h>
#include "wifi_passphrares.h" //includes variables SSID1, WifiPass, auth[] & *GScriptId

#define SMSensor A0
#define Relay D1
#define LED D0

BlynkTimer timer;

WidgetLED led(V0);
WidgetTerminal terminal(V1);
  
ESP8266WiFiMulti wifiMulti;
WiFiUDP ntpUDP;

const long utcOffsetInSeconds = 19800;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const int httpsPort = 443;

String relayStatus;
String url;
float moisture_percentage;
int sensor_analog;
int systemStarted = millis();
float sensorErrorThresh = 90.0; // Moisture reading more than this is sensor failure
int wateringTime = 16; //24 Hrs clock time // Time after to start watering plants
int chkWPTimer = 5*30000UL; //Watering plants timer 
int NRDelay = 15*60000UL; //Watering plants timer 
int AHDelay = 1*60*60000UL;

void OTA(){
  ArduinoOTA.setHostname("Plantwatering_balc_out");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
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
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void blynkConnect()
{
  Blynk.begin(auth, SSID1, WifiPass);
}

void reportSoilMois(){
  timeClient.update();
  if (timeClient.getHours() == wateringTime){
    Serial.println("watering Plants");
    digitalWrite (Relay, HIGH);
    led.on();
    timeClient.update();
    blynkConnect();
    Blynk.notify("Starting Watering Plants @ " + timeClient.getFormattedTime() + "!!!!!");
    delay(180000UL);

    Serial.println("watering Plants Done");
    digitalWrite (Relay, LOW);
    timeClient.update();
    blynkConnect();
    Blynk.notify("Stopping Watering Plants @ " + timeClient.getFormattedTime() + "!!!!!");
    led.off();
    delay(AHDelay);
  }

  sensor_analog = analogRead(SMSensor);
  moisture_percentage = ( 100 - ( (sensor_analog/1023.00) * 100 ) );
  blynkConnect();
  Blynk.virtualWrite(V1, moisture_percentage);
}

void setup() {
  Serial.begin(9600);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  WiFi.mode(WIFI_STA);

  wifiMulti.addAP(SSID1, WifiPass);
  wifiMulti.addAP(SSID2, WifiPass);
  
  pinMode(Relay, OUTPUT);
  pinMode(SMSensor, INPUT);

  Serial.println("Connecting to wifi: ");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.printf("Connected to SSID: %s & IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

  OTA();

  blynkConnect();
  led.off();

  timer.setInterval(NRDelay, reportSoilMois);

  timeClient.begin();
}

void loop() {
  ArduinoOTA.handle();    
  Blynk.run(); 
  timer.run();
}
