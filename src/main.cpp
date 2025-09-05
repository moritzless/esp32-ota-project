#include <WiFi.h>
#include <WiFiClientSecure.h>

// ================================
// TODO: CHANGE THESE TO YOUR REPO!
// ================================
#define OTAGH_OWNER_NAME "moritzless"           // ← Change to your GitHub username
#define OTAGH_REPO_NAME "esp32-ota-project"    // ← Change to your repository name

// For private repositories, uncomment and add your token:
// #define OTAGH_BEARER "your_private_repo_token"

#include <OTA-Hub-diy.hpp>

// ================================
// TODO: CHANGE YOUR WIFI CREDENTIALS!
// ================================
const char* ssid = "Moritz WLAN";           // ← Your WiFi name
const char* password = "Schnorrer123";     // ← Your WiFi password

// Firmware version for identification
const char* FIRMWARE_VERSION = "v1.0.0";

// Update check interval (5 minutes)
const unsigned long updateCheckInterval = 5 * 60 * 1000;
unsigned long lastUpdateCheck = 0;

WiFiClientSecure wifi_client;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Give time for serial monitor to connect
  
  Serial.println("\n" + String("=").substring(0, 50));
  Serial.println("ESP32 OTA-Hub DIY System Starting...");
  Serial.println("Firmware Version: " + String(FIRMWARE_VERSION));
  Serial.println("Repository: " + String(OTAGH_OWNER_NAME) + "/" + String(OTAGH_REPO_NAME));
  Serial.println(String("=").substring(0, 50));
  
  // Connect to WiFi
  connectToWiFi();
  
  // Set up secure WiFi client
  // For production, use proper certificates instead of setInsecure()
  wifi_client.setInsecure(); 
  
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
  
  // Step 1: Check if an update is available
  bool updateAvailable = false;
  
  Serial.println("Contacting GitHub API...");
  updateAvailable = OTA::isUpdateAvailable(wifi_client);
  
  if (updateAvailable) {
    Serial.println("🎉 New firmware available! Starting update process...");
    
    // Step 2: Try to perform the update
    Serial.println("📥 Downloading and installing update...");
    if (OTA::performUpdate(wifi_client)) {
      Serial.println("✅ Update successful! Restarting device...");
      delay(3000);
      ESP.restart();
    } else {
      Serial.println("⚠️ Primary update method failed, trying redirect...");
      
      // Step 3: Try following redirect if the first method failed
      if (OTA::followRedirect(wifi_client)) {
        Serial.println("✅ Redirect update successful! Restarting device...");
        delay(3000);
        ESP.restart();
      } else {
        Serial.println("❌ Both update methods failed!");
        Serial.println("Check your internet connection and GitHub repository.");
      }
    }
  } else {
    Serial.println("✅ Firmware is up to date (running " + String(FIRMWARE_VERSION) + ")");
  }
}