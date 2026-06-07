#include <esp_now.h>
#include <WiFi.h>

// REPLACE WITH THE MAC ADDRESS OF YOUR CENTRAL ESP32
uint8_t centralMac[] = {0x24, 0x62, 0xAB, 0xE7, 0x65, 0xFD};

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
const int ESP_SIDE = 1; // 0 = Left, 1 = Right

// Function to determine slot occupancy based on both sensors in that slot
int checkSlotOccupancy(int slotIndex) {
    int sensor1Index = slotIndex * 2;
    int sensor2Index = (slotIndex * 2) + 1;
    
    long distance1 = 0;
    long distance2 = 0;
    
    // Read both sensors in the slot
    distance1 = readUltrasonic(sensor1Index);
    distance2 = readUltrasonic(sensor2Index);
    
    // Check if either sensor detects an object within 30cm
    if ((distance1 > 0 && distance1 < 30) || (distance2 > 0 && distance2 < 30)) {
        return 1;  // Slot occupied
    }
    return 0;  // Slot empty
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize myData
    myData.side = ESP_SIDE;
    for(int i = 0; i < 10; i++) {
        myData.sensorStatus[i] = true;
    }
    myData.slot1 = myData.slot2 = myData.slot3 = myData.slot4 = myData.slot5 = 0;
    
    // Set ESP32 as WiFi Station
    WiFi.mode(WIFI_STA);
    
    // Print MAC Address for reference
    Serial.print("ESP32 MAC Address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Target MAC Address: ");
    char targetMacStr[18];
    snprintf(targetMacStr, sizeof(targetMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             centralMac[0], centralMac[1], centralMac[2],
             centralMac[3], centralMac[4], centralMac[5]);
    Serial.println(targetMacStr);
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    
    // Register peer
    memcpy(peerInfo.peer_addr, centralMac, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer");
        return;
    } else {
        Serial.println("Peer added successfully");
    }
    
    // Setup ultrasonic sensors
    for(int i = 0; i < 10; i++){
        pinMode(trigPins[i], OUTPUT);
        pinMode(echoPins[i], INPUT);
        digitalWrite(trigPins[i], LOW);
    }
    
    Serial.println("ESP32-WROOM-32 Sensor Node Started");
    Serial.print("Side: ");
    Serial.println(ESP_SIDE == 0 ? "Left" : "Right");
    Serial.println("Monitoring 5 slots with 2 sensors each (Total: 10 sensors)");
    Serial.println("\nPin Configuration:");
    for(int i = 0; i < 10; i++) {
        Serial.printf("Sensor %d (Slot %d, Sensor %d): TRIG=%d, ECHO=%d\n", 
                     i+1, (i/2)+1, (i%2)+1, trigPins[i], echoPins[i]);
    }
    Serial.println();
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
    
    // Send data via ESP-NOW
    esp_err_t result = esp_now_send(centralMac, (uint8_t *) &myData, sizeof(myData));
    
    if (result == ESP_OK) {
        Serial.println("\n✓ Data sent successfully");
    } else {
        Serial.println("\n✗ Error sending data");
    }
    
    Serial.println("================================\n");
    delay(3000); // Send data every 3 seconds
}
