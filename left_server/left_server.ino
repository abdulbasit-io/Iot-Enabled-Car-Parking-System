#include <esp_now.h>
#include <WiFi.h>

// Use broadcast address for ESP-NOW
// This will send to all ESP-NOW peers in range (including central server)
uint8_t broadcastMac[] = {0x24, 0x62, 0xAB, 0xE7, 0x65, 0xFC};

typedef struct sensor_data {
    int slot1;
    int slot2;
    int slot3;
    int slot4;
    int slot5;      // Added slot5 for 5 total slots
    bool sensorStatus[10];  // Now tracking 10 sensors (2 per slot)
    int side; // 0 for left, 1 for right
} sensor_data;

sensor_data myData;
esp_now_peer_info_t peerInfo;

// Ultrasonic pins for ESP32-WROOM-32
// Using safe GPIO pins (avoiding pins with special functions)
// Slot 1: Sensors 0-1, Slot 2: Sensors 2-3, Slot 3: Sensors 4-5, Slot 4: Sensors 6-7, Slot 5: Sensors 8-9
const int trigPins[10] = {13, 14, 26, 27, 15, 16, 17, 18, 19, 21};
const int echoPins[10] = {12, 25, 33, 32, 22, 23, 4, 5, 35, 34};

// Change this based on which side ESP32 this is
const int ESP_SIDE = 0; // 0 = Left, 1 = Right

// Send statistics
int sendSuccessCount = 0;
int sendFailCount = 0;
unsigned long lastSendTime = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=================================");
    Serial.println("Left Side Sensor Node (ESP32-WROOM-32)");
    Serial.println("Using ESP-NOW Broadcast Mode");
    Serial.println("=================================\n");
    
    // Initialize myData
    myData.side = ESP_SIDE;
    for(int i = 0; i < 10; i++) {
        myData.sensorStatus[i] = true;
    }
    myData.slot1 = myData.slot2 = myData.slot3 = myData.slot4 = myData.slot5 = 0;
    
    // Set ESP32 as WiFi Station
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // Set WiFi channel to match central server (channel 6)
    // This ensures broadcast packets are received on the correct channel
    WiFi.setChannel(6);
    
    // Print MAC Address for reference
    Serial.print("ESP32 MAC Address: ");
    Serial.println(WiFi.macAddress());
    Serial.println("Using Broadcast Mode - Sending to all ESP-NOW devices");
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("✗ Error initializing ESP-NOW");
        return;
    }
    Serial.println("✓ ESP-NOW initialized");
    
    // Register broadcast peer
    memcpy(peerInfo.peer_addr, broadcastMac, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("✗ Failed to add broadcast peer");
        return;
    } else {
        Serial.println("✓ Broadcast peer added successfully");
        Serial.println("Sending data in broadcast mode - All ESP-NOW devices will receive");
    }
    
    // Setup ultrasonic sensors
    for(int i = 0; i < 10; i++){
        pinMode(trigPins[i], OUTPUT);
        pinMode(echoPins[i], INPUT);
        digitalWrite(trigPins[i], LOW);
    }
    
    Serial.println("\n✓ All sensors initialized");
    Serial.print("Side: ");
    Serial.println(ESP_SIDE == 0 ? "Left" : "Right");
    Serial.println("Monitoring 5 slots with 2 sensors each (Total: 10 sensors)");
    Serial.println("\nPin Configuration:");
    for(int i = 0; i < 10; i++) {
        Serial.printf("Sensor %d (Slot %d, Sensor %d): TRIG=%d, ECHO=%d\n", 
                     i+1, (i/2)+1, (i%2)+1, trigPins[i], echoPins[i]);
    }
    Serial.println("\n=================================\n");
}

long readUltrasonic(int sensorIndex){
    digitalWrite(trigPins[sensorIndex], LOW);
    delayMicroseconds(2);
    digitalWrite(trigPins[sensorIndex], HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPins[sensorIndex], LOW);
    
    long duration = pulseIn(echoPins[sensorIndex], HIGH, 30000);
    
    if(duration == 0){
        return -1;  // No pulse received
    }
    
    long distance = duration * 0.034 / 2;
    return distance;
}

