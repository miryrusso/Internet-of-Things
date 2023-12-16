

#include "sdkconfig.h"
#include <TFT_eSPI.h>
#include "mrf24j.h"


/*
G18 = SCK
G34 = SDI
G35 = SDO
G23 = CS
G22 = INT
G21 = Wakeup
G19 = Reset
*/

#define MISO    26
#define MOSI    32
#define SCLK    18
#define CS      23
#define INT     22
#define WAKEUP  21
#define RST     19

typedef struct {
    short x;
    short y;
    short deg100;
    unsigned char flags;
    unsigned char bumpers;
}  __attribute__((packed)) t_can_robot_position;


Mrf24j * mrf;
TFT_eSPI tft;       // Invoke custom library

SPIClass radio_spi(HSPI);

void setup() {
  // put your setup code here, to run once
  Serial.begin(115200);

  radio_spi.setBitOrder(MSBFIRST);
  radio_spi.setDataMode(SPI_MODE0);
  radio_spi.begin(SCLK, MISO, MOSI, -1);

  //SPI.setBitOrder(MSBFIRST);
  //SPI.setDataMode(SPI_MODE0);
  //SPI.begin(SCLK, MISO, MOSI, -1);

  mrf = new Mrf24j(RST, CS, INT, WAKEUP, &radio_spi);
  
  
  tft.init();
  tft.setRotation(0);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Adding a background colour erases previous text automatically
  

  mrf->reset();
  mrf->init();
  mrf->set_pan(0xb205);
  mrf->address16_write(0x6001);
  mrf->set_channel(12);

  // uncomment if you want to receive any packet on this channel
  mrf->set_promiscuous(true);

  // uncomment if you want to enable PA/LNA external control
  //mrf.set_palna(true);

  // uncomment if you want to buffer all PHY Payload
  //mrf.set_bufferPHY(true);

  attachInterrupt(INT, interrupt_routine, CHANGE);
  interrupts();
/*
  */
  Serial.println("Starting");
  #if CONFIG_IDF_TARGET_ESP32
  Serial.println("CONFIG_IDF_TARGET_ESP32");
  #endif
  tft.drawCentreString("Starting",120,260,4);
}

void interrupt_routine() {
  mrf->interrupt_handler(); // mrf24 object interrupt routine
}

void loop() {
   mrf->check_flags(&handle_rx, &handle_tx);
}

void handle_rx() {
  /*
    Serial.print("received a packet : ");
    for (int i = 0; i < mrf.rx_datalength(); i++) {
        char s[10];
        sprintf(s," %02X",mrf.get_rxinfo()->rx_data[i]);
          Serial.print(s);
    }

    Serial.println("");
    */
    uint8_t * data = (uint8_t*)mrf->get_rxinfo()->rx_data;

    if (data[0] == 0xe3 && data[1] == 0x3) {
      t_can_robot_position * pos = (t_can_robot_position *)&data[2];
      Serial.print("Robot pos: ");
      Serial.print(pos->x);
      Serial.print(",");    
      Serial.print(pos->y);
      Serial.println("");
      char s[80];
      sprintf(s,"X: %5d, Y: %5d", pos->x, pos->y);
      tft.drawCentreString(s,120,60,4);
    }
    
    //tft.drawCentreString("Time flies",120,260,4);
}

void handle_tx() {
    // code to transmit, nothing to do
}
