/*
Student Number: 09351876
Name: Fiona Hill
Programme: This is the basis of my project "Let Me In!", a small programme that 
lets the user know when their pet (in this case a cat) is outside a window and wants to come in. 
Handy for people who do not have a petdoor.
 */

#define BLYNK_TEMPLATE_ID "TMPL4e5iGDsBY"
#define BLYNK_TEMPLATE_NAME "CatMotionDetector"

#include <SPI.h>
#include <WiFi.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include <BlynkSimpleWifi.h>
#include <Arduino_MKRIoTCarrier.h>
#include "arduino_secrets.h" 
#include "Firebase_Arduino_WiFiNINA.h"


int status = WL_IDLE_STATUS;

///////enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
char b_auth_token[] = SECRET_B_AUTH_TOKEN;
char db_url[] = DATABASE_URL;
char db_secret[] = DATABASE_SECRET;


//Defining the Firebase data object

FirebaseData fbdo;
const long interval = 5000;
unsigned long previousMillis = millis();
unsigned long currentMillis = millis();
int count = 0;


// local port to listen on > this is used for the simulated motion detector
unsigned int localPort = 2390;      

char packetBuffer[256]; //buffer to hold incoming packet
bool motionDetected = false;

int motionStatus = 0; 
bool vigilantMode = false;


WiFiUDP Udp;

BlynkTimer timer;
MKRIoTCarrier carrier;

int red = 0, green = 0, blue = 255;
int ledColour = carrier.leds.Color(red, green, blue);

//This is the action taken when the button is activated by the user.
// It turns on the LEDs to blue (apparently cats can see the colour blue so ideally it'd attract the cat to the motion sensor)
//vigilant mode was a disaster that broke UDP. it didn't work as the on/off switch for the device.  
BLYNK_WRITE(V0) {
  // Set incoming value from virtual pin V0 to a variable
  int vigilantMode = param.asInt();
  if (vigilantMode == 1) {
    turnOnLEDS();
    vigilantMode = true;
  } else {
    turnOffLEDS();
    vigilantMode = false;
  }
}


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // attempt to connect to the wifi:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);

    //give it 10 seconds for connection
    delay(10000);
  }
  Serial.println("Connected to WiFi");
  printWifiStatus();

  Serial.println("\nStarting UDP Server...");
  Udp.begin(localPort);

  Blynk.begin(SECRET_B_AUTH_TOKEN, ssid, pass);
  timer.setInterval(5000L, writeMotion);

  Firebase.begin(DATABASE_URL, DATABASE_SECRET, ssid, pass);
  Firebase.reconnectWiFi(true);

  carrier.withCase();
  carrier.begin();

  Serial.println("Vigilant Mode Activated");
  
}

void loop() {
  Blynk.run();
  timer.run();

//  if (vigilantMode) {
    
    //carrier.leds.fill(ledColour);
    //carrier.leds.show();
  //check the simulated Motion Sensor in Packet Tracer
  motionDetected = checkMotionSensor();
  if (motionDetected) {
    currentMillis = millis();
    Serial.println("Motion Detected");
    writeMotion();
    Blynk.logEvent("let_me_in_");
  }
  delay(1000);

if (currentMillis - previousMillis >= interval){
  //saving the last time a message was sent
  previousMillis = currentMillis;
  Serial.println("Sending Message: ");
  sendMessage();
  Serial.println();
  count++;
  }

}

////////////////////////
///   Methods       ///
//////////////////////



//this method checks the simulated motion sensor in Packet tracer 
bool checkMotionSensor() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.print("Received packet from ");
    IPAddress remoteIp = Udp.remoteIP();
    Serial.print(remoteIp);
    Serial.print(", port ");
    Serial.println(Udp.remotePort());
    //read the packet into packetBuffer > this is the buffer used to store the data coming in from packet tracer
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
    if (packetBuffer[0] == '1'){
      return true; //motion is detected
      motionStatus = 1;
    } else {
      return false;
    }
  }
}

//This function is useful for 
//setting up the simulated motion detector
//in packet tracer
void printWifiStatus() {
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

//  send the motion detector status to Blynk
void writeMotion(){
int motionStatus = 1;
Blynk.virtualWrite(V1, motionStatus); 
}

void turnOnLEDS(){
  carrier.leds.fill(ledColour);
  carrier.leds.show();
}

void turnOffLEDS(){
  carrier.leds.fill(0);
  carrier.leds.show();
}

//This method is for sending a message to firebase.
void sendMessage() {
  String path = "/events";
  String jsonStr;
  Serial.println("Pushing json...");
  jsonStr = "{\"event\":\"push\",\"message\":\"Motion Detected\",\"count\":"+ String(count) + "}";
  Serial.println(jsonStr);
  if (Firebase.pushJSON(fbdo, path + "/motion", jsonStr)) {
    Serial.println("path: " + fbdo.dataPath());
  } else {
    Serial.println("error, " + fbdo.errorReason());
  }
  fbdo.clear();
}

