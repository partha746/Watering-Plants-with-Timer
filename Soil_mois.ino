#include "HTTPSRedirect.h"
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <BlynkSimpleEsp8266.h>
#include "wifi_passphrares.h" //includes variables SSID1, WifiPass, auth[] & *GScriptId

#define SMSensor A0
#define LED D0
#define Relay D1
#define TRIGGERPIN D2
#define ECHOPIN D3

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
int wateringTime = 8; //24 Hrs clock time // Time after to start watering plants
int chkWPTimer = 5*30000UL; //Watering plants timer 
int NRDelay = 15*60000UL; //Watering plants timer 
int AHDelay = 1*60*60000UL;
float LWL = 45.0; //Lower water level
float HWL = 12.0; //Lower water level

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

float sonarDist(){
  long duration, distance;
  digitalWrite(TRIGGERPIN, LOW);  
  delayMicroseconds(3); 
  digitalWrite(TRIGGERPIN, HIGH);
  delayMicroseconds(12);   
  digitalWrite(TRIGGERPIN, LOW);
  duration = pulseIn(ECHOPIN, HIGH);
  distance = (duration/2) / 29.1;

  float WL = ((LWL - distance) / (LWL - HWL)) * 100;
  
  return WL;
}

void blynkConnect()
{
  Blynk.begin(auth, SSID1, WifiPass);
}

void reportSoilMois(){
  if (sonarDist() >= 5.0){
    timeClient.update();
    if (timeClient.getHours() == wateringTime){
      Serial.println("watering Plants");
      digitalWrite (Relay, HIGH);
      led.on();
      timeClient.update();
      blynkConnect();
      Blynk.notify("Starting Watering Plants @ " + timeClient.getFormattedTime() + "!!!!!");
      Blynk.virtualWrite(V2, HIGH);
      delay(180000UL);
  
      Serial.println("watering Plants Done");
      digitalWrite (Relay, LOW);
      timeClient.update();
      blynkConnect();
      Blynk.virtualWrite(V2, LOW);
      Blynk.notify("Stopping Watering Plants @ " + timeClient.getFormattedTime() + "!!!!!");
      led.off();
      delay(AHDelay);
    }
  }
  else
  {
    blynkConnect();
    Blynk.notify("Low water tank level @ " + timeClient.getFormattedTime() + "!!!!!");    
  }
  
  sensor_analog = analogRead(SMSensor);
  moisture_percentage = ( 100 - ( (sensor_analog/1023.00) * 100 ) );
  blynkConnect();
  Blynk.virtualWrite(V1, moisture_percentage);
  Blynk.virtualWrite(V3, sonarDist());
}

void reportwaterLevel(){
  Blynk.virtualWrite(V3, sonarDist());
}

void reporthwTime(){
  timeClient.update();
  Blynk.virtualWrite(V4, timeClient.getFormattedTime());
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

  pinMode(TRIGGERPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

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
  timer.setInterval(NRDelay, reportwaterLevel);
  timer.setInterval(1000, reporthwTime);

  timeClient.begin();
}

BLYNK_WRITE(V2)
{
  if(param.asInt() == 1){digitalWrite(Relay, HIGH); led.on(); Blynk.virtualWrite(V3, sonarDist());}
  else if (param.asInt() == 0){digitalWrite(Relay, LOW); led.off(); Blynk.virtualWrite(V3, sonarDist());}
}

BLYNK_WRITE(V5)
{
  if(param.asInt() == 1){ESP.restart();}
}

BLYNK_WRITE(V6)
{
  if(param.asInt() == 1){
    Blynk.virtualWrite(V3, sonarDist());
  }
}

void loop() {
  ArduinoOTA.handle();    
  Blynk.run(); 
  timer.run();
}
