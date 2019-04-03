/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *  ESP8266 Arduino example
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_SSD1306.h>
#include <splash.h>
#include <ArduinoJson.h>  //https://arduinojson.org/v5/assistant/

#include <SPI.h>  
#include "RF24.h"
byte addresses[][6] = {"0"};
RF24 myRadio (D3, D4);
// TODO: Move this struct into a Header file
struct kothTeamStruct
{
  int teamId;
  int points;
};

typedef struct kothTeamStruct KothTeamStruct;
KothTeamStruct KothTeamPayload;

#define RED_TEAM 0                                                          
#define BLUE_TEAM 1                                                     
#define RESET 2 
#define STATUS 3

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     3 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


int SCL_PIN = D1; // D1 - Not used in code.  Just for reference when connecting pins to Display.
int SDA_PIN = D2; //D2 - Not used in code.  Just for reference when connecting pins to Display.
int redTeamPin = 12; //D6
int blueTeamPin = 13; //D7
int resetPin = 15; //D8

String baseUrl = "http://nerf-data-api-dfw.herokuapp.com";
String urlRedTeam = "/koth/startTimer/Red";
String urlBlueTeam = "/koth/startTimer/Blue";
String urlReset = "/koth/reset";
String urlStatus = "/koth/status";

const char* ssid     = "SSID";
const char* password = "PW";

WiFiClient client;
HTTPClient http;
void setup() {
  Serial.begin(115200);
  delay(10);

  ///////////////RF24 Setup/////////////////
  Serial.println("Testing radio");
  myRadio.begin();  
  myRadio.setChannel(115); 
  myRadio.setPALevel(RF24_PA_MAX);
  myRadio.setDataRate( RF24_250KBPS ) ; 
  myRadio.openWritingPipe( addresses[0]);
  delay(1000);
  /////////////////////////////////////////

  pinMode(redTeamPin, INPUT_PULLUP);
  pinMode(blueTeamPin, INPUT_PULLUP);
  pinMode(resetPin, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();
  
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  String preConnectionDisplayText = "Connecting: " + String(ssid);
  display.println(preConnectionDisplayText);
  display.display();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    preConnectionDisplayText += ".";
    display.println(preConnectionDisplayText);
    display.display();
  }
  
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
//  DiscoverBaseUrl();
}

// This is a hack for my phone's hotspot connection as the arduino needs to find whatever
// the IP my phone decided to use at this moment in time.
void DiscoverBaseUrl () {
  http.setTimeout(100);
  for (int i = 0; i < 255; i++) {    
    baseUrl = "http://192.168.43." + String(i) + ":3000";
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Testing:");    
    display.setCursor(0,20);
    display.println(String(baseUrl));
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.display();
    http.begin(baseUrl);
    int httpCode = http.GET();
    if (httpCode == 200) {
      http.setTimeout(500);
      Serial.println("Real endpont is: " + String(baseUrl));
      return;
    }
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Unable to find API Server.  Power Cycle and retry later.");
  display.display();
  delay(99999999);
}

int value = 0;

void loop() {
  int _redTeamPin = digitalRead(redTeamPin);
  int _blueTeamPin = digitalRead(blueTeamPin);
  int _resetPin = digitalRead(resetPin);
  
  if (_redTeamPin == LOW) {          
    processGetRequest(RED_TEAM);
  } else if (_blueTeamPin == LOW) {          
    processGetRequest(BLUE_TEAM);
  } /*else if (_resetPin == LOW) {          
    processGetRequest(RESET);
  }*/
  processGetRequest(STATUS);
}

void processGetRequest ( int action ) {
  String endpoint = "";
  // We now create a URI for the request
  switch (action)
  {
    case RED_TEAM:
      endpoint = urlRedTeam;
      break;
    case BLUE_TEAM:
      endpoint = urlBlueTeam;
      break;
    case RESET:
      endpoint = urlReset;
      break;
    case STATUS:
      endpoint = urlStatus;
      break;
    default:
      endpoint = "";
      break;
  }
  String fullUrl = baseUrl + endpoint;
  Serial.println("Processing: " + String(action));
  Serial.println("URL: " + String(fullUrl));
  http.begin(fullUrl);
  int httpCode = http.GET();
  if (httpCode == 200) {
    Serial.println(http.getString());
    if (action == STATUS) {
      displayStatus(http.getString());
    }
  }  

  Serial.println(httpCode);
  Serial.println("closing connection");
}

void displayStatus (String jsonVal) {
  const size_t capacity = JSON_ARRAY_SIZE(4) + JSON_OBJECT_SIZE(1) + 4*JSON_OBJECT_SIZE(4) + 330;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& root = jsonBuffer.parseObject(jsonVal);
  
  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  JsonArray& Teams = root["Teams"];
  JsonObject& Teams_0 = Teams[0];
  const char* Teams_0_teamName = Teams_0["teamName"]; // "Red"
  bool Teams_0_isActive = Teams_0["isActive"]; // false
  int Teams_0_elapsedTimeInSeconds = Teams_0["elapsedTimeInSeconds"]; // 0
  
  JsonObject& Teams_1 = Teams[1];
  const char* Teams_1_teamName = Teams_1["teamName"]; // "Blue"
  bool Teams_1_isActive = Teams_1["isActive"]; // false
  int Teams_1_elapsedTimeInSeconds = Teams_1["elapsedTimeInSeconds"]; // 0

  display.clearDisplay();
//  display.setCursor(0,18);
//  display.setTextSize(2);
//  display.setTextColor(WHITE);
//  display.println("Red: " + String(Teams_0_elapsedTimeInSeconds));
//  display.println("Blue: " + String(Teams_1_elapsedTimeInSeconds));
  display.setCursor(36,0);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.println("CONNECTED");
  display.setTextSize(2);
  display.setCursor(0,16);
  display.println(" RED  BLUE");
  display.setCursor(10,32);
  display.print(String(Teams_0_elapsedTimeInSeconds));
  display.setCursor(72,32);
  display.print(String(Teams_1_elapsedTimeInSeconds));
  display.display();
}
