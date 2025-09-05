#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>

// OTA Hub Configuration
#define OTAGH_OWNER_NAME "moritzless"
#define OTAGH_REPO_NAME "esp32-ota-project"

// Required OTA defines for the library
#define OTA_SERVER "api.github.com"
#define OTA_PORT 443
#define OTA_CHECK_PATH "/repos/" OTAGH_OWNER_NAME "/" OTAGH_REPO_NAME "/releases/latest"
#define FIRMWARE_BIN_MATCH "firmware.bin"
#define OTA_ASSET_ENDPOINT_CONSTRUCTOR(asset_id) "/repos/" OTAGH_OWNER_NAME "/" OTAGH_REPO_NAME "/releases/assets/" + String(asset_id)

#include <OTA-Hub.hpp>

// WiFi Configuration
const char* ssid = "Moritz WLAN";           
const char* password = "Schnorrer123";     

// Firmware version
const char* FIRMWARE_VERSION = "v1.0.8";

// Update check interval (5 minutes)
const unsigned long updateCheckInterval = 5 * 60 * 1000;
unsigned long lastUpdateCheck = 0;

// LED configuration
#define LED_PIN 20
#define BUILTIN_LED 2
#define LED_RGB 21

// RGB LED setup
Adafruit_NeoPixel rgbLED(1, LED_RGB, NEO_GRB + NEO_KHZ800);

// LED blink timing
unsigned long lastLedBlink = 0;
bool ledState = false;

// RGB LED cycling
unsigned long lastRgbUpdate = 0;
int rgbMode = 0; // 0=blue, 1=green, 2=red, 3=purple, 4=cyan, 5=yellow

WiFiClientSecure wifi_client;

// Function declarations
void connectToWiFi();
void checkForUpdates();
void blinkLED();
void updateRgbLED();

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // Initialize LEDs
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  
  // Initialize RGB LED
  rgbLED.begin();
  rgbLED.setBrightness(50); // Set brightness to 50 out of 255
  rgbLED.setPixelColor(0, rgbLED.Color(0, 0, 255)); // Start with blue
  rgbLED.show();
  
  Serial.println("\n" + String("=").substring(0, 50));
  Serial.println("ESP32 OTA-Hub DIY System Starting...");
  Serial.println("Firmware Version: " + String(FIRMWARE_VERSION));
  Serial.println("Repository: " + String(OTAGH_OWNER_NAME) + "/" + String(OTAGH_REPO_NAME));
  Serial.println("External LED on pin: " + String(LED_PIN));
  Serial.println("RGB LED on pin: " + String(LED_RGB));
  Serial.println(String("=").substring(0, 50));
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initialize OTA
  wifi_client.setInsecure();
  OTA::init(wifi_client);
  
  Serial.println("System ready! Checking for updates...");
  
  // Initial update check
  checkForUpdates();
  lastUpdateCheck = millis();
  
  Serial.println("Entering main loop with LED blinking and RGB cycling...");
}

void loop() {
  // Check for updates periodically
  if (millis() - lastUpdateCheck >= updateCheckInterval) {
    Serial.println("\n--- Periodic Update Check ---");
    checkForUpdates();
    lastUpdateCheck = millis();
  }
  
  // Blink regular LEDs
  blinkLED();
  
  // Update RGB LED
  updateRgbLED();
  
  // Main application - print status every 10 seconds
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint >= 10000) {
    Serial.println("Main app running... " + String(FIRMWARE_VERSION) +
                   " | Free heap: " + String(ESP.getFreeHeap()) +
                   " | LED pin " + String(LED_PIN) + " | RGB pin " + String(LED_RGB));
    lastStatusPrint = millis();
  }
  
  delay(100); // Small delay for stability
}

void blinkLED() {
  // Blink external LED on pin 20 every 1 second
  if (millis() - lastLedBlink >= 1000) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    digitalWrite(BUILTIN_LED, ledState);
    lastLedBlink = millis();
    
    Serial.println("LED " + String(ledState ? "ON" : "OFF") + " (Pin " + String(LED_PIN) + ")");
  }
}

