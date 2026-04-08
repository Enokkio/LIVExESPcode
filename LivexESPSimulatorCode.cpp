#include <esp_now.h>
#include <WiFi.h>

// --- THE STRUCT MUST MATCH THE RECEIVER EXACTLY ---
typedef struct __attribute__((packed)) {
  float lat;
  float lon;
  uint8_t id; 
} car_packet_t;

car_packet_t myData;

// Broadcast address (sends to all ESP32s on the same channel)
uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Callback to see if the message was actually sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the send callback
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer (Broadcast)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddr, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // Set dummy data to send
  myData.id = 7;             // Example Car ID
  myData.lat = 51.5074 + ((float)random(-100, 100) / 10000.0); // Randomize slightly
  myData.lon = -0.1278 + ((float)random(-100, 100) / 10000.0);

  Serial.printf("Sending: ID %d, Lat %.6f, Lon %.6f\n", myData.id, myData.lat, myData.lon);

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddr, (uint8_t *) &myData, sizeof(myData));
   
  if (result != ESP_OK) {
    Serial.println("Error sending the data");
  }

  delay(2000); // Send every 2 seconds
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
