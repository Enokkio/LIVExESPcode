#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// --- ESP-NOW STRUCT ---
typedef struct __attribute__((packed)) {
  uint64_t mac_id;//MAC ADRESS AS UNIQUE ID
  float lat;
  float lon;
  uint32_t timestamp; // HMMSS.SS format
  int ttl;

} car_packet_t;

std::unordered_map<uint64_t, car_packet_t> mac_table;
uint64_t ESP_MAC_ID = 0;


car_packet_t myData;          // To send
car_packet_t incomingData;    // To receive
volatile bool hasIncoming = false;
uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t lastSenderMac[6]; // To store the MAC of the car that just sent data
// BLE & GNSS (Keep your existing UUIDs and Pins)
#define RXD2 4
#define TXD2 25
HardwareSerial GNSS(1);
BLECharacteristic* pCharacteristic;
static char gnssLine[128];
static int gnssPos = 0;

void setup() {
  Serial.begin(115200);
  GNSS.begin(38400, SERIAL_8N1, RXD2, TXD2);
  
  //GET MAC ID AND STORE IN GLOBAL VARIABLE
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  memcpy(&ESP_MAC_ID, baseMac, 6);

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); 
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  // Neede for broadcasting
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddr, 6);
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  //Kinda like a listener to recieve data uses IRQ
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // BLE Setup (Your existing code)
  BLEDevice::init("ESP32_C5_GNSS");
  BLEServer* pServer = BLEDevice::createServer();
  BLEService* pService = pServer->createService("12345678-1234-5678-1234-56789abcdef0");
  pCharacteristic = pService->createCharacteristic("abcdefab-1234-5678-1234-abcdefabcdef", BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEDevice::getAdvertising()->start();
}

void OnDataRecv(const uint8_t * mac, const uint8_t *data, int len) {
  memcpy(&incomingData, data, sizeof(incomingData));  
  memcpy(lastSenderMac, mac, 6);
  hasIncoming = true; 
}

void loop() {
  handleWhile(); // Processes GNSS and sends MY data via ESP-NOW & BLE

  // Handle data received from OTHER cars, 
  handleIncomingData();
}

void handleIncomingData() {
    if (hasIncoming == true) {
        Serial.println("--- RECEIVED ESP-NOW DATA ---");
        Serial.printf("From Car ID: %llu\n", incomingData.mac_id);
        Serial.printf("Coordinates: %.6f, %.6f\n", incomingData.lat, incomingData.lon);
        Serial.printf("TTL: %d\n", incomingData.ttl);
        Serial.printf("Timestamp: %u\n", incomingData.timestamp);

        std::unordered_map<uint64_t, car_packet_t>::iterator it = mac_table.find(incomingData.mac_id);

        bool isOldData = false;
        if (it != mac_table.end()) {
            car_packet_t existingRecord = it->second;
            if (incomingData.timestamp <= existingRecord.timestamp) {
                isOldData = true;
            }
        }

        if (incomingData.mac_id == ESP_MAC_ID || incomingData.ttl <= 0 || isOldData) {
            Serial.println("Packet Dropped: Self-ID, Expired TTL, or Old Data.");
            hasIncoming = false;
            return;
        }


        Serial.println("Packet Accepted: Updating Map and Sending BLE.");
        mac_table[incomingData.mac_id] = incomingData;
        sendBLE(2, incomingData.lat, incomingData.lon);//SOME TEMPORARY NUMBER 2, BLUETOOHT END DOES NOT HANDLE MAC ADDRESS AS ID YET
        
        incomingData.ttl = incomingData.ttl - 1;
        sendEspNowBroadcast(incomingData);

        hasIncoming = false;
    }
}

