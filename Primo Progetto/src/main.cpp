#include <Arduino.h>

void setup()
{
  // put your setup code here, to run once:

  // Inizialize serial
  Serial.begin(9600);

  // il pin lo controllo dalla configurazione del pinout
  pinMode(3, OUTPUT);

  pinMode(4, OUTPUT);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(15, OUTPUT);

  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(0, HIGH);
  digitalWrite(2, HIGH);
  digitalWrite(15, HIGH);

  Serial.println("Avvio del dispositivo riuscito..");
}

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite(15, !digitalRead(15));
  delay(1000);
}
