/**
 * Semafori comunicanti tramite Bluetooth (1:1)
 * Attention: set own MACSTRING
 */
#include "BluetoothSerial.h"

/*
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
  #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Check Serial Port Profile
#if !defined(CONFIG_BT_SPP_ENABLED)
  #error Serial Port Profile for Bluetooth is not available or not enabled. It is only available for the ESP32 chip.
#endif
*/

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <Update.h>
#include <vector>
#include <string>
#include <sstream>

#define MASTER 0
uint8_t turn = 1;
uint8_t started = 0;

#define RED_LED 3
#define YELLOW_LED 4
#define GREEN_LED 0

#define GREEN_TIME 4000
#define YELLOW_TIME 2000
#define RED_TIME 2000

#define START 1

typedef enum
{
  IDLE,
  GREEN,
  YELLOW,
  RED
} sem_state;

sem_state state = IDLE;

String myName = "NameString";
// String myMac = "64:B7:08:8C:3B:52";

std::vector<std::string> devices;
BluetoothSerial SerialBT;

// Converts MAC String to a uint8_t array
uint8_t *macStrToArray(std::string macAddr)
{

  uint8_t *byteArray = new uint8_t[6];

  std::istringstream iss(macAddr);
  std::string hexByte;
  for (size_t i = 0; i < 6; ++i)
  {
    if (!std::getline(iss, hexByte, ':'))
    {
      // Handle the case where there are not enough bytes in the string
      throw std::invalid_argument("Invalid MAC address string");
      return nullptr;
    }
    byteArray[i] = std::stoi(hexByte, (size_t *)nullptr, 16);
  }

  return byteArray;
}

void Bt_Status(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  if (event == ESP_SPP_OPEN_EVT)
  {
  }
  else if (event == ESP_SPP_CLOSE_EVT)
  {
    Serial.println("Client Disconnected");
  }
}

void setup()
{

  Serial.begin(115200);
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  SerialBT.register_callback(Bt_Status);
  SerialBT.begin(myName, MASTER);
  SerialBT.setPin("5678");

  if (!MASTER)
    return; // skip if not master

  devices.push_back("D4:D4:DA:9E:92:56");
  // devices.push_back("64:B7:08:8C:3F:EA");

  bool connected = false;
  size_t conn_num = 0;
  uint8_t max_tries;
  while (conn_num != devices.size())
  {

    uint8_t *byteArray = macStrToArray(devices.at(conn_num));

    max_tries = 15;
    while (max_tries > 0)
    { // try to connect a number of times equal to max_tries

      Serial.printf("[%d] Connecting to slave BT device with MAC %s\n", (15 - max_tries), devices.at(conn_num).c_str());
      if ((connected = SerialBT.connect(byteArray)))
      {

        Serial.println("Connected Successfully!");
        conn_num++;
        break;
      }
      else
      {

        Serial.println("Retrying Connection");
        sleep(1);
        max_tries--;
      }
    }
    if (max_tries == 0)
    {

      exit(1); // *procede a morire*
    }
    delete byteArray;
  }

  // send start message to SLAVE
  Serial.println("SENT START MESSAGE TO SLAVE");
  SerialBT.write((uint8_t)START);
  state = RED;
}

void handle_case(int ledOff, int ledOn, uint32_t delay_ms, sem_state nextState = IDLE)
{
  digitalWrite(ledOff, LOW);
  digitalWrite(ledOn, HIGH);
  delay(delay_ms);
  state = nextState;
}

void loop()
{

  switch (state)
  {

  case IDLE:
    Serial.println("CASE IDLE, WAITING FOR START MESSAGE");
    // waiting for INIT from master
    while (true)
    {
      if (SerialBT.available())
      {
        uint8_t cmd = (uint8_t)SerialBT.read();
        if (cmd == START)
        {
          Serial.println("START RECEIVED");
          if (!started || turn == 0)
          {
            if (!started)
              started = 1;
            state = GREEN;
            turn = 1;
            break;
          }
        }
      }
      delay(10);
    }
    break;

  case GREEN:
    Serial.println("CASE GREEN");
    handle_case(RED_LED, GREEN_LED, GREEN_TIME, YELLOW);
    break;

  case YELLOW:
    Serial.println("CASE YELLOW");
    handle_case(GREEN_LED, YELLOW_LED, YELLOW_TIME, RED);
    break;

  case RED:
    Serial.println("CASE RED");
    handle_case(YELLOW_LED, RED_LED, RED_TIME, IDLE);
    if (turn)
    {
      SerialBT.write((uint8_t)START); // wait
      turn = 0;
    }

    break;

  default:
    break;
  }
}