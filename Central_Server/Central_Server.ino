#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>  // Add OTA support
#include <FirebaseESP32.h>
#include "web_templates.h"

// Define Firebase Data and Config Objects
FirebaseData fbData;
FirebaseAuth fbAuth;
FirebaseConfig fbConfig;

// ==================== SYSTEM CONFIGURATION ====================
#define SYSTEM_VERSION "v4.0"
#define SYSTEM_MODE_AUTO 0
#define SYSTEM_MODE_ONLINE 1
#define SYSTEM_MODE_OFFLINE 2

// ==================== ONLINE MODE CONFIGURATION ====================
// WiFi Credentials (for Online Mode)
const char* WIFI_SSID = "Kaizen";      // Change to your WiFi
const char* WIFI_PASSWORD = "b@s!t123$";  // Change to your password

// Firebase / Cloud Configuration (Optional)
const char* FIREBASE_HOST = "car-parking-system-a2064-default-rtdb.europe-west1.firebasedatabase.app";
const char* FIREBASE_AUTH = "RgVsCdCUiEB1Ian26wTMPmUMipVuktmWmDildvAW";

// ==================== LCD CONFIGURATION ====================
#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 4

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

unsigned long lastLcdUpdate = 0;
const unsigned long LCD_UPDATE_INTERVAL = 1000;

// ==================== SYSTEM MODE VARIABLES ====================
int systemMode = SYSTEM_MODE_AUTO;  // 0=Auto, 1=Online, 2=Offline
bool isOnlineMode = false;
bool wifiConnected = false;
unsigned long lastModeCheck = 0;
const unsigned long MODE_CHECK_INTERVAL = 2000; // Check every 2 seconds
unsigned long lastCloudSync = 0;
const unsigned long CLOUD_SYNC_INTERVAL = 30000; // Sync every minute
volatile bool pendingCloudSync = false; // Flag to sync to Firebase in main loop

// ==================== FALLBACK DATA STORAGE (Offline Mode) ====================
#define MAX_OFFLINE_RECORDS 100
struct ParkingRecord {
    unsigned long timestamp;
    int occupiedSlots;
    int availableSlots;
    float powerUsage;
    char eventType[20];  // "ENTRY", "EXIT", "FULL", "EMPTY"
};
ParkingRecord offlineRecords[MAX_OFFLINE_RECORDS];
int offlineRecordCount = 0;

// ==================== GATE CONTROL PINS ====================
const int IR_ENTRY_PIN = 26;
const int IR_EXIT_PIN = 27;
const int SERVO_PIN = 25;
const int BUZZER_PIN = 33;
const int MODE_SWITCH_PIN = 32;  // New: Manual mode switch (HIGH=Online, LOW=Offline)

// ==================== SERVO CONFIGURATION ====================
Servo gateServo;
const int SERVO_OPEN = 100;
const int SERVO_CLOSED = 0;
bool gateOpen = false;
unsigned long gateOpenTime = 0;
const unsigned long GATE_OPEN_DURATION = 8000;

// ==================== BUZZER FEEDBACK ====================
const int BUZZER_OPEN = 1000;
const int BUZZER_CLOSE = 500;
const int BUZZER_FULL = 200;
const int BUZZER_ERROR = 150;
const int BUZZER_MODE_CHANGE = 800;
const int BUZZER_OTA_START = 1000;
const int BUZZER_OTA_SUCCESS = 1200;

// ==================== IR SENSOR DEBOUNCE ====================
bool lastEntryState = HIGH;
bool lastExitState = HIGH;
unsigned long lastEntryTime = 0;
unsigned long lastExitTime = 0;
const unsigned long DEBOUNCE_DELAY = 500;

// Current sensor pin
const int CURRENT_SENSOR_PIN = 34;
const float SENSITIVITY = 0.185;
const float ADC_REFERENCE = 3.3;
const int ADC_RESOLUTION = 4095;

AsyncWebServer server(80);

// Data structure for sensor data
typedef struct sensor_data {
    int slot1;
    int slot2;
    int slot3;
    int slot4;
    int slot5;
    bool sensorStatus[10];
    int side;
} sensor_data;

sensor_data leftData = {0, 0, 0, 0, 0, {false, false, false, false, false, false, false, false, false, false}, 0};
sensor_data rightData = {0, 0, 0, 0, 0, {false, false, false, false, false, false, false, false, false, false}, 1};

float currentValue = 0;
float voltageValue = 0;
float powerValue = 0;
float zeroOffset = 0;

struct Peer {
    uint8_t mac[6];
    int side;
    bool registered;
    unsigned long lastSeen;
    unsigned long lastMessageTime;  // Track when last message was received
};

#define MAX_PEERS 10
Peer peers[MAX_PEERS];
int peerCount = 0;

String systemLogs[30];  // Increased for better logging
int logIndex = 0;

// Custom characters
byte progressBarFull[8] = {
  0b11111, 0b11111, 0b11111, 0b11111,
  0b11111, 0b11111, 0b11111, 0b11111
};

byte progressBarEmpty[8] = {
  0b10001, 0b10001, 0b10001, 0b10001,
  0b10001, 0b10001, 0b10001, 0b10001
};

byte carSymbol[8] = {
  0b00000, 0b01110, 0b11111, 0b11111,
  0b11111, 0b11111, 0b01110, 0b00000
};

