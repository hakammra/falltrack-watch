#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <MPU6050IMU.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>

#define EEPROM_SIZE 96
#define MAIN_DEVICE_ID_SIZE 32

// Add these new variables for AP control
bool shouldKeepAP = false;
unsigned long wifiConnectTime = 0;
const unsigned long AP_KEEP_DURATION = 5000; // Keep AP for 30 seconds after connection


#define BUTTON_PIN 14

unsigned long buttonPressStart = 0;
bool buttonPreviouslyPressed = false;
const unsigned long RESET_HOLD_DURATION = 3000; // 3 seconds


const unsigned long HOLD_TIME = 3000;      // 3 seconds
const unsigned long DOUBLE_PRESS_WINDOW = 500;

// OLED and sensor definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define BATTERY_PIN A0
#define BUZZER_PIN 12
#define LED_PIN 2

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MAX30105 particleSensor;
MPU6050IMU imu;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // GMT+5:30

const float MAX_VOLTAGE = 4.2;
const float MIN_VOLTAGE = 3.3;
const float VOLTAGE_DIVIDER_RATIO = 0.5;
const char* firebase_host = "https://falltrack-default-rtdb.firebaseio.com/";
const char* device_id = "B7K4Z2";  // this will be dynamic later

// Fall detection and heartbeat
bool inFreeFall = false;
bool fallDetected = false;
unsigned long lastFallAlert = 0;
const unsigned long FALL_ALERT_DURATION = 10000;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg = 0;
bool fingerDetected = false;
bool pulseFrame = false;
unsigned long lastPulseToggle = 0;

unsigned long lastDisplayUpdate = 0;
const int displayUpdateInterval = 200;

const char* daysOfWeek[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

unsigned long lastFingerDetectedTime = 0;
const unsigned long INACTIVITY_TIMEOUT = 5000;
bool systemActive = true;

// WiFi setup
ESP8266WebServer Server(80);
bool credentialsReceived = false;
String receivedSSID = "";
String receivedPassword = "";

void saveCredentialsToEEPROM(const String& ssid, const String& pass) {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < ssid.length(); i++) EEPROM.write(i, ssid[i]);
  EEPROM.write(ssid.length(), '|');  // delimiter
  for (int i = 0; i < pass.length(); i++) EEPROM.write(ssid.length() + 1 + i, pass[i]);
  EEPROM.write(ssid.length() + 1 + pass.length(), '\0');
  EEPROM.commit();
}

void handleButtonPress() {
  static bool buttonPressed = false;
  static unsigned long pressStartTime = 0;
  static int shortPressCount = 0;
  static unsigned long firstShortPressTime = 0;

  bool currentState = digitalRead(BUTTON_PIN);

  if (currentState && !buttonPressed) {
    // Button just pressed
    pressStartTime = millis();
    buttonPressed = true;
    Serial.println("Button just pressed");
  }

  if (!currentState && buttonPressed) {
    // Button just released
    unsigned long pressDuration = millis() - pressStartTime;
    buttonPressed = false;
    Serial.println("Button just released");
    Serial.println(pressDuration);
    

    if (pressDuration < 2000) {
      // 🟠 Short press
      if (shortPressCount == 0) firstShortPressTime = millis();
      shortPressCount++;

      Serial.println("🔘 Short press detected");

      // 🛑 Silencing buzzer if fall is active
      if (fallDetected && digitalRead(BUZZER_PIN) == HIGH) {
        Serial.println("🔇 Buzzer silenced by button");
        fallDetected = false;
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(LED_PIN, LOW);

        // 🖥️ Show on OLED
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 20);
        display.println("Fall alert cleared");
        display.display();

        return;  // Do not process restart/reset after silencing
      }

      // 🔁 Restart if two presses in 2s
      if (shortPressCount >= 2 && millis() - firstShortPressTime <= 4000) {
        Serial.println("🔁 Restart triggered by 2 short presses");

        // 🖥️ OLED feedback
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 20);
        display.println("Restarting...");
        display.display();

        delay(100);
        ESP.restart();
      }
    }
  }

  // Clear short press counter after timeout
  if (millis() - firstShortPressTime > 2000) {
    shortPressCount = 0;
  }

  // 🧼 Long hold to reset WiFi
  if (currentState && buttonPressed) {
    unsigned long heldDuration = millis() - pressStartTime;

    if (heldDuration >= 6000) {
      Serial.println("🧹 Long press - resetting WiFi credentials");

      // 🖥️ OLED feedback
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 20);
      display.println("Resetting WiFi...");
      display.display();

      EEPROM.begin(EEPROM_SIZE);
      for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
      EEPROM.commit();
      delay(100);
      ESP.restart();
    }
  }
}



