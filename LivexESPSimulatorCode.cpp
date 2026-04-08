#include <esp_now.h>
#include <WiFi.h>

// --- THE STRUCT (Must be identical on all devices) ---
typedef struct __attribute__((packed)) {
  float lat;
  float lon;
  uint8_t id; 
} car_packet_t;

car_packet_t myData;        // To send
car_packet_t incomingData;  // To receive
volatile bool hasIncoming = false;
uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// --- NEW API: SEND CALLBACK ---
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Useful for debugging if the radio actually fired
}

// --- NEW API: RECEIVE CALLBACK ---
// Note the first argument is now the 'info' struct
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *data, int len) {
  if (len >= sizeof(car_packet_t)) {
    memcpy(&incomingData, data, sizeof(incomingData));
    hasIncoming = true;
  }
}

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); 

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  // Registering with explicit casts to satisfy the compiler
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Add Broadcast Peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddr, 6);
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  // 1. RECEIVE LOGIC
  if (hasIncoming) {
    Serial.println("--- RECEIVED FROM OTHER DEVICE ---");
    Serial.printf("ID: %d | Lat: %.6f | Lon: %.6f\n", incomingData.id, incomingData.lat, incomingData.lon);
    hasIncoming = false;
  }

  // 2. SEND LOGIC (Sending "Dummy" data every 3 seconds)
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 3000) {
    myData.id = 99; // Give this device a unique ID
    myData.lat = 40.7128; 
    myData.lon = -74.0060;

    esp_now_send(broadcastAddr, (uint8_t *) &myData, sizeof(myData));
    Serial.println(">> Sent my data via ESP-NOW");
    lastSend = millis();
  }
}