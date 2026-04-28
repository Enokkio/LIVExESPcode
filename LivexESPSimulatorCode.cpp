#include <esp_now.h>
#include <WiFi.h>
#include <unordered_map>

typedef struct __attribute__((packed)) {
  uint64_t mac_id;
  float lat;
  float lon;
  uint32_t timestamp; 
  int ttl;
  uint32_t seqNum;
} car_packet_t;

car_packet_t myData;        
car_packet_t incomingData;  
volatile bool hasIncoming = false;
uint32_t globalSeqNum = 0; 

uint64_t ESP_MAC_ID = 0; 
std::unordered_map<uint64_t, car_packet_t> mac_table;
uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len >= sizeof(car_packet_t)) {
    memcpy(&incomingData, data, sizeof(car_packet_t));
    hasIncoming = true;
  }
}

void sendEspNowBroadcast(car_packet_t data) {
  esp_now_send(broadcastAddr, (uint8_t *) &data, sizeof(car_packet_t));
}

void handleIncomingData() {
  if (hasIncoming == true) {
    std::unordered_map<uint64_t, car_packet_t>::iterator it = mac_table.find(incomingData.mac_id);

    bool isOldData = false;
    if (it != mac_table.end()) {
      if (incomingData.seqNum <= it->second.seqNum) {
        isOldData = true;
      }
    }
// Serial.printf("DEBUG | Is Me: %d | Is TTL Expired: %d | Is Old: %d\n", 
//               (incomingData.mac_id == ESP_MAC_ID), 
//               (incomingData.ttl <= 0), 
//               isOldData);
    if (incomingData.mac_id == ESP_MAC_ID || incomingData.ttl <= 0 || isOldData) {
      Serial.printf("DROP | ID: %llu | TTL: %d | Old: %d | Seq: %u\n", 
              (unsigned long long)incomingData.mac_id, 
              incomingData.ttl, 
              (int)isOldData, 
              incomingData.seqNum);      
              hasIncoming = false;
      return;
    }

    mac_table[incomingData.mac_id] = incomingData;
    // Serial.printf("REC | ID: %llu | LAT: %.6f | LON: %.6f | Seq: %u\n", 
    //           (unsigned long long)incomingData.mac_id, 
    //           incomingData.lat, 
    //           incomingData.lon, 
    //           incomingData.seqNum);
    incomingData.ttl = incomingData.ttl - 1;
    if (incomingData.ttl > 0) {
      sendEspNowBroadcast(incomingData);
    }

    hasIncoming = false;
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  // Fixed: Newer way to get MAC as uint64
  uint8_t mac[6];
  WiFi.macAddress(mac);
  memcpy(&ESP_MAC_ID, mac, 6);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  // Fixed: Cast to required types for modern ESP32 core
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddr, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  }
}

void loop() {
  handleIncomingData();

  if (Serial.available() > 0) {
    Serial.println("ENTERD SERIAL.AVAILABLE");
    String input = Serial.readStringUntil('\n');
    
    int c1 = input.indexOf(',');
    int c2 = input.indexOf(',', c1 + 1);
    int c3 = input.indexOf(',', c2 + 1);
    int c4 = input.indexOf(',', c3 + 1);

    if (c1 != -1 && c2 != -1 && c3 != -1 && c4 != -1) {
      myData.mac_id    = ESP_MAC_ID;
      myData.lat       = input.substring(c1 + 1, c2).toFloat();
      myData.lon       = input.substring(c2 + 1, c3).toFloat();
      myData.timestamp = (uint32_t)input.substring(c3 + 1, c4).toInt();
      myData.ttl       = input.substring(c4 + 1).toInt();
      globalSeqNum++;     // Increment global sequence number for next packet
      myData.seqNum = globalSeqNum;
      sendEspNowBroadcast(myData);
    }
  }
}