bool loadCredentialsFromEEPROM(String &ssid, String &pass) {
  EEPROM.begin(EEPROM_SIZE);
  String data = "";
  for (int i = 0; i < EEPROM_SIZE; i++) {
    char c = EEPROM.read(i);
    if (c == '\0') break;
    data += c;
  }
  int sep = data.indexOf('|');
  if (sep != -1) {
    ssid = data.substring(0, sep);
    pass = data.substring(sep + 1);
    return true;
  }
  return false;
}

void sendFallAlertToFirebase() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected - can't send alert");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String path = String(firebase_host) + "devices/" + device_id + "/alerts.json";
  Serial.println("Attempting to connect to: " + path);
  
  if (http.begin(client, path)) {
    http.addHeader("Content-Type", "application/json");

    time_t rawTime = timeClient.getEpochTime();
    struct tm * ti = localtime(&rawTime);
    char timeBuf[25];
    snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02dT%02d:%02d:%02d",
             ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
             ti->tm_hour, ti->tm_min, ti->tm_sec);

    String body = "{\"timestamp\": \"" + String(timeBuf) + "\", \"type\": \"fall\"}";
    Serial.println("Sending: " + body);

    int statusCode = http.POST(body);
    String response = http.getString();
    Serial.print("Status: "); Serial.println(statusCode);
    Serial.print("Response: "); Serial.println(response);
    
    http.end();
  } else {
    Serial.println("Connection failed");
  }
}

void setupAP() {
  WiFi.mode(WIFI_AP_STA); // Changed to AP+STA mode
  WiFi.softAP("FallTrack-Setup", "12345678");
  Serial.println("AP Mode Started: FallTrack-Setup");

  Server.on("/wifi", HTTP_POST, []() {
    if (Server.hasArg("plain")) {
      String body = Server.arg("plain");
      int sep = body.indexOf(',');
      if (sep > 0) {
        receivedSSID = body.substring(0, sep);
        receivedPassword = body.substring(sep + 1);
        credentialsReceived = true;
        saveCredentialsToEEPROM(receivedSSID, receivedPassword);
        Server.send(200, "text/plain", "Credentials received");
        Serial.println("✅ Credentials: " + receivedSSID);
      } else {
        Server.send(400, "text/plain", "Invalid format");
      }
    } else {
      Server.send(400, "text/plain", "Missing body");
    }
  });

  Server.begin();
}

