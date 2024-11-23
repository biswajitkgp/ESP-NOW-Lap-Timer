/*==========================================================================================================*/
/*                                             INCLUDE LIBRARIES                                            */
/*==========================================================================================================*/
#include <esp_now.h>
#include <WiFi.h>

/*==========================================================================================================*/
/*                                                LIST OF MAC                                               */
/*==========================================================================================================*/

uint8_t slave2Address[] = { 0x0C, 0xB8, 0x15, 0xE5, 0x41, 0x38 };

/*==========================================================================================================*/
/*                                             GLOBAL VARIABLES                                             */
/*==========================================================================================================*/
long slave2StartTime = 0;
unsigned long slave3TriggerTime = 0;
unsigned long slave2ToSlave3Time = 0;
bool irMonitorFlag = false;
const int irPin = 4;
const int ledPin = 16;
char finishmessage[40];  // Define your message
/*==========================================================================================================*/
/*                                      Function to add Peers                                               */
/*==========================================================================================================*/
bool addPeer(const uint8_t *peerAddress, uint8_t channel = 0, bool encrypt = false) {
  esp_now_peer_info_t peerInfo = {};  // Initialize to zero
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = channel;
  peerInfo.encrypt = encrypt;

  // Set the interface explicitly
  peerInfo.ifidx = WIFI_IF_STA;

  // Check if the peer already exists
  if (esp_now_is_peer_exist(peerAddress)) {
    Serial.println("Peer already exists.");
    return true;
  }

  // Add the peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer.");
    return false;
  }

  Serial.println("Peer added successfully.");
  return true;
}

/*==========================================================================================================*/
/*                               Callback function for receiving data                                       */
/*==========================================================================================================*/
void onReceive(const esp_now_recv_info *recv_info, const uint8_t *data, int data_len) {
  char incomingMessage[data_len + 1];
  memcpy(incomingMessage, data, data_len);
  incomingMessage[data_len] = '\0';  // Null-terminate the message

  Serial.print("Received message: ");
  Serial.println(incomingMessage);

  //Check if START is received
  if (String(incomingMessage) == "START") {
    slave2StartTime = millis();
    irMonitorFlag = true;
    digitalWrite(ledPin, LOW);  //Turn on the LED
  }
}
/*==========================================================================================================*/
/*                                Function to Send ESP-NOW Messages                                         */
/*==========================================================================================================*/

bool sendMessage(const uint8_t *peerAddress, const char *message) {
  // Send the message
  esp_err_t result = esp_now_send(peerAddress, (uint8_t *)message, strlen(message));

  // Check if the message was sent successfully
  if (result == ESP_OK) {
    Serial.println("Message sent successfully to: ");
  } else {
    Serial.println("Failed to send message to: ");
  }

  return result == ESP_OK;
}

/*==========================================================================================================*/
/*                                                   SETUP                                                  */
/*==========================================================================================================*/
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  pinMode(irPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  //Turn on the LED

  // Initialize WiFi in STA mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    while (true)
      ;
  }

  // Register receive callback
  esp_now_register_recv_cb(onReceive);


  // Add Peers
  addPeer(slave2Address);
  Serial.println("Slave 3 initialized.");
  digitalWrite(ledPin, HIGH);  //Turn off the LED
}

/*==========================================================================================================*/
/*                                                LOOP                                                      */
/*==========================================================================================================*/
void loop() {
  //Monitor the IR Sensor after START is received from Master
  if (irMonitorFlag == true && digitalRead(irPin) == LOW) {
    slave3TriggerTime = millis();
    irMonitorFlag = false;
    digitalWrite(ledPin, HIGH);  //Turn off the LED
    slave2ToSlave3Time = slave3TriggerTime - slave2StartTime;
    Serial.print("Slave 2 to Slave 3 :");
    Serial.println(slave2ToSlave3Time);

    snprintf(finishmessage, sizeof(finishmessage), "FINISH:%lu", slave2ToSlave3Time);
    // Send the message to slave2Address
    if (sendMessage(slave2Address, finishmessage)) {
      Serial.println("Message successfully sent to Slave 2!");
    } else {
      Serial.println("Message failed to send to Slave 2.");
    }
  }
}
