/*==========================================================================================================*/
/*                                             INCLUDE LIBRARIES                                            */
/*==========================================================================================================*/
#include <esp_now.h>
#include <WiFi.h>

/*==========================================================================================================*/
/*                                                LIST OF MAC                                               */
/*==========================================================================================================*/

uint8_t slave1Address[] = { 0x0C, 0xB8, 0x15, 0xE5, 0x41, 0xA0 };

/*==========================================================================================================*/
/*                                             GLOBAL VARIABLES                                             */
/*==========================================================================================================*/
const int hooterPin = 25;
const int hooterDuration = 200;
const int ledPin = 16;
const char *message = "START";  // Define the message
/*==========================================================================================================*/
/*                      Callback function for receiving data (Parsing Finish Message)                       */
/*==========================================================================================================*/

void onReceive(const esp_now_recv_info *recv_info, const uint8_t *data, int data_len) {
  char incomingMessage[data_len + 1];
  memcpy(incomingMessage, data, data_len);
  incomingMessage[data_len] = '\0';
  // Check if the message begins with "FINISH"
  if (strncmp(incomingMessage, "FINISH", 6) == 0) {  // Compare only the first 6 characters

    // Strip "FINISH:" from the message
    char *valuesPart = incomingMessage + 7;  // Skip "FINISH:" part

    // Extract values
    char *token = strtok(valuesPart, ",");
    int values[3];
    int index = 0;

    while (token != NULL && index < 3) {
      values[index] = atoi(token);  // Convert the token to an integer
      token = strtok(NULL, ",");
      index++;
    }

    // Ensure we have all three values
    if (index == 3) {
      // Print the formatted output
      Serial.printf("Master to Slave 1: %.3f\n", values[2] / 1000.0);
      Serial.printf("Slave 1 to Slave 2: %.3f\n", values[1] / 1000.0);
      Serial.printf("Slave 2 to Slave 3: %.3f\n", values[0] / 1000.0);
    } else {
      Serial.println("Error: Incorrect number of values in the message.");
    }
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
    Serial.print("Message sent successfully");
  } else {
    Serial.print("Failed to send message");
  }
  return result == ESP_OK;
}


/*==========================================================================================================*/
/*                                                  SETUP                                                   */
/*==========================================================================================================*/

void setup() {
  Serial.begin(115200);

  pinMode(hooterPin, OUTPUT);
  digitalWrite(hooterPin, LOW);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

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

  // Add peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, slave1Address, 6);
  peerInfo.channel = 0;  // Default channel
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    while (true)
      ;
  }

  Serial.println("Master Initialized. Type 'START' to send message.");
  digitalWrite(ledPin, LOW);  //Turn on the LED
}

/*==========================================================================================================*/
/*                                                LOOP                                                      */
/*==========================================================================================================*/


void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();  // Remove any trailing newlines or spaces

    if (input == "START") {

      digitalWrite(hooterPin, HIGH);  //Turn on the Hooter
      digitalWrite(ledPin, HIGH);     //Turn off the LED

      // Send the START message
      if (sendMessage(slave1Address, message)) {
        Serial.println("Message sent successfully.");
      } else {
        Serial.println("Failed to send message.");
      }
      delay(hooterDuration);
      digitalWrite(hooterPin, LOW);
    } else {
      Serial.println("Invalid input. Type 'START' to send.");
    }
  }
}
