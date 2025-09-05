#include <WiFi.h>
#include <WiFiClientSecure.h>

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
const char* FIRMWARE_VERSION = "v1.0.5";

// Update check interval (5 minutes)
const unsigned long updateCheckInterval = 5 * 60 * 1000;
unsigned long lastUpdateCheck = 0;

// LED configuration
#define LED_PIN 20
#define BUILTIN_LED 2  // ESP32 built-in LED

// LED blink timing
unsigned long lastLedBlink = 0;
bool ledState = false;

WiFiClientSecure wifi_client;

// Function declarations
void connectToWiFi();
void checkForUpdates();
void blinkLED();

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // Initialize LEDs
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  
  Serial.println("\n" + String("=").substring(0, 50));
  Serial.println("ESP32 OTA-Hub DIY System Starting...");
  Serial.println("Firmware Version: " + String(FIRMWARE_VERSION));
  Serial.println("Repository: " + String(OTAGH_OWNER_NAME) + "/" + String(OTAGH_REPO_NAME));
  Serial.println("External LED on pin: " + String(LED_PIN));
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
  
  Serial.println("Entering main loop with LED blinking...");
}

void loop() {
  // Check for updates periodically
  if (millis() - lastUpdateCheck >= updateCheckInterval) {
    Serial.println("\n--- Periodic Update Check ---");
    checkForUpdates();
    lastUpdateCheck = millis();
  }
  
  // Blink LEDs
  blinkLED();
  
  // Main application - print status every 10 seconds
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint >= 10000) {
    Serial.println("Main app running... " + String(FIRMWARE_VERSION) + 
                   " | Free heap: " + String(ESP.getFreeHeap()) + 
                   " | LED pin " + String(LED_PIN) + " active");
    lastStatusPrint = millis();
  }
  
  delay(100); // Small delay for stability
}

void blinkLED() {
  // Blink external LED on pin 20 every 1 second
  if (millis() - lastLedBlink >= 1000) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    digitalWrite(BUILTIN_LED, ledState); // Also blink built-in LED
    lastLedBlink = millis();
    
    Serial.println("LED " + String(ledState ? "ON" : "OFF") + " (Pin " + String(LED_PIN) + ")");
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
    
    // Flash LEDs rapidly during update
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUILTIN_LED, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUILTIN_LED, LOW);
      delay(100);
    }
    
    // Perform the update
    OTA::InstallCondition result = OTA::performUpdate(&updateInfo);
    
    Serial.println("Update result: " + String(result));
    
    if (result == 0) {
      Serial.println("Update successful! Device will restart automatically.");
    } else {
      Serial.println("Update failed or needs redirect. Trying redirect method...");
      OTA::InstallCondition redirectResult = OTA::continueRedirect(&updateInfo);
      Serial.println("Redirect result: " + String(redirectResult));
    }
  } else {
    Serial.println("Firmware is up to date (running " + String(FIRMWARE_VERSION) + ")");
  }
}