void connectToWiFi(const String& ssid, const String& pass) {
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.print("Connecting to WiFi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connected to " + ssid);
    Serial.println("IP Address: " + WiFi.localIP().toString());
    timeClient.begin();
    
    // Set flag to keep AP running temporarily
    shouldKeepAP = true;
    wifiConnectTime = millis();
  } else {
    Serial.println("\n❌ Failed to connect");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BATTERY_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(4, 5); // I2C for OLED and sensors
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Try to load stored WiFi credentials
  String storedSSID, storedPass;
  if (loadCredentialsFromEEPROM(storedSSID, storedPass)) {
    Serial.println("Loaded credentials from EEPROM");
    Serial.print("SSID: "); Serial.println(storedSSID);
    
    // Attempt to connect to stored WiFi
    connectToWiFi(storedSSID, storedPass);
    
    // If connection fails, start AP mode
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Stored credentials failed - starting AP mode");
      setupAP();
    }
  } else {
    // No stored credentials, start AP mode immediately
    Serial.println("No stored credentials - starting AP mode");
    setupAP();
  }
  Server.on("/reset", HTTP_POST, []() {
    for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
    EEPROM.commit();
    Server.send(200, "text/plain", "Device reset complete");
    ESP.restart();
  });

  Server.on("/get-wifi", HTTP_GET, []() {
    String ssid, pass;
    if (loadCredentialsFromEEPROM(ssid, pass)) {
      Server.send(200, "application/json", 
        "{\"ssid\":\"" + ssid + "\",\"saved\":true}");
    } else {
      Server.send(200, "application/json", "{\"saved\":false}");
    }
  });
  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.display();
  delay(500);

  // Initialize MAX30105 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 not found");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  // Initialize IMU
  imu.begin();

  // Setup web server endpoints
  Server.on("/", HTTP_GET, []() {
    Server.send(200, "text/plain", "FallTrack Device Ready");
  });

  Server.on("/wifi", HTTP_POST, []() {
    if (Server.hasArg("plain")) {
      String body = Server.arg("plain");
      int sep = body.indexOf(',');
      if (sep > 0) {
        receivedSSID = body.substring(0, sep);
        receivedPassword = body.substring(sep + 1);
        credentialsReceived = true;
        saveCredentialsToEEPROM(receivedSSID, receivedPassword);
        Server.send(200, "text/plain", "Credentials received");
        Serial.println("Credentials received: " + receivedSSID);
      } else {
        Server.send(400, "text/plain", "Invalid format");
      }
    } else {
      Server.send(400, "text/plain", "Missing body");
    }
  });

  // Add connection status endpoint
  Server.on("/connection-status", HTTP_GET, []() {
    String status;
    if (WiFi.status() == WL_CONNECTED) {
      status = "{\"connected\":true,\"ssid\":\"" + WiFi.SSID() + "\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
    } else {
      status = "{\"connected\":false}";
    }
    Server.send(200, "application/json", status);
  });

  Server.onNotFound([]() {
    Server.send(404, "text/plain", "Not found");
  });

  // Start server
  Server.begin();
  Serial.println("HTTP server started");

  // Initialize NTP client if connected to WiFi
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.begin();
    timeClient.update();
    Serial.println("NTP client initialized");
  }

  Serial.println("System ready");
}

void loop() {
  handleButtonPress();
  // Check if we should disable AP
  if (shouldKeepAP && (millis() - wifiConnectTime > AP_KEEP_DURATION)) {
    WiFi.softAPdisconnect(true);
    shouldKeepAP = false;
    Serial.println("AP mode disabled");
  }

  if (!credentialsReceived) {
    Server.handleClient();
  } else {
    credentialsReceived = false;
    connectToWiFi(receivedSSID, receivedPassword);
  }

  timeClient.update();

  long irValue = particleSensor.getIR();
  fingerDetected = irValue > 50000;

  if (fingerDetected) {
    lastFingerDetectedTime = millis();
    if (!systemActive) {
      systemActive = true;
      inFreeFall = false;
      fallDetected = false;
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
    }
  } else if (millis() - lastFingerDetectedTime > INACTIVITY_TIMEOUT) {
    systemActive = false;
    updateDisplay();
    delay(100);
    return;
  }

  if (systemActive) {
    checkForFall();

    if (fingerDetected) {
      if (checkForBeat(irValue)) {
        long delta = millis() - lastBeat;
        lastBeat = millis();
        beatsPerMinute = 60 / (delta / 1000.0);
        if (beatsPerMinute > 20 && beatsPerMinute < 255) {
          rates[rateSpot++] = (byte)beatsPerMinute;
          rateSpot %= RATE_SIZE;
          beatAvg = 0;
          for (byte x = 0; x < RATE_SIZE; x++) beatAvg += rates[x];
          beatAvg /= RATE_SIZE;
        }
      }
    } else {
      beatAvg = 0;
      rateSpot = 0;
      for (byte x = 0; x < RATE_SIZE; x++) rates[x] = 0;
    }

    if (millis() - lastPulseToggle > 500) {
      pulseFrame = !pulseFrame;
      lastPulseToggle = millis();
    }

    if (millis() - lastDisplayUpdate >= displayUpdateInterval) {
      updateDisplay();
      lastDisplayUpdate = millis();
    }
  }
}

