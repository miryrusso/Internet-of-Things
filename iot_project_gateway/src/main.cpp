#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <MQTTClient.h>

const char* ssid = "S23";
const char* password = "123test133";

WiFiClient net;
MQTTClient client;

#define LORA_CS 18
#define LORA_RST 23
#define LORA_DI0 26

typedef struct  {
  int8_t IDDEV[3];
  float temp;
} packetLora;

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void setup() {


  Serial.begin(115200);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DI0);

  while (!LoRa.begin(866E6)) {
      Serial.print(".");
      delay(500);
  }
  Serial.println();

  LoRa.setSyncWord(0xcc);
  Serial.println("LoRa Initializing OK!");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  client.begin("broker.hivemq.com", 1883, net);
  client.onMessage(messageReceived);
  while (!client.connect("UniCTGW")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("MQTT Connected");

}

packetLora packet;
char buff[sizeof(packetLora)];

void loop() {
  client.loop();
  // put your main code here, to run repeatedly:
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.println("Test ricezione");
    if(LoRa.available()){
      LoRa.readBytes((uint8_t *)&packet, sizeof(packetLora));
      // packetLora* packet = (packetLora*)buff;

      Serial.println("Messaggio ricevuto");
      Serial.printf("ID Sensore: %02x\n", packet.IDDEV);
      Serial.println(packet.temp);
      
      sprintf(buff, "%02x:%g", packet.IDDEV, packet.temp);
      Serial.println(buff);
      client.publish("unict/temp",buff);

    }
  }
}

