#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

/*
*   Control Elgato keylight using ESP32 WiFi module with push buttons and LED indictors
*
*   Requires the following libaries: ESPmDNS
*   ArduinoJson https://arduinojson.org/
*
*   See blog post on https://www.briandorey.com/post/esp32-three-elgato-key-light-remote for more information
*/

// set WiFi network ssid and password
const char WIFI_SSID[] = "yourWifiSSID";
const char WIFI_PASSWORD[] = "yourWifiPassword";


// crete struct with variables for light settings.
struct elgatolight{
   String hostname;
   IPAddress ipaddress;
   int currentstate;
   int brightness;
   int temperature;
   int ledpin;
   int buttonpin;
   int buttonprev;
   int buttonState;
};

// Create elgatolight struct with host name, default IP address, current state, brightness and colour temperature, led pin, button pin, buttonprev state, buttonstate
// obtain hostname in windows using dns-sd from a command prompt: dns-sd -B _elg._tcp
// the instance name is the host name of your light in lower case and replace spaces with a dash
elgatolight LightLeft = {"elgato-key-light-mk-2-1234", (0,0,0,0), 0, 100, 206, 21, 32, 0, 0}; // Left light and button

elgatolight LightMiddle = {"elgato-key-light-5678", (0,0,0,0), 0, 100, 206, 4, 33, 0, 0}; // Middle light and button

elgatolight LightRight = {"elgato-key-light-mk-2-9101", (0,0,0,0), 0, 100, 206, 2, 25, 0, 0}; // Right light and button

HTTPClient http;

void setup() {
  // Initialize serial console and pin mode to input with pullup enabled
  Serial.begin(9600);
  


  
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

  SetupLight(&LightLeft);
  SetupLight(&LightMiddle);
  SetupLight(&LightRight);
  

}

// main loop
void loop() {
  CheckLightButton(&LightLeft);
  CheckLightButton(&LightMiddle);
  CheckLightButton(&LightRight);
}

/*
*   CheckLightButton function.
*   Variables:
*   struct elgatolight
*/
void CheckLightButton(struct elgatolight *LIGHT) {

 
  // Get button state for button
  LIGHT->buttonState = digitalRead(LIGHT->buttonpin);
  if (LIGHT->buttonState == LOW && LIGHT->buttonprev == HIGH) {
    Serial.println("Click Detected");
     if (LIGHT->ipaddress.toString() != "0.0.0.0") {
      // get current light settings
      GetLightSettings(LIGHT);
      //toggle light on or off
      ToggleLight(LIGHT, 1);
    } else {
      // light not found, run dns lookup again
      digitalWrite(LIGHT->ledpin, LOW); 
      delay(100);
      digitalWrite(LIGHT->ledpin, HIGH); 
      delay(100);
      digitalWrite(LIGHT->ledpin, LOW); 
      delay(100);
      digitalWrite(LIGHT->ledpin, HIGH); 
      delay(100);
      digitalWrite(LIGHT->ledpin, LOW); 
      delay(100);
      digitalWrite(LIGHT->ledpin, HIGH); 
      delay(100);
      digitalWrite(LIGHT->ledpin, LOW); 
      SetupLight(LIGHT);
    }
  }
  LIGHT->buttonprev = digitalRead(LIGHT->buttonpin);
  // End button state for button
  
}



/*
*   SetupLight function.
*   Variables:
*   struct elgatolight
*/
void SetupLight(struct elgatolight *LIGHT) {
  // loop to try to obtain IP address for light using hostname

  // setup button and led
    pinMode(LIGHT->buttonpin, INPUT_PULLUP);
    pinMode(LIGHT->ledpin, OUTPUT);
  int dns_retries = 0;

  while (LIGHT->ipaddress.toString() == "0.0.0.0" && dns_retries < 2) {
    Serial.println("Resolving host " + LIGHT->hostname);
    delay(250);
    dns_retries ++;
    LIGHT->ipaddress = MDNS.queryHost(LIGHT->hostname);
  }
  // if DNS found
  if (LIGHT->ipaddress.toString() != "0.0.0.0") {
    Serial.println("Host address resolved:");
    Serial.println(LIGHT->ipaddress.toString());   

   
    // Get settings from light
    GetLightSettings(LIGHT);
    // print settings to console
    Serial.println("hostname: " + LIGHT->hostname);
    Serial.println("currentstate: " + String(LIGHT->currentstate));
    Serial.println("brightness: " + String(LIGHT->brightness));
    Serial.println("temperature: " + String(LIGHT->temperature));
    Serial.println("ipaddress: " + LIGHT->ipaddress.toString());
  } else {
    Serial.println("Failed to resolve DNS for: " + LIGHT->hostname);
  }

}
/*
*   ToggleLight function.
*   Variables:
*   struct elgatolight
*   int Retries, default value 1
*/
void ToggleLight(struct elgatolight *LIGHT, int Retries) {
    if (LIGHT->ipaddress.toString() != "0.0.0.0") {
    if (LIGHT->currentstate == 1) {
      LIGHT->currentstate = 0;
      digitalWrite(LIGHT->ledpin, LOW); 
    } else {
      LIGHT->currentstate = 1;
      digitalWrite(LIGHT->ledpin, HIGH); 
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
            ToggleLight(&LightMiddle, Retries);
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
}

/*
*   GetLightSettings function.
*   Variables:
*   struct elgatolight
*/
void GetLightSettings(struct elgatolight *LIGHT) {
  if (LIGHT->ipaddress.toString() != "0.0.0.0") {
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
          if (currentOn == 1) {
          digitalWrite(LIGHT->ledpin, HIGH); 
          } else {
            digitalWrite(LIGHT->ledpin, LOW); 
          }

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
}