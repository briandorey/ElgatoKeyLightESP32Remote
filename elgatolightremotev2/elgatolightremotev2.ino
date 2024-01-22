#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

/*
*   Control Elgato keylight using ESP32 WiFi module with push button
*   Version 2.0 code which retreves current light settings for brightness
*   and colour temperature and toggle light on and off for button press events
*   Requires the following libaries: ESPmDNS
*   ArduinoJson https://arduinojson.org/
*/

// set WiFi network ssid and password
const char WIFI_SSID[] = "yourWiFiSSID";
const char WIFI_PASSWORD[] = "yourWifiPassword";

// Setup input button pin and state variables
const int BUTTON_PIN = 32;
int buttonprev = 0;  // variable for previous pushbutton state
int buttonState = 0;  // variable for pushbutton current state


// crete struct with variables for light settings.
struct elgatolight{
   String hostname;
   IPAddress ipaddress;
   int currentstate;
   int brightness;
   int temperature;
};

// Create elgatolight struct with host name, default IP address, current state, brightness and colour temperature
// if you have a fixed IP address, enter below and comment out lines 3 and 57 to 70 for MDNS resolution
elgatolight LightOne = {"elgato-key-light-12345", (0,0,0,0), 0, 100, 206};


HTTPClient http;

void setup() {
  // Initialize serial console and pin mode to input with pullup enabled
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Start Wifi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // Start MDNS service
  while(!MDNS.begin("esp32Host")) {
     Serial.println("Starting mDNS...");
     delay(1000);
  }
 
  Serial.println("MDNS started");  

  while (LightOne.ipaddress.toString() == "0.0.0.0") {
    Serial.println("Resolving host...");
    delay(250);

    LightOne.ipaddress = MDNS.queryHost(LightOne.hostname);
  }
  Serial.println("Host address resolved:");
  Serial.println(LightOne.ipaddress.toString());   

  // Get light settings and assign values to variables
  GetLightSettings(&LightOne);

  Serial.println("LightOne.hostname: " + LightOne.hostname);
  Serial.println("LightOne.currentstate: " + String(LightOne.currentstate));
  Serial.println("LightOne.brightness: " + String(LightOne.brightness));
  Serial.println("LightOne.temperature: " + String(LightOne.temperature));
  Serial.println("LightOne.ipaddress: " + LightOne.ipaddress.toString());
}

// main loop
void loop() {

  // Get button state for button 1
  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && buttonprev == HIGH) {
    Serial.println("Button Click Detected");
    // get current light settings
    GetLightSettings(&LightOne);
    //toggle light on or off
    ToggleLight(&LightOne, 1);
  }
  buttonprev = digitalRead(BUTTON_PIN);
  // End button state for button 1

}
/*
*   ToggleLight function.
*   Variables:
*   struct elgatolight
*   int Retries, default value 1
*/
void ToggleLight(struct elgatolight *LIGHT, int Retries) {
    
    if (LIGHT->currentstate == 1) {
      LIGHT->currentstate = 0;
    } else {
      LIGHT->currentstate = 1;
    }

    HTTPClient http;   
    http.begin("http://" + LIGHT->ipaddress.toString() + ":9123/elgato/lights");  
    http.addHeader("Content-Type", "application/json");
    http.setConnectTimeout(10000);

    String requestBody = "{\"numberOfLights\": 1,\"lights\":[{\"on\":" + String(LIGHT->currentstate) + ",\"brightness\":"
     + String(LIGHT->brightness) + ",\"temperature\":" + String(LIGHT->temperature) + "}]}";
    int httpResponseCode = http.PUT(requestBody);
    if(httpResponseCode>0){
      String response = http.getString();                       
      //Serial.println(httpResponseCode);
      if (httpResponseCode == 200) {
        Serial.println("Command Processed to mode: " + String(LIGHT->currentstate));
      } else {
          Serial.println("Response not 200: " + String(httpResponseCode) + " Retries: " + String(Retries));
          delay(500);  
         
          if (Retries < 5) {
             Retries ++;
            ToggleLight(&LightOne, Retries);
          } else {
            Serial.printf("Error occurred while sending HTTP POST Retry count exceded");
          }
      }
    }
    else {
      Serial.printf("Error occurred while sending HTTP POST:");
    }
    http.end();
}

/*
*   GetLightSettings function.
*   Variables:
*   struct elgatolight
*/
void GetLightSettings(struct elgatolight *LIGHT) {
    Serial.println("http://" + LIGHT->ipaddress.toString() + ":9123/elgato/lights");
    HTTPClient http;   
    http.begin("http://" + LIGHT->ipaddress.toString() + ":9123/elgato/lights");  
    http.addHeader("Content-Type", "application/json");
    http.setConnectTimeout(10000);
    int httpResponseCode = http.GET();
    if(httpResponseCode>0){
      String response = http.getString();                       
      Serial.println(httpResponseCode);
      if (httpResponseCode == 200) {
        Serial.println(response);
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        } else {
          int currentOn = doc["lights"][0]["on"];
          Serial.println("Current On: " + String(currentOn));
          LIGHT->currentstate = currentOn;

          int currentBrightness = doc["lights"][0]["brightness"];
          Serial.println("Current Brightness: " + String(currentBrightness));
          LIGHT->brightness = currentBrightness;

          int currentTemperature = doc["lights"][0]["temperature"];
          Serial.println("Current Temperature: " + String(currentTemperature));
          LIGHT->temperature = currentTemperature;
        }
      }   else {
          Serial.println("Response not 200: " + String(httpResponseCode));
      }
    }
    else {
      Serial.printf("Error occurred GetLightSettings ");
    }
    http.end();
}