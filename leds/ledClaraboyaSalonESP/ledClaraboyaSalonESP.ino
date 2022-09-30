//#define _DEBUG_
//#define _DEBUG_Q_ // Simulation SINQ Nextion with General Rele

// #include "arduino.h"
HardwareSerial Serial2(1); // Use UART channel 1 TX 10 RX 9
byte nextionPing;
byte state=1;

char cmd;
char page;
bool avanced;
byte pinVcc = 22;
byte nameObject = 0;
char object;
byte mode;
byte flanco;
byte from;
byte to;
byte valueNew;
byte numLed;


#include <SPI.h> //
#include <FastLED.h>
const byte ledSalonPin = 21;
const byte ledSalonLEDS = 130;
CRGB ledSalon[ledSalonLEDS];  // Con el constructor creamos el array RGB
byte ledSalonHSV[ledSalonLEDS][3];  // Para guardar todos los valores HSV
byte ledSalonXcada = 1;
byte brightness = 20;

bool ledSalonStatus = 0;
bool ledPasilloStatus = 0;

bool dinamicMode = 0;
bool dinamicModeType = 0;

const byte flancos[9][2]={
  {   0,  16 }, // Flanco 1 (Flanco 1 1ª mitad)
  { 123, 129 }, // Flanco 1 (Flanco 1 2ª mitad)
  {  17,  24 }, // Flanco 2
  {  25,  48 }, // Flanco 3
  {  49,  57 }, // Flanco 4
  {  58,  81 }, // Flanco 5
  {  82,  90 }, // Flanco 6
  {  91, 114 }, // Flanco 7
  { 115, 122 }  // Flanco 8
};

const unsigned int colorRGB565[3][16]=  // Colores de referencia para Nextion
{
 // RED       RtoO      ORANGE     OtoY      YELLOW     YtoG      GREEN      GtoA       AQUA       AtoB       BLUE       BtoPP      PURP       PPtoP       PINK       PtoR        RED
 // 0     7   15    23  31    39   47    55  63    71   79    87  95    103  111   119  127   135  143   151  159   167  175   183  191   199  207   215   223   231  239   247   255 -> HUE
   {63488,    64000,    64512,     64992,    65504,     32736,     2016,      2031,      2047,      1023,        31,     16408,     32784,     45651,      63493,     63488},  // SAT 255
   {64170,    64522,    64842,     65194,    65514,     45034,    22506,     22517,     22527,     21855,     21183,     35519,     64191,     52279,      64174,     64171},  // SAT 170
   {64853,    65013,    65205,     65365,    65525,     55285,    45045,     45050,     45055,     44735,     44383,     50527,     64863,     58907,      65049,     64853}   // SAT 85
};

HardwareSerial rs485Serial(1);

void setup() {
  Serial.begin(9600);
  rs485Serial.begin(57600, SERIAL_8N1, 16, 17);
//  rs485Serial.begin(57600);      //  Nextion falla mucho con los bauds más altos
  Serial.println("INIT");
  delay( 3000 );                     ////   ATENTO AL BLANCO con el 0xFF5090
  FastLED.addLeds<WS2812B, ledSalonPin, GRB>(ledSalon, ledSalonLEDS)
  .setCorrection(  UncorrectedColor ); 
//  .setCorrection(  0xFF5090 );            //  UncorrectedColor, TypicalSMD5050, Typical8mmPixel El resto es identico )
//  FastLED.setBrightness( brightness );          // Con CHSV( 24, 255, 255);  "amarillo"
  synqReleOnOff();                               //  Amarillo,     mas rojo  ,      mas amarillo  
                                                  // Con CHSV( 64, 210, 255);  "amarillo amarillo"
                                                  //  verdoso   , mas amarillo, mas verdoso
                                                  // Con CHSV( 64, 255, 255);  "verdoso" parece el spectrum
                                                  // 
}

