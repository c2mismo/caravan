//#define _DEBUG_

// #include "arduino.h"
HardwareSerial rs485Serial(1); // Use UART channel 1 TX 10 RX 9

#include <FastLED.h>
const byte ledSalonPin = 21;
const byte ledSalonLEDS = 398;
CRGB ledSalon[ledSalonLEDS];  // Con el constructor creamos el array RGB
byte ledSalonHSV[ledSalonLEDS][3];  // Para guardar todos los valores HSV
byte ledSalonXcada = 1;
byte from = 1;
byte to = ledSalonLEDS;
byte brightness = 200;

bool dinamicMode = 0;

const int flancos[5][2]={
  {   0,  39 }, // Flanco 1
  {  40,  126 }, // Flanco 2
  {  127,  257 }, // Flanco 3
  {  258,  345 }, // Flanco 4
  {  346,  398 }, // Flanco 5
};

void setup() {
  Serial.begin(9600);
  rs485Serial.begin(57600, SERIAL_8N1, 9, 10);
//  rs485Serial.begin(57600);      //  Nextion falla mucho con los bauds más altos
  Serial.println("INIT");
  delay( 3000 );                     ////   ATENTO AL BLANCO con el 0xFF5090  // for IP65 GRB  and for IP67 RGB
  FastLED.addLeds<WS2812B, ledSalonPin, GRB>(ledSalon, ledSalonLEDS);
  //.setCorrection(  UncorrectedColor );
//  .setCorrection(  0xFF5090 );            //  UncorrectedColor, TypicalSMD5050, Typical8mmPixel El resto es identico )
  FastLED.setBrightness( brightness );          // Con CHSV( 24, 255, 255);  "amarillo"
//  synqReleOnOff();                               //  Amarillo,     mas rojo  ,      mas amarillo  
                                                  // Con CHSV( 64, 210, 255);  "amarillo amarillo"
                                                  //  verdoso   , mas amarillo, mas verdoso
                                                  // Con CHSV( 64, 255, 255);  "verdoso" parece el spectrum
                                                  //
                                                  
}

void loop() {
//  ShowPacifico();

  writeModeHSV( 64, 156, 254);

  ShowFastLed();

}


                         // No me entra _S por la cara
void writeModeHSV(byte _H, byte _V, byte S) {
  dinamicMode = 0;
  if(from <= to)
  {
    for(byte i=from; i<=to; i+=ledSalonXcada )
    { ledSalonHSV[i][0] = _H, ledSalonHSV[i][1] = _V, ledSalonHSV[i][2] = S; }
  }else{
    for(byte i=flancos[0][0]; i<=to; i+=ledSalonXcada )
    { ledSalonHSV[i][0] = _H, ledSalonHSV[i][1] = _V, ledSalonHSV[i][2] = S; }
    for(byte i=from; i<=flancos[1][1]; i+=ledSalonXcada )
    { ledSalonHSV[i][0] = _H, ledSalonHSV[i][1] = _V, ledSalonHSV[i][2] = S; }
  }
}

void ShowFastLed() {
  for (byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
  {
    ledSalon[i] = CHSV( ledSalonHSV[i][0], ledSalonHSV[i][1], ledSalonHSV[i][2]);
  }
  FastLED.show();
}


void ShowPacifico() {
  EVERY_N_MILLISECONDS( 20) {
    pacifica_loop();
    FastLED.show();
  }
}

//
//  "Pacifica"
//  Gentle, blue-green ocean waves.
//  December 2019, Mark Kriegsman and Mary Corey March.
//  For Dan.
//
//////////////////////////////////////////////////////////////////////////
//
// The code for this animation is more complicated than other examples, and 
// while it is "ready to run", and documented in general, it is probably not 
// the best starting point for learning.  Nevertheless, it does illustrate some
// useful techniques.
//
//////////////////////////////////////////////////////////////////////////
//
// In this animation, there are four "layers" of waves of light.  
//
// Each layer moves independently, and each is scaled separately.
//
// All four wave layers are added together on top of each other, and then 
// another filter is applied that adds "whitecaps" of brightness where the 
// waves line up with each other more.  Finally, another pass is taken
// over the led array to 'deepen' (dim) the blues and greens.
//
// The speed and scale and motion each layer varies slowly within independent 
// hand-chosen ranges, which is why the code has a lot of low-speed 'beatsin8' functions
// with a lot of oddly specific numeric ranges.
//
// These three custom blue-green color palettes were inspired by the colors found in
// the waters off the southern coast of California, https://goo.gl/maps/QQgd97jjHesHZVxQ7
//
CRGBPalette16 pacifica_palette_1 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 };
CRGBPalette16 pacifica_palette_2 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F };
CRGBPalette16 pacifica_palette_3 = 
    { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33, 
      0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF };


void pacifica_loop()
{
  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  sLastms = ms;
  uint16_t speedfactor1 = beatsin16(3, 179, 269);
  uint16_t speedfactor2 = beatsin16(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88(1011,10,13));
  sCIStart2 -= (deltams21 * beatsin88(777,8,11));
  sCIStart3 -= (deltams1 * beatsin88(501,5,7));
  sCIStart4 -= (deltams2 * beatsin88(257,4,6));

  // Clear out the LED array to a dim background blue-green
  fill_solid( ledSalon, ledSalonLEDS, CRGB( 2, 6, 10));

  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer( pacifica_palette_1, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0-beat16( 301) );
  pacifica_one_layer( pacifica_palette_2, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  pacifica_one_layer( pacifica_palette_3, sCIStart3, 6 * 256, beatsin8( 9, 10,38), 0-beat16(503));
  pacifica_one_layer( pacifica_palette_3, sCIStart4, 5 * 256, beatsin8( 8, 10,28), beat16(601));

  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps();

  // Deepen the blues and greens a bit
  pacifica_deepen_colors();
}

// Add one layer of waves into the led array
void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for( uint16_t i = 0; i < ledSalonLEDS; i++) {
    waveangle += 250;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16( ci) + 32768;
    uint8_t sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    ledSalon[i] += c;
  }
}

// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps()
{
  uint8_t basethreshold = beatsin8( 9, 55, 65);
  uint8_t wave = beat8( 7 );

  for( uint16_t i = 0; i < ledSalonLEDS; i++) {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = ledSalon[i].getAverageLight();
    if( l > threshold) {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8( overage, overage);
      ledSalon[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}

// Deepen the blues and greens
void pacifica_deepen_colors()
{
  for( uint16_t i = 0; i < ledSalonLEDS; i++) {
    ledSalon[i].blue = scale8( ledSalon[i].blue,  145); 
    ledSalon[i].green= scale8( ledSalon[i].green, 200); 
    ledSalon[i] |= CRGB( 2, 5, 7);
  }
}