byte gateSymbol[8] = {
  0b00100, 0b01110, 0b11111, 0b11111,
  0b11111, 0b11111, 0b01110, 0b00100
};

byte loadingFrames[4][8] = {
  {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000},
  {0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100},
  {0b01110, 0b01110, 0b01110, 0b01110, 0b01110, 0b01110, 0b01110, 0b01110},
  {0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111}
};

byte checkMark[8] = {
  0b00000, 0b00001, 0b00011, 0b10110,
  0b11100, 0b01000, 0b00000, 0b00000
};

byte wifiSymbol[8] = {
  0b00000, 0b01110, 0b10001, 0b00100,
  0b01010, 0b00100, 0b00000, 0b00000
};

byte cloudSymbol[8] = {
  0b00000, 0b01110, 0b11111, 0b11011,
  0b11111, 0b01110, 0b00000, 0b00000
};

// ==================== OTA SETUP FUNCTION ====================
// ==================== OTA SETUP FUNCTION ====================
bool otaInitialized = false;

void setupOTA() {
    if (otaInitialized) return;
    
    ArduinoOTA.setHostname("SmartParking");
    ArduinoOTA.setPassword("smartpark123");  // Set OTA password for security
    ArduinoOTA.setTimeout(20000);             // Set timeout to 20 seconds for stability
    
    ArduinoOTA.onStart([]() {
        String type;
        if(ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {
            type = "filesystem";
        }
        Serial.println("\n🔧 Start updating " + type);
        addLog("OTA update started - " + type);
        
        // Show OTA update on LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("OTA UPDATING...");
        lcd.setCursor(0, 1);
        lcd.print("DO NOT POWER OFF");
        lcd.setCursor(0, 2);
        lcd.print("Updating: ");
        lcd.print(type);
        
        // Buzzer feedback
        playTone(BUZZER_OTA_START, 300);
        delay(100);
        playTone(BUZZER_OTA_START, 300);
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\n✅ OTA Update Completed!");
        addLog("OTA update completed successfully");
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("OTA COMPLETE!");
        lcd.setCursor(0, 1);
        lcd.print("Restarting...");
        
        // Success buzzer
        playTone(BUZZER_OTA_SUCCESS, 200);
        delay(150);
        playTone(BUZZER_OTA_SUCCESS, 200);
        delay(150);
        playTone(BUZZER_OTA_SUCCESS, 400);
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static int lastPercent = -1;
        int percent = (progress * 100) / total;
        if(percent != lastPercent) {
            lastPercent = percent;
            Serial.printf("Progress: %u%%\r", percent);
            
            // Update LCD with progress
            lcd.setCursor(0, 3);
            lcd.print("Progress: ");
            lcd.print(percent);
            lcd.print("%   ");
            
            // Draw progress bar on LCD row 3
            int bars = (percent * 16) / 100;
            lcd.setCursor(0, 3);
            for(int b = 0; b < bars; b++) {
                lcd.write(0);  // Use progress bar character
            }
        }
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("❌ Error[%u]: ", error);
        String errorMsg;
        
        switch(error) {
            case OTA_AUTH_ERROR:
                errorMsg = "Auth Failed";
                break;
            case OTA_BEGIN_ERROR:
                errorMsg = "Begin Failed";
                break;
            case OTA_CONNECT_ERROR:
                errorMsg = "Connect Failed";
                break;
            case OTA_RECEIVE_ERROR:
                errorMsg = "Receive Failed";
                break;
            case OTA_END_ERROR:
                errorMsg = "End Failed";
                break;
            default:
                errorMsg = "Unknown Error";
                break;
        }
        
        addLog("OTA update failed: " + errorMsg);
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("OTA FAILED!");
        lcd.setCursor(0, 1);
        lcd.print(errorMsg);
        lcd.setCursor(0, 2);
        lcd.print("Restart required");
        
        // Error buzzer
        for(int i = 0; i < 3; i++) {
            playTone(BUZZER_ERROR, 200);
            delay(200);
        }
    });
    
    ArduinoOTA.begin();
    otaInitialized = true;
    addLog("OTA updates enabled - Host: SmartParking");
    Serial.println("✅ OTA Ready - Host: SmartParking");
    Serial.println("   Use Arduino IDE > Tools > Port > Network Ports");
}

// ==================== ONLINE/OFFLINE MODE MANAGEMENT ====================

unsigned long lastConnectAttempt = 0;
const unsigned long CONNECT_ATTEMPT_INTERVAL = 30000; // Try connecting every 30s if down

void determineSystemMode() {
    int manualSwitch = digitalRead(MODE_SWITCH_PIN);
    
    // 1. Check physical switch override (LOW = Force Offline)
    if (manualSwitch == LOW) {
        if (systemMode != SYSTEM_MODE_OFFLINE) {
            systemMode = SYSTEM_MODE_OFFLINE;
            addLog("Switch changed: forcing OFFLINE");
        }
    } else {
        // If switch is HIGH (Online) and system was in OFFLINE mode via switch, restore to AUTO
        if (systemMode == SYSTEM_MODE_OFFLINE) {
            systemMode = SYSTEM_MODE_AUTO;
            addLog("Switch changed: restoring to ONLINE/AUTO");
        }
    }
    
    // 2. Handle connection state based on mode
    if (systemMode == SYSTEM_MODE_AUTO || systemMode == SYSTEM_MODE_ONLINE) {
        if (WiFi.status() != WL_CONNECTED) {
            // Transition out of online mode if we lost connection
            if (isOnlineMode) {
                isOnlineMode = false;
                wifiConnected = false;
                Serial.println("\n⚠️ WiFi Connection Lost - Switched to OFFLINE mode");
                addLog("WiFi connection lost");
                showOfflineModeActivated();
            }
            
            // Try connecting in the background (Non-blocking)
            if (millis() - lastConnectAttempt > CONNECT_ATTEMPT_INTERVAL) {
                lastConnectAttempt = millis();
                Serial.println("📡 WiFi disconnected. Reconnecting in background...");
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            }
        } else {
            // WiFi is connected
            if (!isOnlineMode) {
                WiFi.setSleep(false); // Disable sleep for stability
                isOnlineMode = true;
                wifiConnected = true;
                Serial.println("\n✅ WiFi Connected - ONLINE Mode Activated");
                addLog("WiFi connected - ONLINE mode active");
                playTone(BUZZER_MODE_CHANGE, 100);
                showOnlineModeActivated();
                setupOTA();  // Initialize OTA
            }
        }
    } 
    else if (systemMode == SYSTEM_MODE_OFFLINE) {
        if (isOnlineMode || WiFi.status() == WL_CONNECTED) {
            isOnlineMode = false;
            wifiConnected = false;
            WiFi.disconnect(true);
            Serial.println("\n🚫 Forced OFFLINE mode active. WiFi shut down.");
            addLog("Forced OFFLINE - WiFi disconnected");
            showOfflineModeActivated();
        }
    }
}

void showOnlineModeActivated() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(9);  // WiFi symbol
    lcd.print(" ONLINE MODE");
    lcd.setCursor(0, 1);
    lcd.print("Connected to:");
    lcd.setCursor(0, 2);
    lcd.print(WIFI_SSID);
    lcd.setCursor(0, 3);
    lcd.print("IP: ");
    lcd.print(WiFi.localIP());
    delay(2000);
}