void handleWhile() {

  while (GNSS.available()) {

    char c = GNSS.read();

    if (c == '\n' || c == '\r') {
      if (gnssPos > 0) {
        gnssLine[gnssPos] = 0; // Null terminate

        // Check for GGA sentence (Global Positioning System Fix Data)
        if (strstr(gnssLine, "GGA")) {
          char copy[128];
          strcpy(copy, gnssLine);
          
          char* token = strtok(copy, ",");
          int field = 0;
          float utcTime = 0; // Added utcTime
          // Variables to hold the extracted data
          float rawLat = 0, rawLon = 0;
          char latDir = 'N', lonDir = 'E';
          int fixQual = 0;
          int satsUsed = 0;
          float hdop = 0.0;

          while (token != NULL) {
            switch(field) {
              case 1: utcTime = atof(token); break; // HHMMSS.ss
              case 2: rawLat = atof(token); break;  // Latitude (DDMM.MMMM)
              case 3: latDir = token[0];    break;  // N or S
              case 4: rawLon = atof(token); break;  // Longitude (DDDMM.MMMM)
              case 5: lonDir = token[0];    break;  // E or W
              case 6: fixQual = atoi(token); break; // Fix Quality
              case 7: satsUsed = atoi(token); break;// Satellites
              case 8: hdop = atof(token); break;    // HDOP
            }
            token = strtok(NULL, ",");
            field++;
          }

          // --- CONVERSION TO DECIMAL DEGREES ---
          // Latitude conversion
          int latDeg = (int)(rawLat / 100);
          float latMin = rawLat - (latDeg * 100);
          float decimalLat = latDeg + (latMin / 60.0);
          if (latDir == 'S') decimalLat *= -1.0;

          // Longitude conversion
          int lonDeg = (int)(rawLon / 100);
          float lonMin = rawLon - (lonDeg * 100);
          float decimalLon = lonDeg + (lonMin / 60.0);
          if (lonDir == 'W') decimalLon *= -1.0;

          // --- OUTPUT & BROADCAST ---
    
          // Optional: Send to BLE using your existing helper
          if (fixQual > 0) {
          Serial.println("--- POSITIONING STATS ---");
          Serial.print(" Coordinates: "); Serial.print(decimalLat, 6); 
          Serial.print(", "); Serial.println(decimalLon, 6);
          Serial.print(" Sats in Use: "); Serial.println(satsUsed);
          Serial.print(" Fix Quality: "); Serial.println(fixQual);
          Serial.print(" Precision (HDOP): "); Serial.println(hdop);
          Serial.println("-------------------------");
            sendBLE(0, decimalLat, decimalLon); //NEED TO SEND ESP AS STRUCT ASWELL IN THIS FUNCTION, CURRENTLY JUST SENDING ID 0 FOR GPS DATA
          // Prepare ESP-NOW packet
          myData.mac_id = ESP_MAC_ID;
          myData.lat = decimalLat;
          myData.lon = decimalLon;
          myData.timestamp = (uint32_t)(utcTime);
          myData.ttl = 2;
          sendEspNowBroadcast(myData);
        } else {
            static unsigned long lastMsg = 0;
              if (millis() - lastMsg > 5000) {
                  Serial.print("GPS: Waiting for Fix... Raw: ");
                  Serial.println(gnssLine);
                  lastMsg = millis();
              }
          }
        }
        gnssPos = 0; // Reset buffer
      }
    } else if (gnssPos < sizeof(gnssLine) - 1) {
      gnssLine[gnssPos++] = c;
    }
  }
}
void sendBLE(uint8_t id, float lat, float lon) {
  uint8_t buffer[9];
  buffer[0] = id;
  memcpy(buffer + 1, &lat, sizeof(float));
  memcpy(buffer + 5, &lon, sizeof(float));

  pCharacteristic->setValue(buffer, sizeof(buffer));
  pCharacteristic->notify();

  Serial.print("BLE sent ID "); Serial.print(id);
  Serial.print(" -> Lat: "); Serial.print(lat, 6);
  Serial.print(", Lon: "); Serial.println(lon, 6);
}
void sendEspNowBroadcast(car_packet_t dataToSend) {
       esp_err_t result = esp_now_send(broadcastAddr, (uint8_t *) &dataToSend, sizeof(car_packet_t));

    if (result == ESP_OK) {
        Serial.printf("ESP-NOW Broadcast Sent: Car %llu, TTL %d\n", dataToSend.mac_id, dataToSend.ttl);
    } else {
        Serial.println("ESP-NOW Send Error");
    }
}