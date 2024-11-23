/*==========================================================================================================*/
/*                                             INCLUDE LIBRARIES                                            */
/*==========================================================================================================*/
#include <esp_now.h>
#include <WiFi.h>

/*==========================================================================================================*/
/*                                                LIST OF MAC                                               */
/*==========================================================================================================*/
uint8_t masterAddress[] = { 0x0C, 0xB8, 0x15, 0xE5, 0x41, 0x64 };
uint8_t slave2Address[] = { 0x0C, 0xB8, 0x15, 0xE5, 0x41, 0x38 };

/*==========================================================================================================*/
/*                                             GLOBAL VARIABLES                                             */
/*==========================================================================================================*/
long masterStartTime = 0;
unsigned long slave1TriggerTime = 0;
unsigned long masterToSlave1Time = 0;
bool irMonitorFlag = false;
const int irPin = 4;
const int ledPin = 16;
const char *message = "START";  // Define the message
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
    masterStartTime = millis();
    irMonitorFlag = true;
    digitalWrite(ledPin, LOW);  //Turn on the LED
  }
  // Check if the message begins with "FINISH"
  if (strncmp(incomingMessage, "FINISH", 6) == 0) {  // Compare only the first 6 characters
    char updatedMessage[40];                         // Buffer to hold the updated message

    // Format the new message with the appended unsigned long value
    snprintf(updatedMessage, sizeof(updatedMessage), "%s,%lu", incomingMessage, masterToSlave1Time);

    // Send the updated message back or to another peer
    Serial.print("Updated message to send: ");
    Serial.println(updatedMessage);

    // Send the message to masterAddress
    if (sendMessage(masterAddress, updatedMessage)) {
      Serial.println("Message successfully sent to Master!");
    } else {
      Serial.println("Message failed to send to Master.");
    }
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
    Serial.println("Message sent successfully");
  } else {
    Serial.println("Failed to send message");
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
  addPeer(masterAddress);
  addPeer(slave2Address);


  Serial.println("Slave 1 Initialized.");
  digitalWrite(ledPin, HIGH);  //Turn off the LED
}

/*==========================================================================================================*/
/*                                                LOOP                                                      */
/*==========================================================================================================*/
void loop() {
  //Monitor the IR Sensor after START is received from Master
  if (irMonitorFlag == true && digitalRead(irPin) == LOW) {
    slave1TriggerTime = millis();

    // Send the message to slave2Address
    if (sendMessage(slave2Address, message)) {
      Serial.println("Message successfully sent to Slave 2!");
    } else {
      Serial.println("Message failed to send to Slave 2.");
    }
    irMonitorFlag = false;
    digitalWrite(ledPin, HIGH);  //Turn off the LED
    masterToSlave1Time = slave1TriggerTime - masterStartTime;
    Serial.print("Master to Slave 1 :");
    Serial.println(masterToSlave1Time);
  }
}