void showOfflineModeActivated() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" OFFLINE MODE");
    lcd.setCursor(0, 1);
    lcd.print("Standalone Ops");
    lcd.setCursor(0, 2);
    lcd.print("Data Stored Locally");
    lcd.setCursor(0, 3);
    lcd.print("AP: SmartParking");
    delay(2000);
}

void syncToCloud() {
    if (!isOnlineMode) return;
    
    int occupiedSlots = calculateOccupiedSlots();
    int totalSlots = 10;
    int availableSlots = totalSlots - occupiedSlots;
    
    // Find Left and Right nodes
    String leftNodeMac = "Not Connected";
    String rightNodeMac = "Not Connected";
    bool leftNodeOnline = false;
    bool rightNodeOnline = false;
    
    for(int i = 0; i < peerCount; i++) {
        if(peers[i].registered) {
            String macAddr = formatMacAddress(peers[i].mac);
            bool isOnline = (millis() - peers[i].lastMessageTime < 60000); // Online if message in last 60s
            
            if(peers[i].side == 0) {
                leftNodeMac = macAddr;
                leftNodeOnline = isOnline;
            } else if(peers[i].side == 1) {
                rightNodeMac = macAddr;
                rightNodeOnline = isOnline;
            }
        }
    }

    // Prepare JSON data
    StaticJsonDocument<512> doc;
    doc["uptime"] = millis();
    doc["occupiedSlots"] = occupiedSlots;
    doc["availableSlots"] = availableSlots;
    doc["occupancyRate"] = (occupiedSlots * 100) / totalSlots;
    doc["powerUsage"] = powerValue;
    doc["gateStatus"] = gateOpen ? "OPEN" : "CLOSED";
    doc["mode"] = isOnlineMode ? "ONLINE" : "OFFLINE";
    doc["sysMode"] = systemMode;
    doc["leftNodeMac"] = leftNodeMac;
    doc["rightNodeMac"] = rightNodeMac;
    doc["leftNodeOnline"] = leftNodeOnline;
    doc["rightNodeOnline"] = rightNodeOnline;
    
    // Left side slots
    JsonArray leftSlots = doc.createNestedArray("leftSlots");
    leftSlots.add(leftData.slot1);
    leftSlots.add(leftData.slot2);
    leftSlots.add(leftData.slot3);
    leftSlots.add(leftData.slot4);
    leftSlots.add(leftData.slot5);
    
    // Right side slots
    JsonArray rightSlots = doc.createNestedArray("rightSlots");
    rightSlots.add(rightData.slot1);
    rightSlots.add(rightData.slot2);
    rightSlots.add(rightData.slot3);
    rightSlots.add(rightData.slot4);
    rightSlots.add(rightData.slot5);
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    FirebaseJson json;
    json.setJsonData(jsonString);
    json.set("timestamp/.sv", "timestamp");
    
    Serial.println("☁️ Cloud sync: Attempting Firebase upload...");
    
    if (Firebase.ready()) {
        // 1. Update the live real-time state (overwrites)
        bool liveStatusUpdated = Firebase.setJSON(fbData, "/parking/current", json);
        
        // 2. Append to the historical telemetry log (creates a history entry)
        bool historyLogged = Firebase.pushJSON(fbData, "/parking/history", json);
        
        if (liveStatusUpdated && historyLogged) {
            Serial.println("✅ Firebase upload SUCCESSFUL (Live & History)");
            addLog("Firebase sync completed");
        } else {
            Serial.printf("❌ Firebase upload FAILED. Live status error: %s | History error: %s\n", 
                          fbData.errorReason().c_str(), 
                          fbData.errorReason().c_str());
            addLog("Firebase sync failed");
        }
    } else {
        Serial.println("⚠️ Firebase client not ready");
    }
}

