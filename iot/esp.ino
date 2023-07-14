#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

#define WIFI_SSID "xxxxxxx" // wifi ssid
#define WIFI_PASS "xxxxxxx" // wifi password

#define SS_PIN    2  // ss at mfrc522 
#define RST_PIN   27 // rst at mfrc522
#define RELAY_PIN 22 // relay

MFRC522 rfid(SS_PIN, RST_PIN);

String keyTagUIDString;
String urlGet;
String is_granted;
String card_holder;

void setupWiFi() {
    Serial.printf("\r\n[WIFI]: Connecting");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(250);  
    }

    Serial.println("CONNECTED!");
}

String uidBytesToString(byte byte0, byte byte1, byte byte2, byte byte3) {
  String uidString;
  byte bytes[] = {byte0, byte1, byte2, byte3};
  for (int i = 0; i < sizeof(bytes); i++) {
    if (bytes[i] < 0x10) {
      uidString += "0";  // Just in case
    }
    uidString += String(bytes[i], HEX);  // hex mode
  }
  return uidString;
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
  http.begin(serverName);
  int httpResponseCode = http.GET();
  String payload = "{}"; 
  if (httpResponseCode>0) {
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.println(httpResponseCode);
  }
  http.end();
  return payload;
}

void setup() {
  Serial.begin(9600);
  Serial.printf("\r\n\r\n");    
  SPI.begin(); // init SPI
  rfid.PCD_Init(); // init MFRC522
  rfid.PCD_DumpVersionToSerial(); // dump MFRC522 version
  pinMode(RELAY_PIN, OUTPUT); // init output pin
  digitalWrite(RELAY_PIN, LOW); // init lock
  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
  setupWiFi();
}

void loop() {
  try {
    if (rfid.PICC_IsNewCardPresent()) {
      if (rfid.PICC_ReadCardSerial()) {
        keyTagUIDString = ""; 
        is_granted = "";
        card_holder = "";
        MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak); // get PICC type

        urlGet = "http://10.0.1.8:3000/auth?card_id=";
        keyTagUIDString = uidBytesToString(rfid.uid.uidByte[0], rfid.uid.uidByte[1], rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
        urlGet.concat(keyTagUIDString);
        JSONVar jsonObject = JSON.parse(httpGETRequest(urlGet.c_str()));
        if (JSON.typeof(jsonObject) == "undefined") {
          Serial.println("Parsing input failed!");
          return;
        }
        JSONVar keys = jsonObject.keys();
        for (int i = 0; i < keys.length(); i++) {
          JSONVar value = jsonObject[keys[i]];
          Serial.println(JSON.stringify(keys[i]));
          if(JSON.stringify(keys[i]) == "\"granted\""){
            is_granted = JSON.stringify(value);
          } else if (JSON.stringify(keys[i]) == "\"card_holder\""){
            card_holder = JSON.stringify(value);
          }
        }

        urlGet = "http://10.0.1.8:3000/log?card_holder=";
        if (is_granted == "\"true\"") {
          Serial.println();
          Serial.print("Card UID:");
          Serial.print(rfid.uid.uidByte[0], HEX);
          Serial.print(rfid.uid.uidByte[1], HEX);
          Serial.print(rfid.uid.uidByte[2], HEX);
          Serial.println(rfid.uid.uidByte[3], HEX);
          Serial.print("Access is granted to ");
          Serial.println(card_holder);
          digitalWrite(RELAY_PIN, HIGH);  // unlock the door
          while(rfid.PICC_IsNewCardPresent()){ // wait until card is removed
            rfid.PICC_HaltA(); // halt PICC
            delay(100);
          }
          delay(2000);
          digitalWrite(RELAY_PIN, LOW); // lock the door
          urlGet.concat(String(card_holder));
          httpGETRequest(urlGet.c_str());
        } else {
          Serial.print("Access denied, UID:");
          for (int i = 0; i < rfid.uid.size; i++) {
            Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
            Serial.print(rfid.uid.uidByte[i], HEX);
          }
          Serial.println();
          urlGet.concat("UNAUTHORIZED");
          httpGETRequest(urlGet.c_str());
        }
      }
    }
    delay(500);
  } catch (...) {
    Serial.println("Access denied, Error");
  }
}