void loop() {
    // Arrays to store distances for all sensors
    long distances[10];
    bool sensorValid[10];
    
    // Read all 10 sensors
    for(int i = 0; i < 10; i++){
        distances[i] = readUltrasonic(i);
        
        // Check if sensor reading is valid
        if(distances[i] > 0 && distances[i] < 400) {  // Valid range up to 400cm
            sensorValid[i] = true;
            myData.sensorStatus[i] = true;
        } else {
            sensorValid[i] = false;
            myData.sensorStatus[i] = false;
        }
        
        // Print individual sensor data for debugging
        Serial.print("Sensor ");
        Serial.print(i+1);
        Serial.print(" (Slot ");
        Serial.print((i/2)+1);
        Serial.print(", Sensor ");
        Serial.print((i%2)+1);
        Serial.print("): ");
        if(sensorValid[i] && distances[i] > 0){
            Serial.print(distances[i]);
            Serial.print("cm - ");
            Serial.println(distances[i] < 30 ? "OCCUPIED" : "EMPTY");
        } else if(distances[i] == -1) {
            Serial.println("ERROR - No echo received");
        } else {
            Serial.println("INVALID - Out of range");
        }
    }
    
    // Determine occupancy for each slot (based on both sensors)
    myData.slot1 = 0;
    myData.slot2 = 0;
    myData.slot3 = 0;
    myData.slot4 = 0;
    myData.slot5 = 0;
    
    // Check each slot's occupancy using both sensors
    for(int slot = 0; slot < 5; slot++) {
        int sensor1Index = slot * 2;
        int sensor2Index = (slot * 2) + 1;
        
        // Slot is occupied if either sensor detects object within 30cm
        if((distances[sensor1Index] > 0 && distances[sensor1Index] < 30) ||
           (distances[sensor2Index] > 0 && distances[sensor2Index] < 30)) {
            switch(slot) {
                case 0: myData.slot1 = 1; break;
                case 1: myData.slot2 = 1; break;
                case 2: myData.slot3 = 1; break;
                case 3: myData.slot4 = 1; break;
                case 4: myData.slot5 = 1; break;
            }
        }
    }
    
    // Print slot occupancy summary
    Serial.println("\n=== SLOT OCCUPANCY SUMMARY ===");
    Serial.print("Slot 1: ");
    Serial.println(myData.slot1 ? "█ OCCUPIED" : "□ EMPTY");
    Serial.print("Slot 2: ");
    Serial.println(myData.slot2 ? "█ OCCUPIED" : "□ EMPTY");
    Serial.print("Slot 3: ");
    Serial.println(myData.slot3 ? "█ OCCUPIED" : "□ EMPTY");
    Serial.print("Slot 4: ");
    Serial.println(myData.slot4 ? "█ OCCUPIED" : "□ EMPTY");
    Serial.print("Slot 5: ");
    Serial.println(myData.slot5 ? "█ OCCUPIED" : "□ EMPTY");
    
    // Print sensor health status
    Serial.println("\nSensor Health:");
    for(int i = 0; i < 10; i++) {
        Serial.printf("Sensor %d: %s\n", i+1, myData.sensorStatus[i] ? "OK" : "FAIL");
    }
    
    // Send data via ESP-NOW broadcast
    esp_err_t result = esp_now_send(broadcastMac, (uint8_t *) &myData, sizeof(myData));
    
    if (result == ESP_OK) {
        sendSuccessCount++;
        Serial.println("\n✓ Data sent successfully (Broadcast mode)");
        Serial.printf("Send Statistics - Success: %d, Failed: %d\n", sendSuccessCount, sendFailCount);
        if(lastSendTime > 0) {
            Serial.printf("Time since last send: %d ms\n", millis() - lastSendTime);
        }
        lastSendTime = millis();
    } else {
        sendFailCount++;
        Serial.printf("\n✗ Error sending data! Error code: %d\n", result);
        Serial.printf("Send Statistics - Success: %d, Failed: %d\n", sendSuccessCount, sendFailCount);
        
        // Try to re-add broadcast peer if send keeps failing
        if(sendFailCount > 5) {
            Serial.println("Attempting to re-add broadcast peer...");
            esp_now_del_peer(broadcastMac);
            if (esp_now_add_peer(&peerInfo) != ESP_OK){
                Serial.println("✗ Failed to re-add peer");
            } else {
                Serial.println("✓ Peer re-added successfully");
                sendFailCount = 0;
            }
        }
    }
    
    Serial.println("================================\n");
    delay(3000); // Send data every 3 seconds
}