void saveOfflineRecord(const char* eventType) {
    if (offlineRecordCount < MAX_OFFLINE_RECORDS) {
        offlineRecords[offlineRecordCount].timestamp = millis();
        offlineRecords[offlineRecordCount].occupiedSlots = calculateOccupiedSlots();
        offlineRecords[offlineRecordCount].availableSlots = 10 - offlineRecords[offlineRecordCount].occupiedSlots;
        offlineRecords[offlineRecordCount].powerUsage = powerValue;
        strcpy(offlineRecords[offlineRecordCount].eventType, eventType);
        offlineRecordCount++;
        
        addLog(String("Offline record saved: ") + eventType);
    } else {
        // Shift records if full
        for (int i = 1; i < MAX_OFFLINE_RECORDS; i++) {
            offlineRecords[i-1] = offlineRecords[i];
        }
        offlineRecordCount = MAX_OFFLINE_RECORDS - 1;
        saveOfflineRecord(eventType); // Retry
    }
}

String getOfflineDataJSON() {
    String json = "{\"offlineRecords\":[";
    for (int i = 0; i < offlineRecordCount; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"timestamp\":" + String(offlineRecords[i].timestamp) + ",";
        json += "\"occupied\":" + String(offlineRecords[i].occupiedSlots) + ",";
        json += "\"available\":" + String(offlineRecords[i].availableSlots) + ",";
        json += "\"power\":" + String(offlineRecords[i].powerUsage) + ",";
        json += "\"event\":\"" + String(offlineRecords[i].eventType) + "\"";
        json += "}";
    }
    json += "]}";
    return json;
}

// ==================== BOOT ANIMATION ====================
void showBootAnimation() {
    for(int i = 0; i < 4; i++) {
        lcd.createChar(4 + i, loadingFrames[i]);
    }
    lcd.createChar(8, checkMark);
    lcd.createChar(9, wifiSymbol);
    lcd.createChar(10, cloudSymbol);
    
    lcd.clear();
    for(int frame = 0; frame < 3; frame++) {
        lcd.setCursor(0, 0);
        lcd.print("=================");
        lcd.setCursor(0, 1);
        lcd.print("   SMART PARK   ");
        lcd.setCursor(0, 2);
        lcd.print("     SYSTEM     ");
        lcd.setCursor(0, 3);
        lcd.print("   ");
        lcd.print(SYSTEM_VERSION);
        lcd.print(" PRO     ");
        
        for(int i = 0; i < 16; i++) {
            lcd.setCursor(i, 0);
            lcd.write(4 + frame);
            lcd.setCursor(i, 3);
            lcd.write(4 + frame);
            delay(10);
        }
        delay(100);
    }
    delay(800);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("INITIALIZING");
    lcd.setCursor(0, 1);
    lcd.print("[");
    lcd.setCursor(15, 1);
    lcd.print("]");
    
    for(int i = 0; i <= 100; i += 5) {
        int bars = (i * 13) / 100;
        lcd.setCursor(1, 1);
        for(int b = 0; b < bars; b++) {
            lcd.write(0);
        }
        lcd.setCursor(12, 2);
        lcd.print(i);
        lcd.print("%");
        delay(30);
    }
    delay(200);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MODE DETECTION");
    delay(1000);
}

void showSystemReady() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("=================");
    lcd.setCursor(0, 1);
    lcd.print("   SYSTEM READY  ");
    lcd.setCursor(0, 2);
    lcd.print("  SMART PARKING  ");
    lcd.setCursor(0, 3);
    lcd.print("   ");
    lcd.print(SYSTEM_VERSION);
    lcd.print(" PRO     ");
    delay(1500);
    
    tone(BUZZER_PIN, 800, 200);
    delay(200);
    tone(BUZZER_PIN, 1200, 200);
}

// ==================== LCD DISPLAY FUNCTIONS ====================
void createCustomCharacters() {
    lcd.createChar(0, progressBarFull);
    lcd.createChar(1, progressBarEmpty);
    lcd.createChar(2, carSymbol);
    lcd.createChar(3, gateSymbol);
}

void drawProgressBar(int percentage, int row, int col, int length) {
    int filled = (percentage * length) / 100;
    lcd.setCursor(col, row);
    for(int i = 0; i < length; i++) {
        if(i < filled) {
            lcd.write(0);
        } else {
            lcd.write(1);
        }
    }
}

