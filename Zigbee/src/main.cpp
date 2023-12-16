#include <Arduino.h>
#include <mrf24j.h>

#define MISO 2
#define MOSI 15
#define SCLK 14
#define CS 13
#define INT 4
#define WAKEUP 12
#define RST 0

Mrf24j *mrf;
SPIClass radio_spi(HSPI);

void interrupt_routine()
{
  mrf->interrupt_handler(); // mrf24 object interrupt routine
}

void handle_rx()
{
  /*
    Serial.print("received a packet : ");
    for (int i = 0; i < mrf.rx_datalength(); i++) {
        char s[10];
        sprintf(s," %02X",mrf.get_rxinfo()->rx_data[i]);
          Serial.print(s);
    }

    Serial.println("");
    */
  uint8_t *data = (uint8_t *)mrf->get_rxinfo()->rx_data;
  Serial.println("Ho ricevuto qualcosa");
  String s = String((char *)data);
  Serial.println(s);

  // tft.drawCentreString("Time flies",120,260,4);
}

void setup()
{
  Serial.begin(115200);
  radio_spi.setBitOrder(MSBFIRST);
  radio_spi.setDataMode(SPI_MODE0);
  radio_spi.begin(SCLK, MISO, MOSI, -1);

  mrf = new Mrf24j(RST, CS, INT, WAKEUP, &radio_spi);

  mrf->reset();
  mrf->init();
  mrf->set_pan(0xb205);
  mrf->address16_write(0x5001);
  mrf->set_channel(12);
  // mrf->set_promiscuous(true);
  mrf->set_bufferPHY(true);
  attachInterrupt(INT, interrupt_routine, CHANGE);
  interrupts();
}

void handle_tx()
{
  // code to transmit, nothing to do
}

void loop()
{

  mrf->check_flags(&handle_rx, &handle_tx);
}
