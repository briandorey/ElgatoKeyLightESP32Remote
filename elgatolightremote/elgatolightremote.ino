#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>

const char WIFI_SSID[] = "YourWIFIUsername";
const char WIFI_PASSWORD[] = "YourWIFIPassword";

String ELGATO_KEY_LIGHT_1_HOST_NAME = "elgato-key-light-name";

// use pin 32 for switch
const int BUTTON_PIN = 32;
HTTPClient http;

int CurrentLightState = 0;

// Variables will change:
int lastState = HIGH;  // the previous state from the input pin
int currentState;      // the current reading from the input pin

IPAddress serverIp1;


void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  while(!MDNS.begin("esp32Host")) {
     Serial.println("Starting mDNS...");
     delay(1000);
  }
 
  Serial.println("MDNS started");  

  while (serverIp1.toString() == "0.0.0.0") {
    Serial.println("Resolving host...");
    delay(250);
    serverIp1 = MDNS.queryHost(ELGATO_KEY_LIGHT_1_HOST_NAME);
  }
  Serial.println("Host address resolved:");
  Serial.println(serverIp1.toString());   
}
void loop() {
  currentState = digitalRead(BUTTON_PIN);

  if (lastState == LOW && currentState == HIGH) {
    Serial.println("The state changed from LOW to HIGH");
    if (CurrentLightState == 1) {
 LightOff(serverIp1.toString());
    } else {
 LightOn(serverIp1.toString());
    }
  }


  // save the last state
  lastState = currentState;
}

void LightOn(String ip) {
    HTTPClient http;   
    http.begin("http://" + ip + ":9123/elgato/lights");  
    http.addHeader("Content-Type", "application/json");
    String requestBody = "{\"numberOfLights\":1,\"lights\":[{\"on\":1,\"brightness\":100,\"temperature\":206}]}";
    int httpResponseCode = http.PUT(requestBody);
    if(httpResponseCode>0){
       
      String response = http.getString();                       
       
      Serial.println(httpResponseCode);   
      Serial.println(response);
    }
    else {
     
      Serial.printf("Error occurred while sending HTTP POST:");
    }
    http.end();
    CurrentLightState = 1;
}
void LightOff(String ip) {
  HTTPClient http;   
    http.begin("http://" + ip + ":9123/elgato/lights");  
    http.addHeader("Content-Type", "application/json");
    String requestBody = "{\"numberOfLights\":1,\"lights\":[{\"on\":0,\"brightness\":100,\"temperature\":206}]}";
    int httpResponseCode = http.PUT(requestBody);

    if(httpResponseCode>0){
       
      String response = http.getString();                       
       
      Serial.println(httpResponseCode);   
      Serial.println(response);
    }
    else {
     
      Serial.printf("Error occurred while sending HTTP POST:");
    }
    http.end();
    CurrentLightState = 0;
}
