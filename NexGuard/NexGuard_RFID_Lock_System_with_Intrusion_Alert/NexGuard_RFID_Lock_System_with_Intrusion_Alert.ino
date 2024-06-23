#include "Arduino.h"
#include <EMailSender.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>

const char* ssid = "shiven";
const char* password = "shivenpatel";

const int reedSwitchPin = D1;  // GPIO pin where the reed switch is connected
const uint8_t RST_PIN = D3;     // Configurable, see typical pin layout above
const uint8_t SS_PIN = D4;     // Configurable, see typical pin layout above
const uint8_t RELAY_PIN = D2;   // Relay control pin connected to D2
const uint8_t BUZZER_PIN = D8;  // Buzzer control pin connected to D8

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

EMailSender emailSend("esp8266.adit.cp@gmail.com", "osmy hbzf jery ahhh");

uint8_t WiFiConnect(const char* nSSID = nullptr, const char* nPassword = nullptr) {
  static uint16_t attempt = 0;
  Serial.print("Connecting to ");
  if (nSSID) {
    WiFi.begin(nSSID, nPassword);
    Serial.println(nSSID);
  }

  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 50) {
    delay(200);
    Serial.print(".");
  }
  ++attempt;
  Serial.println("");
  if (i == 51) {
    Serial.print("Connection: TIMEOUT on attempt: ");
    Serial.println(attempt);
    if (attempt % 2 == 0)
      Serial.println("Check if access point available or SSID and Password\r\n");
    return false;
  }
  Serial.println("Connection: ESTABLISHED");
  Serial.print("Got IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(reedSwitchPin, INPUT_PULLUP); // Configure reed switch pin as input with internal pull-up resistor
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT); // Set buzzer pin as output

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  if (!WiFiConnect(ssid, password)) {
    Serial.println("Failed to connect to WiFi");
    while (true) {}  // Loop indefinitely if WiFi connection fails
  }
}

void loop() {
  // Check the state of the reed switch
  int doorState = digitalRead(reedSwitchPin);

  if (!rfid.PICC_IsNewCardPresent()) {

    // Check for intrusion (door open without RFID access)
    if (digitalRead(reedSwitchPin) == HIGH) { // Door is opened and no valid RFID card detected.

      // Activate the buzzer for 2 seconds
      digitalWrite(BUZZER_PIN, HIGH); // Turn on the buzzer
      delay(2000);
      digitalWrite(BUZZER_PIN, LOW); // Turn off the buzzer

      Serial.println("Intrtusion Alert!");
      Serial.println("doorState:");
      Serial.println(doorState);

      // Send email for intrusion
      EMailSender::EMailMessage message;
      message.subject = "Intrusion Alert !";
      message.message = "Door has been opened unauthentically !";
      EMailSender::Response resp = emailSend.send("shivenpatel1963@gmail.com", message);
      Serial.println("Sending status: ");
      Serial.println(resp.status);
      Serial.println(resp.code);
      Serial.println(resp.desc);

      // Wait for a while to prevent continuous sending of emails
      delay(10000);  // Adjust delay as per your requirement
    }
    return;
  }

  if (rfid.PICC_ReadCardSerial()) {
    String tag;
    for (byte i = 0; i < 4; i++) {
      tag += rfid.uid.uidByte[i];
    }
    Serial.println(tag);
    if (tag == "222974629") { // Your authorized tag number
      Serial.println("Access Granted!");
      digitalWrite(RELAY_PIN, HIGH); // Turn on relay to open the lock
      delay(5000); // Keep the lock open for 5 seconds
      digitalWrite(RELAY_PIN, LOW); // Turn off relay
      //      lockStatus = 1;

      // go ahead if the door is lock after opening authentically.
      while (digitalRead(reedSwitchPin) == HIGH) {
        Serial.println("Door is still open, Please close it!");
        if (digitalRead(reedSwitchPin) == LOW) {
          break;
        }
        delay(3000);
      }
    } else {
      Serial.println("Access Denied!");
      // Activate the buzzer for 2 seconds
      digitalWrite(BUZZER_PIN, HIGH); // Turn on the buzzer
      delay(2000);
      digitalWrite(BUZZER_PIN, LOW); // Turn off the buzzer

      // Send email for unauthorized access
      EMailSender::EMailMessage message;
      message.subject = "Unauthorized Access Alert !";
      message.message = "Unauthorized access attempt detected!";
      EMailSender::Response resp = emailSend.send("shivenpatel1963@gmail.com", message);
      Serial.println("Sending status: ");
      Serial.println(resp.status);
      Serial.println(resp.code);
      Serial.println(resp.desc);
    }
    tag = "";
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  delay(1000);  // Add a small delay to avoid excessive checking
}
