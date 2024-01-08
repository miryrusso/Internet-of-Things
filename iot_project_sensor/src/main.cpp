#include <Arduino.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SPI.h>              // include libraries
#include <LoRa.h>

#include "esp_bt_main.h"
#include "esp_bt_device.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

//pin sensore
#define DHTPIN 4   

//pin lora
const long frequency = 866E6;  // LoRa Frequency

#define LORA_CS 18
#define LORA_RST 23
#define LORA_DI0 26


// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT_Unified dht(DHTPIN, DHTTYPE);

float temp = 0;

typedef struct  {
  int8_t IDDEV[3];
  float temp;
} packetLora;

packetLora tosend;

bool initBluetooth()
{
  if (!btStart()) {
    Serial.println("Failed to initialize controller");
    return false;
  }
 
  if (esp_bluedroid_init() != ESP_OK) {
    Serial.println("Failed to initialize bluedroid");
    return false;
  }
 
  if (esp_bluedroid_enable() != ESP_OK) {
    Serial.println("Failed to enable bluedroid");
    return false;
  }
 
}

void setup() {


  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.begin(115200);
  dht.begin();

  LoRa.setPins(LORA_CS, LORA_RST, LORA_DI0);

  while (!LoRa.begin(866E6)) {
      Serial.print(".");
      delay(500);
  }
  Serial.println();

  LoRa.setSyncWord(0xcc);
  Serial.println("LoRa Initializing OK!");

  initBluetooth();
  const uint8_t* point = esp_bt_dev_get_address();
  for (int i = 3; i < 6; i++) {
    tosend.IDDEV[i - 3] = (char)point[i];
  }
  Serial.printf("%02x\n", tosend.IDDEV);
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
}

void loop() {

  delay(2000);
  
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    temp = event.temperature;
    Serial.print(F("Temperature: "));
    Serial.print(temp);
    Serial.println(F("°C"));
    tosend.temp = temp;
    LoRa.beginPacket();
    uint8_t * buff = (uint8_t *)(&tosend);
    for(int i = 0; i < sizeof(packetLora); i++){
      LoRa.write(buff[i]);
    }
    LoRa.endPacket();

    delay(500);
    esp_deep_sleep_start();

  }

}