void updateLCD() {
    int occupiedSlots = calculateOccupiedSlots();
    int totalSlots = 10;
    int availableSlots = totalSlots - occupiedSlots;
    int occupancyRate = (occupiedSlots * 100) / totalSlots;
    
    lcd.clear();
    
    // Row 0: System Title and Mode
    lcd.setCursor(0, 0);
    lcd.write(2);
    lcd.print(" SMART PARK");
    lcd.setCursor(14, 0);
    if (isOnlineMode) {
        lcd.write(9);  // WiFi symbol
    } else {
        lcd.write(10); // Cloud with slash (offline)
    }
    
    // Row 1: Slot Availability
    lcd.setCursor(0, 1);
    lcd.print("Avail:");
    lcd.print(availableSlots);
    lcd.print("/");
    lcd.print(totalSlots);
    
    lcd.setCursor(10, 1);
    lcd.print("Occ:");
    lcd.print(occupiedSlots);
    
    // Row 2: Occupancy Progress Bar
    lcd.setCursor(0, 2);
    lcd.print("Occ:");
    lcd.print(occupancyRate);
    lcd.print("% ");
    drawProgressBar(occupancyRate, 2, 6, 8);
    
    // Row 3: Dynamic Status
    lcd.setCursor(0, 3);
    
    if(availableSlots == 0) {
        lcd.print("!FULL! NO ENTRY");
    } 
    else if(gateOpen) {
        lcd.print("GATE OPEN     ");
    }
    else if(countHealthySensors(leftData) < 10 || countHealthySensors(rightData) < 10) {
        lcd.print("!SENSOR ISSUE!");
    }
    else {
        lcd.print(isOnlineMode ? "ONLINE " : "OFFLINE ");
        lcd.print(powerValue, 1);
        lcd.print("W ");
        if(currentValue > 0.1) {
            lcd.print(currentValue, 1);
            lcd.print("A");
        }
    }
}

void showLCDGateMessage(String message) {
    String tempMessage = message;
    if(tempMessage.length() > 16) tempMessage = tempMessage.substring(0, 16);
    
    lcd.setCursor(0, 3);
    lcd.print(tempMessage);
    for(int i = tempMessage.length(); i < 16; i++) {
        lcd.print(" ");
    }
}

void showLCDParkingFull() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("!!!!!!!!!!!!!!!!");
    lcd.setCursor(0, 1);
    lcd.print("! PARKING FULL !");
    lcd.setCursor(0, 2);
    lcd.print("!  NO ENTRY    !");
    lcd.setCursor(0, 3);
    lcd.print("!!!!!!!!!!!!!!!!");
    delay(2000);
}

void showLCDGateStatus() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("   GATE STATUS  ");
    lcd.setCursor(0, 1);
    if(gateOpen) {
        lcd.print("  [OPEN]  ");
        lcd.setCursor(0, 2);
        lcd.print("  Vehicle Pass  ");
    } else {
        lcd.print("  [CLOSED] ");
        lcd.setCursor(0, 2);
        lcd.print("   Waiting...   ");
    }
    lcd.setCursor(0, 3);
    lcd.print("=================");
    delay(1500);
}

void showLCDSystemInfo() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SYSTEM INFO");
    lcd.setCursor(0, 1);
    lcd.print("Mode:");
    lcd.print(isOnlineMode ? "ONLINE" : "OFFLINE");
    lcd.setCursor(0, 2);
    lcd.print("Nodes:");
    lcd.print(peerCount);
    lcd.setCursor(0, 3);
    lcd.print("Uptime:");
    unsigned long uptime = millis() / 1000;
    lcd.print(uptime / 3600);
    lcd.print("h");
    lcd.print((uptime % 3600) / 60);
    lcd.print("m");
    delay(3000);
}

// ==================== BUZZER FUNCTIONS ====================
void playTone(int frequency, int duration) {
    tone(BUZZER_PIN, frequency, duration);
    delay(duration);
}

void buzzerGateOpen() {
    Serial.println("🔊 Buzzer: Gate Opening");
    playTone(BUZZER_OPEN, 200);
    delay(100);
    playTone(BUZZER_OPEN, 200);
    showLCDGateMessage("Gate Opening...");
}

void buzzerGateClose() {
    Serial.println("🔊 Buzzer: Gate Closing");
    playTone(BUZZER_CLOSE, 300);
    showLCDGateMessage("Gate Closing...");
}

void buzzerParkingFull() {
    Serial.println("🔊 Buzzer: Parking Full");
    for(int i = 0; i < 3; i++) {
        playTone(BUZZER_FULL, 200);
        delay(200);
    }
}

void buzzerError() {
    Serial.println("🔊 Buzzer: Error");
    for(int i = 0; i < 2; i++) {
        playTone(BUZZER_ERROR, 100);
        delay(100);
    }
}

// ==================== SERVO CONTROL FUNCTIONS ====================
void openGate() {
    if(!gateOpen) {
        Serial.println("🚪 Opening Gate...");
        gateServo.write(SERVO_OPEN);
        gateOpen = true;
        gateOpenTime = millis();
        buzzerGateOpen();
        addLog("Gate opened");
        showLCDGateStatus();
        
        // Record event
        if (!isOnlineMode) {
            saveOfflineRecord("GATE_OPEN");
        } else {
            pendingCloudSync = true;
        }
    }
}

void closeGate() {
    if(gateOpen) {
        Serial.println("🚪 Closing Gate...");
        gateServo.write(SERVO_CLOSED);
        gateOpen = false;
        buzzerGateClose();
        addLog("Gate closed");
        showLCDGateStatus();
        
        // Record event / trigger cloud sync immediately
        if (!isOnlineMode) {
            saveOfflineRecord("GATE_CLOSED");
        } else {
            pendingCloudSync = true;
        }
    }
}