void loop() {
#ifdef _DEBUG_
  if(state>0)
  {
    if(state==1)
    {
     //ShowLedWhite();
     ShowLedBlack();
     state=2;
    //  Serial.print("state = ");Serial.print(state);
    }else if(state=2)
    {
     delay(1000);
     ShowFastLed();
     state=0;
    }
  }
#endif

  
  rs485SerialRead();    // No confiar en la comunicación serial
                          // con FastLED los timers muy transtocados
                          // La comunicación es posible pero
                          // con tiempos de resapuesta imprebisibles
  if(dinamicMode)
  {
    switch(dinamicModeType)
    {
      case 1:
        ShowPacifico();
      break;
      default:
        dinamicMode = 0;
      break;
    }
  }

 // FastLED.show();
}

void ShowLedTest() {
  EVERY_N_MILLISECONDS( 20) {
    pacifica_loop();
    FastLED.show();
  }
}

void ShowLedBlack() {
  mode = 2;
  valueNew = 0;
  from = flancos[0][0];
  to = flancos[1][1];
  writeValueHSV();
  ShowFastLed();
}

void ShowPacifico() {
  EVERY_N_MILLISECONDS( 20) {
    pacifica_loop();
    FastLED.show();
  }
}

void ShowFastLed() {
  for (byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
  {
    ledSalon[i] = CHSV( ledSalonHSV[i][0], ledSalonHSV[i][1], ledSalonHSV[i][2]);
  }
  FastLED.show();
}

void rs485SerialRead() {
  /*
 * <init>
 * Heade - Indicate start of transmission
 * <check>
 * Indicate number of byte that remains to be sent
 * <DIR>
 * Indicate the send address G=General Relays S=Leds Salon N=Nextion
 * <CMD>
 * Indicate the specific command to execute
 * 
 * /////   COMANDS   /////////
 * 
 * Number relay for change state
 * <init> <check> <DIR> <CMD> <Pin relay>
 *    #      3      G    R
 *  0x23   0x03   0x47 0x52
 * printh 23 03 47 52
 * <Pin relay>
 * prints pageTeclaNum.vaRele.val,1
 * 
 * Control LEDs
 * <init> <check> <DIR> <CMD> <pinVcc> <type> <from> <to> <value>
 *   #       7      G     L           /////Cambiar G x S 0x53 claraboya Salón ////////
 *  0x23   0x07   0x47  0x4C          ////////////////////////////////////////////////
 * printh 23 07 47 4C
 * <pinVcc>         0=Salón 1=Pasillo
 * prints pageCmdLed.vaPinVccClara.val,1
 * <type>         0=OFF 1=ON 2=HUE 3=SAT 4=VAL or >10=Predefinied Modes
 * <from>         First led selection
 * prints pageClaraboya.nFrom.val,1
 * <to>           Last led selection
 * prints pageClaraboya.nTo.val,1     XX= value 0 to 129
 * <value>
 * prints pageClaraboya.nXXX.val,1   XXX= HUE, SAT or VAL. value 0 to 255
 * 
 * Request for refresh all dates
 * <init> <check> <DIR> <CMD> <CMD2> <pinVcc>
 *   #       2      G     Q
 *  0x23   0x02   0x47  0x51
 * printh 23 02 47 51
 *  <CMD2>          M=pagaMain L=LED
 * <pinVcc>         0=Salón 1=Pasillo
 * prints pageCmdLed.vaPinVccClara.val,1
 * 
 */
  if(rs485Serial.available()>2)
  {
    char initData = rs485Serial.read();
    if(initData=='#')
    {
//Serial.print("initData = "); Serial.println(initData);
      byte checkData = rs485Serial.read();
      unsigned long tmr1 = millis();
      bool minLength = true;
      while(rs485Serial.available()<checkData)
      {
        if((millis()-tmr1)>100)
        {
          minLength=false;
          break;
        }
      }
// Verify that the entire data string that has been received does not exceed the expected
      if(minLength==true && rs485Serial.available()==checkData)
      {
        char dir = rs485Serial.read();
        if(dir == 'S')                        // Direction for Leds Salón
        {
          cmd = rs485Serial.read();
          switch(cmd)
          {
            case 'L':
              pinVcc = rs485Serial.read();
              nameObject = rs485Serial.read();
              mode = rs485Serial.read();
              switch(mode)
              {
                case 5: // ON/OFF
                  ShowLedBlack();
                  dinamicMode = 0;
                  sendReleOnOff();
                break;
                case 4: // SHOW printleds
                  dinamicMode = 0;
                  sendNextion_hook();
                  sendNextion_synqOk();
                  if(avanced){ sendNextion_symbolicsColours(); }
                  ShowFastLed();
                break;
                case 8:  // Stop Motion pLedClaraBasic.
                  dinamicMode=0;
                  sendNextion_hook();
                  sendNextion_synqOk();
                break;
                case 3: // Intervalos
                  ledSalonXcada = rs485Serial.read();
                  sendNextion_hook();
                  sendNextion_interval();
                  sendNextion_synqOk();
                break;
                case 0: // HUE
                  selectLeds_values();
                  writeValueHSV();
                break;
                case 1: // SAT
                  selectLeds_values();
                  writeValueHSV();
                break;
                case 2: // VAL
                  selectLeds_values();
                  writeValueHSV();
                break;
  /// simpleMode ( 21 ~ 110 )
                case 21:  // Mode White
                  selectLeds_modes();
                  writeModeHSV( 64, 0, 75);
                break;
                case 22:  // Mode Red
                  selectLeds_modes();
                  writeModeHSV( 0, 255, 75);
                break;
                case 23:  // Mode Orange
                  selectLeds_modes();
                  writeModeHSV( 32, 255, 75);
                break;
                case 24:  // Mode Yellow
                  selectLeds_modes();
                  writeModeHSV( 64, 255, 75);
                break;
                case 25:  // Mode Green
                  selectLeds_modes();
                  writeModeHSV( 96, 255, 75);
                break;
                case 26:  // Mode Aqua
                  selectLeds_modes();
                  writeModeHSV( 128, 255, 75);
                break;
                case 27:  // Mode Blue
                  selectLeds_modes();
                  writeModeHSV( 160, 255, 75);
                break;
                case 28:  // Mode Purple
                  selectLeds_modes();
                  writeModeHSV( 192, 255, 75);
                break;
                case 29:  // Mode Pink
                  selectLeds_modes();
                  writeModeHSV( 224, 255, 75);
                break;
                case 30:  // Mode
                  selectLeds_modes();
                  //writeModeHSV( 224, 255, 75);
                break;
                case 31:  // Mode
                  selectLeds_modes();
                  //writeModeHSV( 224, 255, 75);
                break;
                case 32:  // Mode
                  selectLeds_modes();
                  //writeModeHSV( 224, 255, 75);
                break;
// dinamicMode ( 111 ~ 200 )
                case 111: // Pacifico
                  sendNextion_hook(), sendNextion_synqOk();
                  dinamicMode = 1, dinamicModeType = 1;
                break;
                case 112:
                  sendNextion_hook(), sendNextion_synqOk();
                  dinamicMode = 1, dinamicModeType = 2;
                break;
                case 113:
                  sendNextion_hook(), sendNextion_synqOk();
                  dinamicMode = 1, dinamicModeType = 3;
                break;
                case 114:
                  sendNextion_hook(), sendNextion_synqOk();
                  dinamicMode = 1, dinamicModeType = 4;
                break;
                case 115:
                  sendNextion_hook(), sendNextion_synqOk();
                  dinamicMode = 1, dinamicModeType = 5;
                break;
                case 116:
                  sendNextion_hook(), sendNextion_synqOk();
                  dinamicMode = 1, dinamicModeType = 6;
                break;
// Blancos      // Hoguera 12,187,255    7,201,255  24,210,255  
                case 201: // Candle // HSV 30 84 100
                  selectLeds_modes();
                  //{ ledSalonHSV[i][0] = 21, ledSalonHSV[i][1] = 130, ledSalonHSV[i][2] = 255; }
                  //{ ledSalon[i] = CRGB(255,147,41); }
                  //{ ledSalonHSV[i][0] = 24, ledSalonHSV[i][1] = 210, ledSalonHSV[i][2] = 255; }
                  writeModeHSV( 24, 210, 255);
                break;
                case 202: // Tungsten40W
                  selectLeds_modes();
                  //{ ledSalon[i] = CRGB(255,197,143); } // Tungsten40W
                  //writeModeHSV( 24, 210, 255);
                break;
                case 203: // Tungsten100W
                  selectLeds_modes();
                  //{ ledSalon[i] = CRGB(255,214,170); }
                  //writeModeHSV( 24, 210, 255);
                break;
                case 204: // Halogen
                  selectLeds_modes();
                  //{ ledSalon[i] = CRGB(255,241,224); }
                  //writeModeHSV( 24, 210, 255);
                break;
                case 205: // CarbonArc
                  selectLeds_modes();
                  //{ ledSalon[i] = CRGB(255,250,224); }
                  //writeModeHSV( 24, 210, 255);
                break;
                case 206: // HighNoonSun
                  selectLeds_modes();
                  //{ ledSalon[i] = CRGB(255,255,251); }
                  //writeModeHSV( 24, 210, 255);
                break;
                case 207: // DirectSunlight
                  selectLeds_modes();
                  //{ ledSalon[i] = CRGB(255,255,255); }
                  //writeModeHSV( 24, 210, 255);
                break;
                case 208: // OvercastSky
                  selectLeds_modes();
                //  { ledSalon[i] = CRGB(201,226,255); }
                //{ ledSalon[i] = CRGB(64,156,255); } // ClearBlueSky
                  //writeModeHSV( 24, 210, 255);
                break;
                case 209: // ClearBlueSky //149,191,255
                  selectLeds_modes();
                  //{ ledSalonHSV[i][0] = 144, ledSalonHSV[i][1] = 130, ledSalonHSV[i][2] = 255; }
                  //{ ledSalon[i] = CRGB(64,156,255); }
                  //writeModeHSV( 24, 210, 255);
                break;
// Blancos Personalizados
                case 221: // WarmFluorescent
                  selectLeds_modes();
                  //{ ledSalonHSV[i][0] = 64, ledSalonHSV[i][1] = 156, ledSalonHSV[i][2] = 255; }
                  //writeModeHSV( 64, 156, 255);
                break;
                case 222: // StandardFluorescent
                  selectLeds_modes();
                  //writeModeHSV( 64, 156, 255);
                break;
                case 223: // CoolWhiteFluorescent
                  selectLeds_modes();
                  //writeModeHSV( 64, 156, 255);
                break;
                case 224: // FullSpectrumFluorescent
                  selectLeds_modes();
                  //writeModeHSV( 64, 156, 255);
                break;
                case 225: // GrowLightFluorescent
                  selectLeds_modes();
                  //writeModeHSV( 64, 156, 255);
                break;
                case 226: // BlackLightFluorescent
                  selectLeds_modes();
                  //writeModeHSV( 64, 156, 255);
                break;
                case 227: // MercuryVapor
                  selectLeds_modes();
                  //writeModeHSV( 64, 156, 255);
                break;
                case 228: // SodiumVapor
                  selectLeds_modes();
                  //writeModeHSV( 64, 156, 255);
                break;
                case 229: // MetalHalide
                  selectLeds_modes();
                  //writeModeHSV( 64, 156, 255);
                break;
                case 230: // HighPressureSodium
                  selectLeds_modes();
                  //writeModeHSV( 64, 156, 255);
                break;
              }
            break;
  // synq
            case 'Q':
              page = rs485Serial.read();
              switch(page)
              {
                case 'G': // SYNQ for General Relays ON/OFF
                  pinVcc = rs485Serial.read();
                  nameObject = rs485Serial.read();  // Name
                  ledSalonStatus = rs485Serial.read();
                  if(pinVcc==22 && nameObject==0) // synq Led Salon
                  {
                    sendNextion_hook();
                    sendNextion_synqOk();
                    sendNextion_OnOff();
                  }
                break;
                case 'Q': // synq Led page AVANCE
                  pinVcc = rs485Serial.read();
                  nameObject = rs485Serial.read();  // Name
                  avanced = rs485Serial.read(); // 1 = Print leds colours
                  if(pinVcc==22 && nameObject==0) // synq Led Salon
                  {
                    sendNextion_hook();
                    sendNextion_synq();
                  }
                break;
                case 'P':  // Print leds colours
                  pinVcc = rs485Serial.read();
                  nameObject = rs485Serial.read();  // Name
                  if(pinVcc==22 && nameObject==0) // synq Led Salon
                  {
                    sendNextion_hook();
                    sendNextion_synqOk();
                    sendNextion_symbolicsColours();
                  }
                break;
                case 'R':  // Send led colour
                  pinVcc = rs485Serial.read();
                  nameObject = rs485Serial.read();  // Name
                  numLed = rs485Serial.read();  // Number Led
                  if(pinVcc==22 && nameObject==0) // synq Led Salon
                  {
                    sendNextion_hook();
                    sendNextion_colourLed(0, numLed);
                  }
                break;
              }
            break;
          }
        }
      }
    }
  }
}

void selectLeds_values() {
  flanco = rs485Serial.read();
  if( flanco==10 )
  {
    from = rs485Serial.read();
    to = rs485Serial.read();
    valueNew = rs485Serial.read();
  }else if( flanco==9 ){
    valueNew = rs485Serial.read();
    from = flancos[0][0];
    to = flancos[1][1];
  }else{
    valueNew = rs485Serial.read();
    from = flancos[flanco][0];
    to = flancos[flanco][1];
  }
}

void selectLeds_modes() {
  flanco = rs485Serial.read();
  if( flanco==10 )
  {
    from = rs485Serial.read();
    to = rs485Serial.read();
  }else if( flanco==9 ){
    valueNew = rs485Serial.read();
    from = flancos[0][0];
    to = flancos[1][1];
  }else{
    from = flancos[flanco][0];
    to = flancos[flanco][1];
  }
}

void sendReleOnOff() {
  //delay(1);
  byte bufSend[] = { '#', 4, 'G', 'R', pinVcc, nameObject };
  rs485Serial.write(bufSend, 6);
}

void synqReleOnOff() {
  ShowLedBlack();
  //delay(1);
  byte bufSend[] = { '#', 4, 'G', 'Q', 'L', pinVcc, nameObject };
  rs485Serial.write(bufSend, 7);
}

void writeValueHSV() {
  dinamicMode = 0;
  if(from <= to)
  {
    for(byte i=from; i<=to; i+=ledSalonXcada )
    { ledSalonHSV[i][mode] = valueNew; }
  }else{
    for(byte i=flancos[0][0]; i<=to; i+=ledSalonXcada )
    { ledSalonHSV[i][mode] = valueNew; }
    for(byte i=from; i<=flancos[1][1]; i+=ledSalonXcada )
    { ledSalonHSV[i][mode] = valueNew; }
  }
}

void writeModeHSV(byte H, byte V, byte S) {
  dinamicMode = 0;
  if(from <= to)
  {
    for(byte i=from; i<=to; i+=ledSalonXcada )
    { ledSalonHSV[i][0] = H, ledSalonHSV[i][1] = V, ledSalonHSV[i][2] = S; }
  }else{
    for(byte i=flancos[0][0]; i<=to; i+=ledSalonXcada )
    { ledSalonHSV[i][0] = H, ledSalonHSV[i][1] = V, ledSalonHSV[i][2] = S; }
    for(byte i=from; i<=flancos[1][1]; i+=ledSalonXcada )
    { ledSalonHSV[i][0] = H, ledSalonHSV[i][1] = V, ledSalonHSV[i][2] = S; }
  }
}

void sendNextion_hook() {
  //delay(1),
  FF(), rs485Serial.print("tsw hpSendValue,0"); FF(); // Send signal lure "señuelo" Conseguimos que falle menos
}

void sendNextion_synq() {
  sendNextion_synqOk();
  sendNextion_OnOff();
  sendNextion_interval();
}

void sendNextion_synqOk() {
  rs485Serial.print("click hpU,1"); FF();
}

void sendNextion_OnOff() {
  if(ledSalonStatus){                     // ON
    if(avanced){ sendNextion_symbolicsColours(); }
    rs485Serial.print("pOn.pic=27"); FF();
  }
  else {                                  // OFF
    rs485Serial.print("pOn.pic=26"); FF();
    rs485Serial.print("nVal.val=0"); FF();
    rs485Serial.print("hVal.val=0"); FF();
    rs485Serial.print("pVaLedSalon.nVal.val=0"); FF();
    //rs485Serial.print("pVaLedPasillo.nVal.val=0"); FF();
    if(avanced)
    {
      for(byte i = 0; i < ledSalonLEDS; i++)
      {
        rs485Serial.print("l" + String(i) + ".bco=0"); FF();  // OFF = Black
      }
    }
  }
}

void sendNextion_interval() {
  rs485Serial.print("nI.val=" + String(ledSalonXcada)); FF();
}

void sendNextion_colourLed(byte a, byte _numLed) {
  switch(a)
  {
    case 0:
      byte HUE, SAT, VAL;
      HUE = ledSalonHSV[_numLed][0], SAT = ledSalonHSV[_numLed][1], VAL = ledSalonHSV[_numLed][2];
      rs485Serial.print("nHue.val=" + String(HUE)); FF();
      rs485Serial.print("hHue.val=" + String(HUE)); FF();
      rs485Serial.print("nSat.val=" + String(SAT)); FF();
      rs485Serial.print("hSat.val=" + String(SAT)); FF();
      rs485Serial.print("nVal.val=" + String(VAL)); FF();
      rs485Serial.print("hVal.val=" + String(VAL)); FF();
      rs485Serial.print("tBCK.txt=\"Led " + String(_numLed) + " seleccionado\""); FF();
    break;
  }
  
}

void sendNextion_symbolicsColours() {
  byte HUE, SAT;
  byte l, k;
  for(byte i = 0; i < ledSalonLEDS; i++)
    {
    HUE = ledSalonHSV[i][0], SAT = ledSalonHSV[i][1];
    int m, n;
    for (int j = 0; j < 16; j++)
    {
      if(j==0 || j==16-1)
      {
        m=j*8;
        n=(j+1)*8;
        if(HUE>=m && HUE<n)
        {
          if(SAT>=0 && SAT<85){
            rs485Serial.print("l" + String(i) + ".bco=" + String(colorRGB565[2][0])); FF();
          }
          if(SAT>=85 && SAT<170){
            rs485Serial.print("l" + String(i) + ".bco=" + String(colorRGB565[1][0])); FF();
          }
          if(SAT>=170 && SAT<256){
            rs485Serial.print("l" + String(i) + ".bco=" + String(colorRGB565[0][0])); FF(); 
          }
        }
      }else{
        m=j*16;
        n=(j+1)*16;
        if(HUE>=m && HUE<n)
        {
          if(SAT>=0 && SAT<85){
            rs485Serial.print("l" + String(i) + ".bco=" + String(colorRGB565[2][j])); FF(); 
          }
          if(SAT>=85 && SAT<170){
            rs485Serial.print("l" + String(i) + ".bco=" + String(colorRGB565[1][j])); FF(); 
          }
          if(SAT>=170 && SAT<256){
            rs485Serial.print("l" + String(i) + ".bco=" + String(colorRGB565[0][j])); FF(); 
          }
        }
      }
    }
  }
}

void FF(){
  rs485Serial.print("\xFF\xFF\xFF");
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
