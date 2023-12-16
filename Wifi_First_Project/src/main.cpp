/***
 * Connection Wifi by ESP32
 * Attention: set usernamen and password by code
 */
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RESET -1 // Reset del microcontrollore

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool withDisplay = false;

void initDisplay()
{
  Serial.println("Init display...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed");
  }
  else
  {
    withDisplay = true;
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Display OK");
    display.display();
  }
}

void setup()
{
  Serial.begin(115200);
  initDisplay();

  // WiFi.mode(WIFI_AP); // access point con il nostro dispositivo
  // WiFi.softAP("votamiri", "Mirix463ueuhdue");

  // station
  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks(); // restituisce il numero di reti trovabili
  for (int i = 0; i < n; i++)
  {
    Serial.println(WiFi.SSID(i) + " " + WiFi.RSSI(i)); // lista delle reti
  }

  WiFi.begin("username", "password"); // per collegarmi ad un access point
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }

  Serial.println(WiFi.localIP()); // stampa il mio indirizzo IP
}

void loop()
{
  // put your main code here, to run repeatedly:
}