void checkForFall() {
  if (!systemActive) return;

  float acc[3], gyro[3];
  imu.readaccelerometer(acc, 8);
  imu.readgyroscope(gyro, 8);

  float accMag = sqrt(acc[0]*acc[0] + acc[1]*acc[1] + acc[2]*acc[2]);
  float gyroMag = sqrt(gyro[0]*gyro[0] + gyro[1]*gyro[1] + gyro[2]*gyro[2]);

  Serial.print("Accel: ");
  imu.readaccelerometer(acc, 8);
  Serial.print(acc[0]); Serial.print(", ");
  Serial.print(acc[1]); Serial.print(", ");
  Serial.print(acc[2]); Serial.print(" | Gyro: ");
  
  imu.readgyroscope(gyro, 8);
  Serial.print(gyro[0]); Serial.print(", ");
  Serial.print(gyro[1]); Serial.print(", ");
  Serial.println(gyro[2]);

  if (!inFreeFall && accMag < 3.0) inFreeFall = true;

 if (inFreeFall && accMag > 15.0 && gyroMag > 300.0) {
  fallDetected = true;
  inFreeFall = false;
  lastFallAlert = millis();
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("⚠️ FALL DETECTED!");

  sendFallAlertToFirebase();  // 🔔 send alert to Firebase here
}

  if (fallDetected && (millis() - lastFallAlert > FALL_ALERT_DURATION)) {
    fallDetected = false;
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
  }
}

float getBatteryVoltage() {
  int raw = analogRead(BATTERY_PIN);
  return (raw / 1023.0) * 3.3 / VOLTAGE_DIVIDER_RATIO;
}

int getBatteryPercentage() {
  float voltage = getBatteryVoltage();
  voltage = constrain(voltage, MIN_VOLTAGE, MAX_VOLTAGE);
  return map(voltage * 100, MIN_VOLTAGE * 100, MAX_VOLTAGE * 100, 0, 100);
}

void drawBatteryIcon(int x, int y, int percent) {
  display.drawRect(x, y, 18, 8, WHITE);
  display.fillRect(x + 18, y + 2, 2, 4, WHITE);
  int fillWidth = map(percent, 0, 100, 0, 16);
  display.fillRect(x + 1, y + 1, fillWidth, 6, WHITE);
}

void drawMobileDataIcon(int x, int y, bool connected) {
  if (connected) {
    display.fillRect(x, y + 6, 3, 8, WHITE);
    display.fillRect(x + 5, y + 10, 3, 4, WHITE);
    display.fillRect(x + 10, y + 12, 3, 2, WHITE);
  } else {
    display.drawLine(x, y + 6, x + 13, y + 16, WHITE);
    display.drawLine(x, y + 16, x + 13, y + 6, WHITE);
  }
}

void updateDisplay() {
  if (!systemActive) {
    display.clearDisplay();
    display.display();
    return;
  }

  display.clearDisplay();

  time_t rawTime = timeClient.getEpochTime();
  struct tm *ti = localtime(&rawTime);

  display.setTextSize(2);
  display.setCursor(0, 20);
  display.printf("%02d:%02d", ti->tm_hour, ti->tm_min);

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.printf("%02d/%02d", ti->tm_mday, ti->tm_mon + 1);

  display.setCursor(0, 55);
  display.print(daysOfWeek[ti->tm_wday]);

  int heartX = 85;
  if (pulseFrame) {
    display.fillCircle(heartX + 6, 15, 4, WHITE);
    display.fillCircle(heartX + 14, 15, 4, WHITE);
    display.fillTriangle(heartX, 16, heartX + 20, 16, heartX + 10, 32, WHITE);
  } else {
    display.fillCircle(heartX + 6, 16, 2, WHITE);
    display.fillCircle(heartX + 14, 16, 2, WHITE);
    display.fillTriangle(heartX + 2, 17, heartX + 18, 17, heartX + 10, 28, WHITE);
  }

  display.setTextSize(3);
  display.setCursor(heartX, 40);
  if (fingerDetected && beatAvg > 0) display.print(beatAvg);
  else display.print("--");

  if (fallDetected) {
    display.setTextSize(1);
    display.setCursor(50, 0);
    display.print("FALL!");
  }

  int batteryPercent = getBatteryPercentage();
  drawBatteryIcon(0, 5, batteryPercent);
  drawMobileDataIcon(25, 0, WiFi.status() == WL_CONNECTED);

  display.display();
}

