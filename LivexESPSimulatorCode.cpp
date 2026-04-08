#include <esp_now.h>
#include <WiFi.h>

typedef struct __attribute__((packed)) {
  float lat;
  float lon;
  uint8_t id; 
} car_packet_t;

car_packet_t myData;        
car_packet_t incomingData;  
volatile bool hasIncoming = false;
uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {}

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

  if (esp_now_init() != ESP_OK) return;

  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddr, 6);
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Serial.println("System Ready. Send data in format: ID,LAT,LON (e.g., 1,40.71,-74.00)");
}

void loop() {
  // 1. RECEIVE LOGIC (From other ESPs)
  if (hasIncoming) {
    Serial.printf("REC|%d|%.6f|%.6f\n", incomingData.id, incomingData.lat, incomingData.lon);
    hasIncoming = false;
  }

  // 2. LISTEN TO COM PORT (From Computer)
  if (Serial.available() > 0) {
    // Read the incoming string until a newline
    String input = Serial.readStringUntil('\n');
    
    // Parse the string: Expecting "ID,Lat,Lon"
    int firstComma = input.indexOf(',');
    int secondComma = input.indexOf(',', firstComma + 1);

    if (firstComma != -1 && secondComma != -1) {
      myData.id = input.substring(0, firstComma).toInt();
      myData.lat = input.substring(firstComma + 1, secondComma).toFloat();
      myData.lon = input.substring(secondComma + 1).toFloat();

      // Send the parsed data via ESP-NOW
      esp_now_send(broadcastAddr, (uint8_t *) &myData, sizeof(myData));
      
      Serial.print(">> Sent to ESP-NOW: ");
      Serial.println(input);
    } else {
      Serial.println("!! Invalid Format. Use: ID,Lat,Lon");
    }
  }
}