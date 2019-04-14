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
//////////////////////////////////////
#include <SPI.h>
#include "RF24.h"

byte addresses[][6] = {"1Node","2Node"};


/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
bool radioNumber = 1;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(D3,D4);
/**********************************************************/


// Used to control whether this node is sending or receiving
bool role = radioNumber;

/**
* Create a data structure for transmitting and receiving data
* This allows many variables to be easily sent and received in a single transmission
* See http://www.cplusplus.com/doc/tutorial/structures/
*/
struct dataStruct{
  int redTeam = 0;
  int blueTeam = 0;
}myData;
///////////////////////////////////////
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

  Serial.println("STARTING");
  ///////////////RF24 Setup/////////////////
  Serial.println("RF24/examples/GettingStarted_HandlingData");
  Serial.println("*** PRESS 'T' to begin transmitting to the other node");
  
  radio.begin();

  radio.setPALevel(RF24_PA_MAX);           // If you want to save power use "RF24_PA_MIN" but keep in mind that reduces the module's range
  radio.setDataRate(RF24_250KBPS );
  radio.setRetries(15,15);                // Max delay between retries & number of retries
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  if(radioNumber){
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
  }else{
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
  }
  
//  myData.value = 1.22;
  // Start the radio listening for data
  radio.startListening();
  /////////////////////////////////////////

//  pinMode(redTeamPin, INPUT_PULLUP);
//  pinMode(blueTeamPin, INPUT_PULLUP);
//  pinMode(resetPin, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();
  
  // We start by connecting to a WiFi network
  if (role) {
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
    
    DiscoverBaseUrl();
  }
  
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
//  int _redTeamPin = digitalRead(redTeamPin);
//  int _blueTeamPin = digitalRead(blueTeamPin);
//  int _resetPin = digitalRead(resetPin);
  

//  if (_redTeamPin == LOW) {          
//    processGetRequest(RED_TEAM);
//  } else if (_blueTeamPin == LOW) {          
//    processGetRequest(BLUE_TEAM);
//  } /*else if (_resetPin == LOW) {          
//    processGetRequest(RESET);
//  }*/
  if (role) {
    delay(250);
    processGetRequest(STATUS);
  }
  ProcessRadio();
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
  myData.redTeam = Teams_0_elapsedTimeInSeconds;
  
  JsonObject& Teams_1 = Teams[1];
  const char* Teams_1_teamName = Teams_1["teamName"]; // "Blue"
  bool Teams_1_isActive = Teams_1["isActive"]; // false
  int Teams_1_elapsedTimeInSeconds = Teams_1["elapsedTimeInSeconds"]; // 0
  myData.blueTeam = Teams_1_elapsedTimeInSeconds;

//  display.clearDisplay();
//  display.setCursor(36,0);
//  display.setTextColor(WHITE);
//  display.setTextSize(1);
//  display.println("CONNECTED");
//  display.setTextSize(2);
//  display.setCursor(0,16);
//  display.println(" RED  BLUE");
//  display.setCursor(10,32);
//  display.print(String(myData.redTeam));
//  display.setCursor(72,32);
//  display.print(String(myData.blueTeam));
//  display.display();
}

void ProcessRadio () {
  if (role == 1)  {
    
    radio.stopListening();                                    // First, stop listening so we can talk.
    
    delay(250);
    
    Serial.println(String("Now sending"));

//    myData.redTeam = micros();
     if (!radio.write( &myData, sizeof(myData) )){
       Serial.println(String("failed"));
     }
        
    radio.startListening();                                    // Now, continue listening
    
    unsigned long started_waiting_at = micros();               // Set up a timeout period, get the current microseconds
    boolean timeout = false;                                   // Set up a variable to indicate if a response was received or not
    
    while ( ! radio.available() ){                             // While nothing is received
      if (micros() - started_waiting_at > 200000 ){            // If waited longer than 200ms, indicate timeout and exit while loop
          timeout = true;
          break;
      }      
    }
        
    if ( timeout ){                                             // Describe the results
        Serial.println(String("Failed, response timed out."));
    }else{
                                                                // Grab the response, compare, and send to debugging spew
        radio.read( &myData, sizeof(myData) );
        unsigned long time = micros();
        
        // Spew it
//        Serial.print(String("Sent "));
//        Serial.print(time);
//        Serial.print(String(", Got response "));
//        Serial.print(myData.redTeam);
//        Serial.print(String(", Round-trip delay "));
//        Serial.print(time-myData.redTeam);
//        Serial.print(String(" microseconds Value "));
//        Serial.println(myData.value);
        display.clearDisplay();
        display.setCursor(36,0);
        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.println("CONNECTED");
        display.setTextSize(2);
        display.setCursor(0,16);
        display.println(" RED  BLUE");
        display.setCursor(10,32);
        display.print(String(myData.redTeam));
        display.setCursor(72,32);
        display.print("123");
        display.display();
    }

    // Try again 1s later
  }



/****************** Pong Back Role ***************************/

  if ( role == 0 )
  {
    if( radio.available()){
                                                           // Variable for the received timestamp
      while (radio.available()) {                          // While there is data ready
        radio.read( &myData, sizeof(myData) );             // Get the payload
      }
     
      radio.stopListening();                               // First, stop listening so we can talk  
//      myData.value += 0.01;                                // Increment the float value
      radio.write( &myData, sizeof(myData) );              // Send the final one back.      
      radio.startListening();                              // Now, resume listening so we catch the next packets.     
      Serial.println("Sent response " + String(myData.redTeam) + ":" + String(myData.blueTeam));
//      Serial.print(myData.redTeam);  
//      Serial.print(String(" : "));
//      Serial.println(myData.value);
      display.clearDisplay();
      display.setCursor(36,0);
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.println("CONNECTED");
      display.setTextSize(2);
      display.setCursor(0,16);
      display.println(" RED  BLUE");
      display.setCursor(10,32);
      display.print(String(myData.redTeam));
      display.setCursor(72,32);
      display.print(String(myData.blueTeam));
      display.display();
   }
 }




/****************** Change Roles via Serial Commands ***************************/

  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'T' && role == 0 ){      
      Serial.print(String("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK"));
      role = 1;                  // Become the primary transmitter (ping out)
    
   }else
    if ( c == 'R' && role == 1 ){
      Serial.println(String("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK"));      
       role = 0;                // Become the primary receiver (pong back)
       radio.startListening();
       
    }
  }
}