// ==================== GATE CONTROL LOGIC ====================
void checkGateControl() {
    int occupiedSlots = calculateOccupiedSlots();
    int totalSlots = 10;
    int availableSlots = totalSlots - occupiedSlots;
    
    bool currentEntryState = digitalRead(IR_ENTRY_PIN);
    bool currentExitState = digitalRead(IR_EXIT_PIN);
    unsigned long currentTime = millis();
    
    if(currentEntryState == LOW && lastEntryState == HIGH && 
       (currentTime - lastEntryTime) > DEBOUNCE_DELAY) {
        lastEntryTime = currentTime;
        Serial.println("📥 Car detected at ENTRY gate");
        showLCDGateMessage("Car at Entry");
        
        if(availableSlots > 0) {
            Serial.printf("✅ Parking available (%d slots), opening gate\n", availableSlots);
            openGate();
            addLog("Entry request granted - " + String(availableSlots) + " slots available");
            
            if (!isOnlineMode) {
                saveOfflineRecord("ENTRY");
            }
        } else {
            Serial.println("❌ Parking FULL! Gate remains closed");
            buzzerParkingFull();
            addLog("Entry DENIED - Parking full");
            showLCDGateMessage("FULL! No Entry");
            
            if (!isOnlineMode) {
                saveOfflineRecord("ENTRY_DENIED_FULL");
            }
        }
    }
    
    if(currentExitState == LOW && lastExitState == HIGH && 
       (currentTime - lastExitTime) > DEBOUNCE_DELAY) {
        lastExitTime = currentTime;
        Serial.println("📤 Car detected at EXIT gate");
        showLCDGateMessage("Car at Exit");
        openGate();
        addLog("Exit request granted - Gate opening");
        
        if (!isOnlineMode) {
            saveOfflineRecord("EXIT");
        }
    }
    
    if(gateOpen && (currentTime - gateOpenTime) > GATE_OPEN_DURATION) {
        closeGate();
    }
    
    lastEntryState = currentEntryState;
    lastExitState = currentExitState;
}

// ==================== EXISTING FUNCTIONS ====================
void addLog(String message) {
    String timestamp = String(millis()/1000) + "s";
    String modeTag = isOnlineMode ? "[ONLINE]" : "[OFFLINE]";
    systemLogs[logIndex] = timestamp + " " + modeTag + ": " + message;
    logIndex = (logIndex + 1) % 30;
    Serial.println("[" + timestamp + "] " + message);
}

void calibrateCurrentSensor() {
    Serial.println("Calibrating current sensor...");
    long sum = 0;
    int samples = 100;
    
    for(int i = 0; i < samples; i++) {
        sum += analogRead(CURRENT_SENSOR_PIN);
        delay(5);
    }
    
    float avgReading = sum / samples;
    float avgVoltage = (avgReading / ADC_RESOLUTION) * ADC_REFERENCE;
    zeroOffset = avgVoltage - (ADC_REFERENCE / 2);
    
    Serial.printf("Calibration complete - Zero offset: %.2f mV\n", zeroOffset * 1000);
    addLog("Current sensor calibrated - Offset: " + String(zeroOffset * 1000, 2) + "mV");
}

float readCurrent() {
    long sum = 0;
    int samples = 50;
    
    for(int i = 0; i < samples; i++) {
        sum += analogRead(CURRENT_SENSOR_PIN);
        delay(1);
    }
    
    float avgReading = sum / samples;
    float voltage = (avgReading / ADC_RESOLUTION) * ADC_REFERENCE;
    voltage -= zeroOffset;
    float current = voltage / SENSITIVITY;
    
    if(abs(current) < 0.05) {
        current = 0;
    }
    
    return current;
}

int findOrAddPeer(const uint8_t *mac) {
    for(int i = 0; i < peerCount; i++) {
        if(memcmp(peers[i].mac, mac, 6) == 0) {
            peers[i].lastSeen = millis();
            return i;
        }
    }
    
    if(peerCount < MAX_PEERS) {
        memcpy(peers[peerCount].mac, mac, 6);
        peers[peerCount].registered = false;
        peers[peerCount].lastSeen = millis();
        peers[peerCount].lastMessageTime = millis();
        peers[peerCount].side = -1;
        peerCount++;
        return peerCount - 1;
    }
    
    return -1;
}

bool registerPeer(const uint8_t *mac) {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if(esp_now_add_peer(&peerInfo) == ESP_OK) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        addLog("Registered peer: " + String(macStr));
        return true;
    }
    return false;
}

int calculateOccupiedSlots() {
    int occupied = 0;
    if(leftData.slot1 == 1) occupied++;
    if(leftData.slot2 == 1) occupied++;
    if(leftData.slot3 == 1) occupied++;
    if(leftData.slot4 == 1) occupied++;
    if(leftData.slot5 == 1) occupied++;
    if(rightData.slot1 == 1) occupied++;
    if(rightData.slot2 == 1) occupied++;
    if(rightData.slot3 == 1) occupied++;
    if(rightData.slot4 == 1) occupied++;
    if(rightData.slot5 == 1) occupied++;
    return occupied;
}

int countHealthySensors(sensor_data data) {
    int healthy = 0;
    for(int i = 0; i < 10; i++) {
        if(data.sensorStatus[i]) healthy++;
    }
    return healthy;
}

// Helper function to format MAC address
String formatMacAddress(const uint8_t *mac) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