void updateRgbLED() {
  // Change RGB LED color every 3 seconds
  if (millis() - lastRgbUpdate >= 3000) {
    switch(rgbMode) {
      case 0: // Blue
        rgbLED.setPixelColor(0, rgbLED.Color(0, 0, 255));
        Serial.println("RGB LED: Blue");
        break;
      case 1: // Green
        rgbLED.setPixelColor(0, rgbLED.Color(0, 255, 0));
        Serial.println("RGB LED: Green");
        break;
      case 2: // Red
        rgbLED.setPixelColor(0, rgbLED.Color(255, 0, 0));
        Serial.println("RGB LED: Red");
        break;
      case 3: // Purple
        rgbLED.setPixelColor(0, rgbLED.Color(128, 0, 128));
        Serial.println("RGB LED: Purple");
        break;
      case 4: // Cyan
        rgbLED.setPixelColor(0, rgbLED.Color(0, 255, 255));
        Serial.println("RGB LED: Cyan");
        break;
      case 5: // Yellow
        rgbLED.setPixelColor(0, rgbLED.Color(255, 255, 0));
        Serial.println("RGB LED: Yellow");
        break;
    }
    
    rgbLED.show();
    rgbMode = (rgbMode + 1) % 6; // Cycle through 6 colors
    lastRgbUpdate = millis();
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: " + String(ssid));
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected successfully!");
    Serial.println("IP address: " + WiFi.localIP().toString());
    Serial.println("Signal strength: " + String(WiFi.RSSI()) + " dBm");
  } else {
    Serial.println("\nFailed to connect to WiFi!");
    Serial.println("Check your credentials and try again.");
  }
}

void checkForUpdates() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No WiFi connection - skipping update check");
    return;
  }
  
  Serial.println("Checking for firmware updates...");
  Serial.println("Repository: " + String(OTAGH_OWNER_NAME) + "/" + String(OTAGH_REPO_NAME));
  
  // Check if an update is available
  OTA::UpdateObject updateInfo = OTA::isUpdateAvailable();
  
  if (updateInfo.condition != OTA::NO_UPDATE) {
    Serial.println("New firmware available! Starting update process...");
    Serial.println("Asset ID: " + String(updateInfo.firmware_asset_id));
    
    // Flash all LEDs rapidly during update
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUILTIN_LED, HIGH);
      rgbLED.setPixelColor(0, rgbLED.Color(255, 255, 255)); // White
      rgbLED.show();
      delay(100);
      
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUILTIN_LED, LOW);
      rgbLED.setPixelColor(0, rgbLED.Color(0, 0, 0)); // Off
      rgbLED.show();
      delay(100);
    }
    
    // Set RGB LED to orange during update
    rgbLED.setPixelColor(0, rgbLED.Color(255, 165, 0)); // Orange
    rgbLED.show();
    
    // Perform the update
    OTA::InstallCondition result = OTA::performUpdate(&updateInfo);
    
    Serial.println("Update result: " + String(result));
    
    if (result == 0) {
      Serial.println("Update successful! Device will restart automatically.");
      // Set RGB LED to green for success
      rgbLED.setPixelColor(0, rgbLED.Color(0, 255, 0));
      rgbLED.show();
    } else {
      Serial.println("Update failed or needs redirect. Trying redirect method...");
      OTA::InstallCondition redirectResult = OTA::continueRedirect(&updateInfo);
      Serial.println("Redirect result: " + String(redirectResult));
      
      if (redirectResult != 0) {
        // Set RGB LED to red for failure
        rgbLED.setPixelColor(0, rgbLED.Color(255, 0, 0));
        rgbLED.show();
      }
    }
  } else {
    Serial.println("Firmware is up to date (running " + String(FIRMWARE_VERSION) + ")");
  }
}
