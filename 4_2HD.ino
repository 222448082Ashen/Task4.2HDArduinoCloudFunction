#include <WiFiNINA.h>
#include <Arduino_JSON.h>

// WiFi credentials
const char* ssid = "RHD32";
const char* password = "12345678";

// Firebase URL and host
const char* firebaseHost = "traffic-light-1a02f-default-rtdb.firebaseio.com";

// Firebase endpoint
const char* endpoint = "/LEDS.json";

// Pins for LEDs
#define RED_LED_PIN 2
#define GREEN_LED_PIN 3
#define BLUE_LED_PIN 4

WiFiSSLClient client;

// Timer variables
unsigned long redTimer = 0;
unsigned long greenTimer = 0;
unsigned long blueTimer = 0;
unsigned long redStartTime = 0;
unsigned long greenStartTime = 0;
unsigned long blueStartTime = 0;

// Previous state variables
bool previousRedState = LOW;
bool previousGreenState = LOW;
bool previousBlueState = LOW;
int previousRedTimer = 0;
int previousGreenTimer = 0;
int previousBlueTimer = 0;

void setup() {
  Serial.begin(9600);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  // Connect to WiFi
  connectToWiFi();
}
//main loop which executes the code
void loop() {
  unsigned long currentMillis = millis();

  // Fetch data from Firebase
  if (fetchDataFromFirebase(currentMillis)) {
    // Process and update LEDs based on fetched data
    
  }
  
  // Handle timers
  handleTimers(currentMillis);

  // Add a delay to avoid making too many requests
  delay(1000); // 10 seconds
}
//for connecting to the wifi
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");
}

