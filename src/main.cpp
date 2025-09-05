#include <WiFi.h>
#include <WiFiClientSecure.h>

// ================================
// OTA Hub Configuration - REQUIRED DEFINES
// ================================
#define OTAGH_OWNER_NAME "moritzless"           // Your GitHub username
#define OTAGH_REPO_NAME "esp32-ota-project"    // Your repository name

// Required OTA defines for the library
#define OTA_SERVER "api.github.com"
#define OTA_PORT 443
#define OTA_CHECK_PATH "/repos/" OTAGH_OWNER_NAME "/" OTAGH_REPO_NAME "/releases/latest"
#define FIRMWARE_BIN_MATCH "firmware.bin"
#define OTA_ASSET_ENDPOINT_CONSTRUCTOR(asset_id) "/repos/" OTAGH_OWNER_NAME "/" OTAGH_REPO_NAME "/releases/assets/" + String(asset_id)

// For private repositories, uncomment and add your token:
// #define OTAGH_BEARER "your_private_repo_token"

#include <OTA-Hub.hpp>

// ================================
// WiFi Configuration
// ================================
const char* ssid = "Moritz WLAN";           
const char* password = "Schnorrer123";     

// Firmware version for identification
const char* FIRMWARE_VERSION = "v1.0.0";

// Update check interval (5 minutes)
const unsigned long updateCheckInterval = 5 * 60 * 1000;
unsigned long lastUpdateCheck = 0;

// LED pin for ESP32 (GPIO 2)
#define LED_BUILTIN 2

WiFiClientSecure wifi_client;

// Function declarations
void connectToWiFi();
void checkForUpdates();

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // Initialize LED
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println("\n" + String("=").substring(0, 50));
  Serial.println("ESP32 OTA-Hub DIY System Starting...");
  Serial.println("Firmware Version: " + String(FIRMWARE_VERSION));
  Serial.println("Repository: " + String(OTAGH_OWNER_NAME) + "/" + String(OTAGH_REPO_NAME));
  Serial.println(String("=").substring(0, 50));
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initialize OTA with secure client
  wifi_client.setInsecure(); // For testing - use proper certs in production
  OTA::init(wifi_client);
  
  Serial.println("System ready! Checking for updates...");
  
  // Initial update check on boot
  checkForUpdates();
  lastUpdateCheck = millis();
  
  Serial.println("Entering main loop...");
}

void loop() {
  // Check for updates periodically
  if (millis() - lastUpdateCheck >= updateCheckInterval) {
    Serial.println("\n--- Periodic Update Check ---");
    checkForUpdates();
    lastUpdateCheck = millis();
  }
  
  // Your main application code goes here
  Serial.println("Main app running... " + String(FIRMWARE_VERSION) + " | Free heap: " + String(ESP.getFreeHeap()));
  
  // Blink built-in LED to show device is alive
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(9900); // Total 10 seconds
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
    Serial.println("\n✓ WiFi connected successfully!");
    Serial.println("IP address: " + WiFi.localIP().toString());
    Serial.println("Signal strength: " + String(WiFi.RSSI()) + " dBm");
  } else {
    Serial.println("\n✗ Failed to connect to WiFi!");
    Serial.println("Check your credentials and try again.");
  }
}

void checkForUpdates() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("✗ No WiFi connection - skipping update check");
    return;
  }
  
  Serial.println("🔍 Checking for firmware updates...");
  Serial.println("Repository: " + String(OTAGH_OWNER_NAME) + "/" + String(OTAGH_REPO_NAME));
  
  // Check if an update is available (new API)
  OTA::UpdateObject updateInfo = OTA::isUpdateAvailable();
  
  if (updateInfo.update_available) {
    Serial.println("🎉 New firmware available! Starting update process...");
    Serial.println("Asset ID: " + String(updateInfo.firmware_asset_id));
    
    // Perform the update with the update object
    OTA::InstallCondition result = OTA::performUpdate(&updateInfo);
    
    if (result == OTA::INSTALL_OK) {
      Serial.println("✅ Update successful! Device will restart automatically.");
      // Device will restart automatically
    } else {
      Serial.println("⚠️ Update failed with error code: " + String(result));
      
      // Try redirect method
      Serial.println("Trying redirect method...");
      OTA::InstallCondition redirectResult = OTA::continueRedirect(&updateInfo);
      
      if (redirectResult == OTA::INSTALL_OK) {
        Serial.println("✅ Redirect update successful! Device will restart automatically.");
      } else {
        Serial.println("❌ Both update methods failed!");
        Serial.println("Error code: " + String(redirectResult));
      }
    }
  } else {
    Serial.println("✅ Firmware is up to date (running " + String(FIRMWARE_VERSION) + ")");
  }
}