// Helper function to get time since last message
String getTimeSinceLastMessage(unsigned long lastMsgTime) {
    unsigned long elapsed = (millis() - lastMsgTime) / 1000; // seconds
    if (elapsed < 60) {
        return String(elapsed) + "s ago";
    } else if (elapsed < 3600) {
        return String(elapsed / 60) + "m " + String(elapsed % 60) + "s ago";
    } else {
        return String(elapsed / 3600) + "h " + String((elapsed % 3600) / 60) + "m ago";
    }
}

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    sensor_data receivedData;
    
    if(len != sizeof(receivedData)) {
        addLog("Error: Received data size mismatch");
        return;
    }
    
    memcpy(&receivedData, incomingData, sizeof(receivedData));
    
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             info->src_addr[0], info->src_addr[1], info->src_addr[2],
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);
    
    int peerIndex = findOrAddPeer(info->src_addr);
    if(peerIndex >= 0 && !peers[peerIndex].registered) {
        if(registerPeer(info->src_addr)) {
            peers[peerIndex].registered = true;
            peers[peerIndex].side = receivedData.side;
            peers[peerIndex].lastMessageTime = millis();
            addLog("New peer registered: " + String(macStr) + " Side: " + (receivedData.side == 0 ? "Left" : "Right"));
        }
    } else if(peerIndex >= 0) {
        peers[peerIndex].lastSeen = millis();
        peers[peerIndex].lastMessageTime = millis();
        peers[peerIndex].side = receivedData.side;
    }
    
    if(receivedData.side == 0) {
        leftData = receivedData;
        addLog("Left side data received");
        Serial.print("Left Side - Slots: ");
        Serial.print(leftData.slot1);
        Serial.print(leftData.slot2);
        Serial.print(leftData.slot3);
        Serial.print(leftData.slot4);
        Serial.println(leftData.slot5);
        
        int healthySensors = countHealthySensors(leftData);
        if(healthySensors < 10) {
            addLog("Warning: Left side has " + String(10 - healthySensors) + " sensor(s) not working");
        }
    } else if(receivedData.side == 1) {
        rightData = receivedData;
        addLog("Right side data received");
        Serial.print("Right Side - Slots: ");
        Serial.print(rightData.slot1);
        Serial.print(rightData.slot2);
        Serial.print(rightData.slot3);
        Serial.print(rightData.slot4);
        Serial.println(rightData.slot5);
        
        int healthySensors = countHealthySensors(rightData);
        if(healthySensors < 10) {
            addLog("Warning: Right side has " + String(10 - healthySensors) + " sensor(s) not working");
        }
    }
    
    // Sync to cloud if online mode
    if (isOnlineMode) {
        pendingCloudSync = true;
    } else {
        saveOfflineRecord("DATA_UPDATE");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize mode switch
    pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
    
    // Initialize LCD
    Wire.begin();
    lcd.begin(LCD_COLUMNS, LCD_ROWS);
    lcd.backlight();
    createCustomCharacters();
    
    showBootAnimation();
    
    Serial.println("\n\n=================================");
    Serial.println("Smart Parking System - " + String(SYSTEM_VERSION));
    Serial.println("=================================\n");
    
    // Initialize gate components
    pinMode(IR_ENTRY_PIN, INPUT_PULLUP);
    pinMode(IR_EXIT_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    
    gateServo.attach(SERVO_PIN);
    gateServo.write(SERVO_CLOSED);
    Serial.println("✓ Servo initialized");
    
    Serial.println("🔊 Testing buzzer...");
    playTone(BUZZER_OPEN, 100);
    delay(100);
    playTone(BUZZER_CLOSE, 100);
    Serial.println("✓ Buzzer test complete");
    
    // Current sensor
    pinMode(CURRENT_SENSOR_PIN, INPUT);
    analogReadResolution(12);
    delay(1000);
    calibrateCurrentSensor();
    
    // Initialize peer array
    for(int i = 0; i < MAX_PEERS; i++) {
        peers[i].registered = false;
        peers[i].side = -1;
        peers[i].lastSeen = 0;
        peers[i].lastMessageTime = 0;
    }
    
    // Setup WiFi AP (always available for configuration)
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    
    // Determine system mode
    determineSystemMode();
    
    const char* ssid = "SmartParking";
    const char* password = "12345678";
    
    Serial.println("Configuring Access Point...");
    bool apResult = WiFi.softAP(ssid, password, 6, 0, 4);
    delay(100);
    
    if (apResult) {
        Serial.println("✓ Access Point started");
        Serial.print("  AP IP: ");
        Serial.println(WiFi.softAPIP());
        addLog("AP started: " + String(ssid));
    } else {
        Serial.println("✗ Failed to start AP");
        addLog("AP startup failed!");
    }
    
    // Initialize ESP-NOW
    Serial.println("\nInitializing ESP-NOW...");
    if (esp_now_init() != ESP_OK) {
        Serial.println("✗ Error initializing ESP-NOW");
        addLog("ESP-NOW initialization failed");
        return;
    }
    
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("✓ ESP-NOW initialized");
    addLog("ESP-NOW initialized");
    
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
    
    showSystemReady();
    showLCDSystemInfo();
    
    // Initialize Firebase
    fbConfig.host = FIREBASE_HOST;
    fbConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
    
    Firebase.reconnectWiFi(true);
    Firebase.begin(&fbConfig, &fbAuth);
    
    addLog("Firebase client initialized");
    
    // ==================== WEB SERVER ROUTES ====================
    // Route for Public Dashboard
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", PUBLIC_HTML);
    });

    // Route for Admin Control Panel
    server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", ADMIN_HTML);
    });
    
    // API endpoint for mode control
    server.on("/api/mode", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("mode")) {
            String mode = request->getParam("mode")->value();
            if (mode == "auto") {
                systemMode = SYSTEM_MODE_AUTO;
                addLog("Mode changed to AUTO via web");
            } else if (mode == "online") {
                systemMode = SYSTEM_MODE_ONLINE;
                addLog("Mode changed to ONLINE via web");
            } else if (mode == "offline") {
                systemMode = SYSTEM_MODE_OFFLINE;
                addLog("Mode changed to OFFLINE via web");
            }
            determineSystemMode();
            request->send(200, "text/plain", "Mode changed to " + mode);
        } else {
            request->send(400, "text/plain", "Missing mode parameter");
        }
    });
    
    // API endpoint for offline data
    server.on("/api/offline", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", getOfflineDataJSON());
    });
    
    // API endpoint for system info
    server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"version\":\"" + String(SYSTEM_VERSION) + "\",";
        json += "\"mode\":\"" + String(isOnlineMode ? "ONLINE" : "OFFLINE") + "\",";
        json += "\"uptime\":" + String(millis() / 1000) + ",";
        json += "\"peers\":" + String(peerCount) + ",";
        json += "\"offlineRecords\":" + String(offlineRecordCount);
        json += "}";
        request->send(200, "application/json", json);
    });

    // API endpoint for raw system logs
    server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *request){
        String logContent = "";
        for(int i = 0; i < 30; i++){
            int index = (logIndex + i) % 30;
            if(systemLogs[index] != ""){
                logContent += systemLogs[index] + "\n";
            }
        }
        request->send(200, "text/plain", logContent);
    });
    
    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"left\":[" + String(leftData.slot1) + "," + String(leftData.slot2) + "," + String(leftData.slot3) + "," + String(leftData.slot4) + "," + String(leftData.slot5) + "],";
        json += "\"right\":[" + String(rightData.slot1) + "," + String(rightData.slot2) + "," + String(rightData.slot3) + "," + String(rightData.slot4) + "," + String(rightData.slot5) + "],";
        json += "\"current\":" + String(currentValue, 2) + ",";
        json += "\"power\":" + String(powerValue, 2) + ",";
        json += "\"occupied\":" + String(calculateOccupiedSlots()) + ",";
        json += "\"gateOpen\":" + String(gateOpen ? "true" : "false") + ",";
        json += "\"sysMode\":" + String(systemMode) + ",";
        json += "\"mode\":\"" + String(isOnlineMode ? "ONLINE" : "OFFLINE") + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });
    
    // API endpoint for connected nodes
    server.on("/api/nodes", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"connectedNodes\":[";
        for(int i = 0; i < peerCount; i++) {
            if(peers[i].registered) {
                if(i > 0) json += ",";
                json += "{";
                json += "\"mac\":\"" + formatMacAddress(peers[i].mac) + "\",";
                json += "\"side\":" + String(peers[i].side) + ",";
                json += "\"lastMessage\":" + String(peers[i].lastMessageTime) + ",";
                json += "\"lastMessageStr\":\"" + getTimeSinceLastMessage(peers[i].lastMessageTime) + "\"";
                json += "}";
            }
        }
        json += "]}";
        request->send(200, "application/json", json);
    });
    
    server.begin();
    addLog("Web server started");
    
    Serial.println("\n=================================");
    Serial.println("System Ready - Mode: " + String(isOnlineMode ? "ONLINE" : "OFFLINE"));
    Serial.println("=================================");
    Serial.println("\nConnect to WiFi: SmartParking");
    Serial.println("Password: 12345678");
    Serial.print("Web Interface: http://");
    Serial.println(WiFi.softAPIP());
    if(isOnlineMode) {
        Serial.print("OTA Updates: Upload via Arduino IDE (Host: SmartParking)");
        Serial.println(" | Password: smartpark123");
    }
    Serial.println("=================================\n");
}

