#include "HTTPSRedirect.h"
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <BlynkSimpleEsp8266.h>
#include "wifi_passphrares.h" //includes variables SSID1, WifiPass, auth[] & *GScriptId

#define SMSensor A0
#define Relay D1
#define LED D0

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
int wateringTime = 9; //24 Hrs clock time // Time after to start watering plants
int chkWPTimer = 5*30000UL; //Watering plants timer 
int NRDelay = 15*60000UL; //Watering plants timer 
int AHDelay = 1*60*60000UL;

void blynkConnect()
{
  Blynk.begin(auth, SSID1, WifiPass);
}

void customDelay (int cDelay){
  long counterStart = millis();
  long counterElapsed = millis() - counterStart;
  while(counterElapsed <= cDelay){
    delay(30500UL);
    counterElapsed = millis() - counterStart;
  }  
}
 
void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  Serial.begin(9600);
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
}

int reportSensorError(float smReading){
  HTTPSRedirect client(httpsPort);
  timeClient.begin();
  client.setInsecure();

  if (smReading > sensorErrorThresh){
    url = String("/macros/s/") + GScriptId + "/exec?relay=NA&tmp=0&status=Sensor_Failure";
    while (!client.connected())           
      client.connect(host, httpsPort);
    client.printRedir(url, host, googleRedirHost);      
    timeClient.update();
    blynkConnect();
    Blynk.notify("Sensor Failure@ " + timeClient.getFormattedTime() + "!!");
    return 1;
  }
  else{
    return 0;
  }
}

void loop() {
  blynkConnect();
  Blynk.run(); 
  
  HTTPSRedirect client(httpsPort);
  timeClient.begin();
  client.setInsecure();

  led.off();
  
  timeClient.update();
  Serial.println(timeClient.getHours());
  if (timeClient.getHours() == wateringTime){
    Serial.println("watering Plants");
    digitalWrite (Relay, HIGH);
    led.on();
    timeClient.update();
    blynkConnect();
    Blynk.notify("Starting Watering Plants @ " + timeClient.getFormattedTime() + "!!!!!");
    customDelay(chkWPTimer);

    Serial.println("watering Plants Done");
    digitalWrite (Relay, LOW);
    timeClient.update();
    blynkConnect();
    Blynk.notify("Stopping Watering Plants @ " + timeClient.getFormattedTime() + "!!!!!");
    led.off();
    customDelay(AHDelay);
  }
  
  customDelay(NRDelay);
  sensor_analog = analogRead(SMSensor);
  moisture_percentage = ( 100 - ( (sensor_analog/1023.00) * 100 ) );
  blynkConnect();
  Blynk.virtualWrite(V1, moisture_percentage);
  Blynk.run(); 
}
