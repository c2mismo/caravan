#include "Arduino.h"
#include "Leds.h"

Leds::Leds(const int LeftIn, const int LeftOut, const int RightIn, const int RightOut, const int Inverter, const int CHR, const int Home, const int Automatic)

{
  pinMode(LeftIn, OUTPUT);
  pinMode(LeftOut, OUTPUT);
  pinMode(RightIn, OUTPUT);
  pinMode(RightOut, OUTPUT);
  pinMode(Inverter, OUTPUT);
  pinMode(CHR, OUTPUT);
  pinMode(Home, OUTPUT);
  pinMode(Automatic, OUTPUT);
  _Leds[0] = LeftIn;
  _Leds[1] = LeftOut;
  _Leds[2] = RightIn;
  _Leds[3] = RightOut;
  _Leds[4] = Inverter;
  _Leds[5] = CHR;
  _Leds[6] = Home;
  _Leds[7] = Automatic;
  off();
}

void Leds::on()
{
	for (_i = 0; _i < 9; _i++)
  {
    digitalWrite(_Leds[_i], HIGH);
  }
}

void Leds::off()
{
	for (_i = 0; _i < 9; _i++)
  {
    digitalWrite(_Leds[_i], LOW);
  }
}

void Leds::onDelay()
{
	for (_i = 0; _i < 9; _i++)
  {
    digitalWrite(_Leds[_i], HIGH);
    delay(200);
  }
}

void Leds::offDelay()
{
	for (_i = 7; _i >= 0; _i--)
  {
    digitalWrite(_Leds[_i], LOW);
    delay(200);
  }
}

void Leds::blinkOn()
{
  onDelay();
  delay(1500);
  off();
  delay(1000);
  on();
  delay(1500);
  off();
  delay(1000);
  on();
  delay(3000);
  off();
}

void Leds::blinkOff()
{
  on();
  delay(1500);
  off();
  delay(1000);
  on();
  delay(1500);
  off();
  delay(1000);
  on();
  delay(3000);
  offDelay();
}