void loop() {
    // Handle OTA updates
    ArduinoOTA.handle();
    
    // Read current sensor
    currentValue = readCurrent();
    voltageValue = 5.0;
    powerValue = currentValue * voltageValue;
    
    // Check gate control
    checkGateControl();
    
    // Update LCD
    if(millis() - lastLcdUpdate > LCD_UPDATE_INTERVAL) {
        lastLcdUpdate = millis();
        updateLCD();
    }
    
    // Check mode periodically
    if(millis() - lastModeCheck > MODE_CHECK_INTERVAL) {
        lastModeCheck = millis();
        determineSystemMode();
    }
    
    // Sync to cloud periodically or if pending
    if(isOnlineMode && (pendingCloudSync || (millis() - lastCloudSync > CLOUD_SYNC_INTERVAL))) {
        pendingCloudSync = false;
        lastCloudSync = millis();
        syncToCloud();
    }
    
    delay(50);
    
    // Periodic status
    static unsigned long lastStatus = 0;
    if(millis() - lastStatus > 30000) {
        lastStatus = millis();
        int occupied = calculateOccupiedSlots();
        Serial.printf("Status: Mode=%s | Occ=%d/10 | Gate=%s | Power=%.1fW\n",
                     isOnlineMode ? "ONLINE" : "OFFLINE",
                     occupied, gateOpen ? "OPEN" : "CLSD", powerValue);
        
        if (!isOnlineMode && offlineRecordCount > 0) {
            Serial.printf("Offline records stored: %d\n", offlineRecordCount);
        }
    }
}