bool fetchDataFromFirebase(unsigned long currentMillis) {
  Serial.print("Connecting to Firebase...");
  if (client.connect(firebaseHost, 443)) { // Use port 443 for HTTPS
    Serial.println("Connected!");

    // Make the HTTPS GET request
    client.print(String("GET ") + endpoint + " HTTP/1.1\r\n" +
                 "Host: " + firebaseHost + "\r\n" +
                 "Connection: close\r\n\r\n");

    // Wait for response
    while (client.connected() && !client.available()) {
      delay(100);
    }

    // Read the response
    String payload;
    while (client.available()) {
      payload += client.readString();
    }

    client.stop();

    // Print the entire HTTP response
    Serial.println("Received payload:");
    Serial.println(payload);

    // Extract JSON from HTTP response
    int jsonStart = payload.indexOf("\r\n\r\n") + 4;
    if (jsonStart < 4) {
      Serial.println("Invalid JSON start position");
      return false;
    }
    String json = payload.substring(jsonStart);

    Serial.println("Extracted JSON:");
    Serial.println(json);

    // Parse JSON
    JSONVar data = JSON.parse(json);
    if (JSON.typeof(data) == "undefined") {
      Serial.println("Parsing JSON failed!");
      return false;
    }

    // Update LED states and timers
    JSONVar blueData = data["blue"];
    JSONVar greenData = data["green"];
    JSONVar redData = data["red"];
    Serial.println(blueData);

    // Update LED states and timers only if they have changed
    bool currentBlueState = bool(blueData["state"]);
    bool currentGreenState = bool(greenData["state"]);
    bool currentRedState = bool(redData["state"]);
     
    String tempBlue = blueData["timer"];
    int currentBlueTimer = tempBlue.toInt();
    String tempGreen = greenData["timer"];
    int currentGreenTimer = tempGreen.toInt();
    String tempRed = redData["timer"];
    int currentRedTimer = tempRed.toInt();
    
    //only update if there is a state change
    if (currentBlueState != previousBlueState) {
      digitalWrite(BLUE_LED_PIN, currentBlueState ? HIGH : LOW);
      previousBlueState = currentBlueState;
    }
    if (currentGreenState != previousGreenState) {
      digitalWrite(GREEN_LED_PIN, currentGreenState ? HIGH : LOW);
      previousGreenState = currentGreenState;
    }
    if (currentRedState != previousRedState) {
      digitalWrite(RED_LED_PIN, currentRedState ? HIGH : LOW);
      previousRedState = currentRedState;
    }

    // Only update timers if the timer value changes and is non-zero
    if (currentBlueTimer != previousBlueTimer && currentBlueTimer > 0) {
      setTimer("blue", currentBlueTimer, currentMillis);
      previousBlueTimer = currentBlueTimer;
    }
    if (currentGreenTimer != previousGreenTimer && currentGreenTimer > 0) {
      setTimer("green", currentGreenTimer, currentMillis);
      previousGreenTimer = currentGreenTimer;
    }
    if (currentRedTimer != previousRedTimer && currentRedTimer > 0) {
      setTimer("red", currentRedTimer, currentMillis);
      previousRedTimer = currentRedTimer;
    }

    return true;
  } else {
    Serial.println("Connection to Firebase failed!");
    return false;
  }
}
//totgling LED state
void toggleLED(String color) {
  if (color == "red") {
    digitalWrite(RED_LED_PIN, !digitalRead(RED_LED_PIN));
    Serial.println("Red LED toggled");
    updateFirebaseState("red", digitalRead(RED_LED_PIN));
  } else if (color == "green") {
    digitalWrite(GREEN_LED_PIN, !digitalRead(GREEN_LED_PIN));
    Serial.println("Green LED toggled");
    updateFirebaseState("green", digitalRead(GREEN_LED_PIN));
  } else if (color == "blue") {
    digitalWrite(BLUE_LED_PIN, !digitalRead(BLUE_LED_PIN));
    Serial.println("Blue LED toggled");
    updateFirebaseState("blue", digitalRead(BLUE_LED_PIN));
  }
}
//setting up timers
void setTimer(String color, int timerValue, unsigned long currentMillis) {
  if (color == "red") {
    redTimer = timerValue * 1000;
    redStartTime = currentMillis;
  } else if (color == "green") {
    greenTimer = timerValue * 1000;
    greenStartTime = currentMillis;
  } else if (color == "blue") {
    blueTimer = timerValue * 1000;
    blueStartTime = currentMillis;
  }
  Serial.print(color);
  Serial.print(" timer set to ");
  Serial.println(timerValue);
}
//handling the timers set
void handleTimers(unsigned long currentMillis) {
  if (redTimer > 0 && currentMillis - redStartTime >= redTimer) {
    toggleLED("red");
    redTimer = 0; // Stop the timer
  }

  if (greenTimer > 0 && currentMillis - greenStartTime >= greenTimer) {
    toggleLED("green");
    greenTimer = 0; // Stop the timer
  }

  if (blueTimer > 0 && currentMillis - blueStartTime >= blueTimer) {
    toggleLED("blue");
    blueTimer = 0; // Stop the timer
  }
}


//update firebase database when timer is over
void updateFirebaseState(String color, bool state) {
  String sestate;
  Serial.print("Updating Firebase for ");
  Serial.print(color);
  Serial.print(" state to ");
  Serial.println(state);
  if (state == 0 ){
    sestate = "false";
  }else {
    sestate = "true";
  }
  

  if (client.connect(firebaseHost, 443)) {
    String path = "/LEDS/" + color + ".json";
    String payload = "{\"state\":" + sestate + ",\"timer\":0}"; // Reset timer to 0 if state is updated to false

    // Make the HTTPS PUT request
    client.print(String("PUT ") + path + " HTTP/1.1\r\n" +
                    "Host: " + firebaseHost + "\r\n" +
                    "Connection: close\r\n" +
                    "Content-Length: " + payload.length() + "\r\n" +
                    "Content-Type: application/json\r\n\r\n" +
                    payload + "\r\n");


    // Wait for response
    while (client.connected() && !client.available()) {
      delay(100);
    }

    // Read the response
    while (client.available()) {
      String response = client.readString();
      Serial.println("Response:");
      Serial.println(response);
    }



    client.stop();
  } else {
    Serial.println("Connection to Firebase failed!");
  }
}
