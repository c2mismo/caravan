//#define _DEBUG_
//#define _DEBUG_Q_ // Simulation SINQ Nextion with General Rele

// #include "arduino.h"
#include <SoftwareSerial.h>
//SoftwareSerial rs485Serial(16, 14); // RX, TX
SoftwareSerial rs485Serial(16, 14); // RX, TX
byte nextionPing;
byte state=1;


#include <FastLED.h>
const byte ledSalonPin = 5;
const byte ledSalonLEDS = 130;
CRGB ledSalon[ledSalonLEDS];  // Con el constructor creamos el array RGB
byte ledSalonHSV[ledSalonLEDS][3];  // Para guardar todos los valores HSV
byte ledSalonXcada = 1;
byte brightness = 20;

bool dinamicMode = 0;
bool dinamicModeType = 0;

unsigned int colorRGB565[3][32]=
{
 // RED       RtoO      ORANGE     OtoY      YELLOW     YtoG      GREEN      GtoA       AQUA       AtoB       BLUE       BtoPP      PURP       PPtoP       PINK       PtoR        RED
 // 0     7   15    23  31    39   47    55  63    71   79    87  95    103  111   119  127   135  143   151  159   167  175   183  191   199  207   215   223   231  239   247   255 -> HUE
   {63488,    64000,    64512,     64992,    65504,     32736,     2016,      2031,      2047,      1023,        31,     16408,     32784,     45651,      63493,     63488},  // SAT 255
   {64170,    64522,    64842,     65194,    65514,     45034,    22506,     22517,     22527,     21855,     21183,     35519,     64191,     52279,      64174,     64171},  // SAT 170
   {64853,    65013,    65205,     65365,    65525,     55285,    45045,     45050,     45055,     44735,     44383,     50527,     64863,     58907,      65049,     64853}   // SAT 85
};

void setup() {
  Serial.begin(9600);
  rs485Serial.begin(9600);      //  Nextion falla mucho con los bauds más altos
  Serial.println("INIT");
//  delay(500);
  rs485Serial.print("pLedClarAvance.nInterval.val=" + String(ledSalonXcada)); FF();
  
  delay( 3000 );                     ////   ATENTO AL BLANCO con el 0xFF5090
  FastLED.addLeds<WS2812B, ledSalonPin, GRB>(ledSalon, ledSalonLEDS)
  .setCorrection(  UncorrectedColor ); 
//  .setCorrection(  0xFF5090 );            //  UncorrectedColor, TypicalSMD5050, Typical8mmPixel El resto es identico )
//  FastLED.setBrightness( brightness );          // Con CHSV( 24, 255, 255);  "amarillo"
  ShowLedBlack();                               //  Amarillo,     mas rojo  ,      mas amarillo  
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
  for (byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
  {
    ledSalon[i] = CRGB::Black;
  }
  //ShowFastLed();
  FastLED.show();
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
      int checkData = rs485Serial.read();
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
//Serial.print("dir = "); Serial.println(dir);
#ifdef _DEBUG_Q_
        if(dir == 'G')
        {
          char cmd = rs485Serial.read();
          char page;
          switch(cmd)
          {
            case 'Q':
              page = rs485Serial.read();  // page: N = pageMain F = Fans L = Leds
              switch(page)
              {
                case 'M':
  // synq PageMain
                  NextionSYNQ(255);
                break;
              }
            break;
          }
        }
#endif
        if(dir == 'S')                        // Direction for Leds Salón
        {
          char cmd = rs485Serial.read();
          char page;
          byte pinVcc;
          byte nameObject;
          char object;
          byte mode;
          byte from;
          byte to;
          byte valueNew;
          byte numLed;
          switch(cmd)
          {
            case 'L':
              pinVcc = rs485Serial.read();
              nameObject = rs485Serial.read();
              mode = rs485Serial.read();
Serial.print("mode = "); Serial.println(mode);
              switch(mode)
              {
                case 6: // OFF
                  dinamicMode = 0;
                break;
                case 5: // ON
                  dinamicMode = 0;
                break;
                case 4: // SHOW
                  NextionSYNQ(1);
                  ShowFastLed();
                break;
                case 8:  // Stop Motion pLedClaraBasic.
                  dinamicMode=0;
                  NextionSYNQ(1);
                break;
                case 3: // Intervalos
                  ledSalonXcada = rs485Serial.read();
Serial.print("ledSalonXcada = "); Serial.println(ledSalonXcada);
                  NextionSYNQ(2);
                break;
                case 0: // HUE
                  dinamicMode = 0;
                  from = rs485Serial.read();
                  to = rs485Serial.read();
                  valueNew = rs485Serial.read();  // ledSalonHSV
Serial.print("HUE = "); Serial.println(valueNew);
                  if(from < to)
                  {
                    for(from; from <= to; from+=ledSalonXcada) // Num from array limited into nextion (for <=)
                    { ledSalonHSV[from][mode] = valueNew; }
                  } //  if(from > to) { Edited into the nextion }
                  NextionSYNQ(0);
                break;
                case 1: // SAT
                  dinamicMode = 0;
                  from = rs485Serial.read();
                  to = rs485Serial.read();
                  valueNew = rs485Serial.read();  // ledSalonHSV
Serial.print("SAT = "); Serial.println(valueNew);
                  if(from < to)
                  {
                    for(from; from <= to; from+=ledSalonXcada) // Num from array limited into nextion (for <=)
                    { ledSalonHSV[from][mode] = valueNew; }
                  } //  if(from > to) { Edited into the nextion }
                  NextionSYNQ(0);
                break;
                case 2: // VAL
                  dinamicMode = 0;
                  from = rs485Serial.read();
                  to = rs485Serial.read();
                  valueNew = rs485Serial.read();  // ledSalonHSV
Serial.print("VAL = "); Serial.println(valueNew);
                  if(from < to)
                  {
                    for(from; from <= to; from+=ledSalonXcada) // Num from array limited into nextion (for <=)
                    { ledSalonHSV[from][mode] = valueNew; }
                  } //  if(from > to) { Edited into the nextion }
                  NextionSYNQ(0);
                break;
  /*/ simpleMode ( 21 ~ 110 )
                case 21:  // Mode White
                  dinamicMode = 0;
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalonHSV[i][0] = 64, ledSalonHSV[i][1] = 0, ledSalonHSV[i][2] = 75; }
                  NextionSYNQ(1);
                  ShowFastLed();
                break;
                case 22:  // Mode Red
                  dinamicMode = 0;
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalonHSV[i][0] = 0, ledSalonHSV[i][1] = 255, ledSalonHSV[i][2] = 75; }
                  NextionSYNQ(1);
                  ShowFastLed();
                break;
                case 23:  // Mode Orange
                  dinamicMode = 0;
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalonHSV[i][0] = 32, ledSalonHSV[i][1] = 255, ledSalonHSV[i][2] = 75; }
                  NextionSYNQ(1);
                  ShowFastLed();
                break;
                case 24:  // Mode Yellow
                  dinamicMode = 0;
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalonHSV[i][0] = 64, ledSalonHSV[i][1] = 255, ledSalonHSV[i][2] = 75; }
                  NextionSYNQ(1);
                  ShowFastLed();
                break;
                case 25:  // Mode Green
                  dinamicMode = 0;
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalonHSV[i][0] = 96, ledSalonHSV[i][1] = 255, ledSalonHSV[i][2] = 75; }
                  NextionSYNQ(1);
                  ShowFastLed();
                break;
                case 26:  // Mode Aqua
                  dinamicMode = 0;
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalonHSV[i][0] = 128, ledSalonHSV[i][1] = 255, ledSalonHSV[i][2] = 75; }
                  NextionSYNQ(1);
                  ShowFastLed();
                break;
                case 27:  // Mode Blue
                  dinamicMode = 0;
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalonHSV[i][0] = 160, ledSalonHSV[i][1] = 255, ledSalonHSV[i][2] = 75; }
                  NextionSYNQ(1);
                  ShowFastLed();
                break;
                case 28:  // Mode Purple
                  dinamicMode = 0;
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalonHSV[i][0] = 192, ledSalonHSV[i][1] = 255, ledSalonHSV[i][2] = 75; }
                  NextionSYNQ(1);
                  ShowFastLed();
                break;
                case 29:  // Mode Pink
                  dinamicMode = 0;
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalonHSV[i][0] = 224, ledSalonHSV[i][1] = 255, ledSalonHSV[i][2] = 75; }
                  NextionSYNQ(1);
                  ShowFastLed();
                break;*/
                case 30:  // Mode
                  dinamicMode = 0;
                  //FastLED.setCorrection(  0xFFFFFF );  // Solo Rojo
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  //{ ledSalonHSV[i][0] = 71, ledSalonHSV[i][1] = 0, ledSalonHSV[i][2] = 255; }
                  { ledSalon[i] = CRGB::White; }
                  FastLED.show();
                  NextionSYNQ(1);
                  //ShowFastLed();
                break;
                case 31:  // Mode
                  dinamicMode = 0;
                  //FastLED.setCorrection(  0x00FF00 ); // Verde
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalon[i] = CRGB::White; }
                  FastLED.show();
                  NextionSYNQ(1);
                  //ShowFastLed();
                break;
                case 32:  // Mode
                  dinamicMode = 0;
                  //FastLED.setCorrection(  0x0000FF ); // Azul
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  //{ ledSalonHSV[i][0] = 71, ledSalonHSV[i][1] = 0, ledSalonHSV[i][2] = 255; }
                  { ledSalon[i] = CRGB::White; }
                  FastLED.show();
                  NextionSYNQ(1);
                  //ShowFastLed();
                break;
// dinamicMode ( 111 ~ 200 )
                case 111: // Pacifico
                  NextionSYNQ(1);
                  dinamicMode = 1, dinamicModeType = 1;
                break;
                case 112:
                  NextionSYNQ(1);
                  dinamicMode = 1, dinamicModeType = 2;
                  ShowLedTest();
                break;
                case 113:
                  NextionSYNQ(1);
                  dinamicMode = 1, dinamicModeType = 3;
                  ShowLedTest();
                break;
                case 114:
                  NextionSYNQ(1);
                  dinamicMode = 1, dinamicModeType = 4;
                  ShowLedTest();
                break;
                case 115:
                  NextionSYNQ(1);
                  dinamicMode = 1, dinamicModeType = 5;
                  ShowLedTest();
                break;
                case 116:
                  NextionSYNQ(1);
                  dinamicMode = 1, dinamicModeType = 6;
                  ShowLedTest();
                break;
/*/ setCorrections
                case 201:
                  FastLED.setCorrection( UncorrectedColor );
                break;
                case 202:
                  FastLED.setCorrection( TypicalSMD5050 );
                break;
                case 203:
                  FastLED.setCorrection( TypicalLEDStrip );
                break;
                case 204:
                  FastLED.setCorrection( Typical8mmPixel );
                break;
                case 205:
                  FastLED.setCorrection( TypicalPixelString );
                break;*/
// Blancos      // Hoguera 12,187,255    7,201,255  24,210,255  
                case 201: // Candle // HSV 30 84 100
                  dinamicMode = 0;
                  FastLED.setBrightness( brightness );
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  //{ ledSalonHSV[i][0] = 21, ledSalonHSV[i][1] = 130, ledSalonHSV[i][2] = 255; }
                  //{ ledSalon[i] = CRGB(255,147,41); }
                  { ledSalonHSV[i][0] = 24, ledSalonHSV[i][1] = 210, ledSalonHSV[i][2] = 255; }
                  ShowFastLed();
                  NextionSYNQ(1);
                  //FastLED.show(), NextionSYNQ(0);
                break;
                case 202: // Tungsten40W
                  dinamicMode = 0;
                  FastLED.setBrightness( brightness );
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalon[i] = CRGB(255,147,41); } // Candle
                  //{ ledSalon[i] = CRGB(255,197,143); } // Tungsten40W
                  NextionSYNQ(1);
                  FastLED.show();
                break;
                case 203: // Tungsten100W
                  dinamicMode = 0;
                  FastLED.setBrightness( brightness );
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalon[i] = CRGB(255,214,170); }
                  NextionSYNQ(1);
                  FastLED.show();
                break;
                case 204: // Halogen
                  dinamicMode = 0;
                  FastLED.setBrightness( brightness );
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalon[i] = CRGB(255,241,224); }
                  NextionSYNQ(1);
                  FastLED.show();
                break;
                case 205: // CarbonArc
                  dinamicMode = 0;
                  FastLED.setBrightness( brightness );
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalon[i] = CRGB(255,250,224); }
                  NextionSYNQ(1);
                  FastLED.show();
                break;
                case 206: // HighNoonSun
                  dinamicMode = 0;
                  FastLED.setBrightness( brightness );
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalon[i] = CRGB(255,255,251); }
                  NextionSYNQ(1);
                  FastLED.show();
                break;
                case 207: // DirectSunlight
                  dinamicMode = 0;
                  FastLED.setBrightness( brightness );
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalon[i] = CRGB(255,255,255); }
                  NextionSYNQ(1);
                  FastLED.show();
                break;
                case 208: // OvercastSky
                  dinamicMode = 0;
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                //  { ledSalon[i] = CRGB(201,226,255); }
                { ledSalon[i] = CRGB(64,156,255); } // ClearBlueSky
                  NextionSYNQ(1);
                  FastLED.show();
                break;
                case 209: // ClearBlueSky //149,191,255
                  dinamicMode = 0;
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada)
                  { ledSalonHSV[i][0] = 144, ledSalonHSV[i][1] = 130, ledSalonHSV[i][2] = 255; }
                  //{ ledSalon[i] = CRGB(64,156,255); }
                  NextionSYNQ(1);
                  ShowFastLed();
                  //FastLED.show(), NextionSYNQ(0);
                break;
// Blancos Personalizados
                case 221:
                  dinamicMode = 0;
                  for(byte i = 0; i < ledSalonLEDS; i+=ledSalonXcada) // WarmFluorescent
                  { ledSalonHSV[i][0] = 64, ledSalonHSV[i][1] = 156, ledSalonHSV[i][2] = 255; }
                  NextionSYNQ(1);
                  ShowFastLed();
                break;
                case 222:
                  NextionSYNQ(1);
                  dinamicMode = 0;
                  FastLED.setTemperature( StandardFluorescent );
                break;
                case 223:
                  dinamicMode = 0;
                  NextionSYNQ(1);
                  FastLED.setTemperature( CoolWhiteFluorescent );
                break;
                case 224:
                  dinamicMode = 0;
                  NextionSYNQ(1);
                  FastLED.setTemperature( FullSpectrumFluorescent );
                break;
                case 225:
                  dinamicMode = 0;
                  NextionSYNQ(1);
                  FastLED.setTemperature( GrowLightFluorescent );
                break;
                case 226:
                  dinamicMode = 0;
                  NextionSYNQ(1);
                  FastLED.setTemperature( BlackLightFluorescent );
                break;
                case 227:
                  dinamicMode = 0;
                  NextionSYNQ(1);
                  FastLED.setTemperature( MercuryVapor );
                break;
                case 228:
                  dinamicMode = 0;
                  NextionSYNQ(1);
                  FastLED.setTemperature( SodiumVapor );
                break;
                case 229:
                  dinamicMode = 0;
                  NextionSYNQ(1);
                  FastLED.setTemperature( MetalHalide );
                break;
                case 230:
                  dinamicMode = 0;
                  NextionSYNQ(1);
                  FastLED.setTemperature( HighPressureSodium );
                break;
              }
            break;
  // synq
            case 'Q':
              page = rs485Serial.read();
Serial.print("Synq Page = "); Serial.println(page);
              switch(page)
              {
                case 'L': // synq Led page SIMPLE
                  pinVcc = rs485Serial.read();
                  nameObject = rs485Serial.read();  // Name
                  if(pinVcc==22 && nameObject==0) // synq Led Salon
                  {
                    NextionSYNQ(0);
                  }
                break;
                /*
                case 'C': // synq Led page AVANCE
                  pinVcc = rs485Serial.read();
                  nameObject = rs485Serial.read();  // Name
                  if(pinVcc==22 && nameObject==0) // synq Led Salon
                  {
                    NextionSYNQ(0);
                  }
                break;
                */
                case 'P':  // Print leds colours
                  pinVcc = rs485Serial.read();
                  nameObject = rs485Serial.read();  // Name
                  if(pinVcc==22 && nameObject==0) // synq Led Salon
                  {
                    NextionSYNQ(3);
                  }
                break;
                case 'R':  // Print led colour
                  pinVcc = rs485Serial.read();
                  nameObject = rs485Serial.read();  // Name
                  if(pinVcc==22 && nameObject==0) // synq Led Salon
                  {
                    NextionSYNQ(0, numLed);
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

void NextionSYNQ(byte a, byte _numLed) {
// Read & send value led
  switch(a)
  {
    case 0:
      byte HUE, SAT, VAL;
      HUE = ledSalonHSV[_numLed][0], SAT = ledSalonHSV[_numLed][1], VAL = ledSalonHSV[_numLed][2];
      rs485Serial.print("pLedSelect.nHUE.val=" + String(HUE)); FF();
      rs485Serial.print("pLedSelect.hHUE.val=" + String(HUE)); FF();
      rs485Serial.print("pLedSelect.nSAT.val=" + String(SAT)); FF();
      rs485Serial.print("pLedSelect.hSAT.val=" + String(SAT)); FF();
      rs485Serial.print("pLedSelect.nVAL.val=" + String(VAL)); FF();
      rs485Serial.print("pLedSelect.hVAL.val=" + String(VAL)); FF();
      rs485Serial.print("pLedSelect.tBCK.txt=\"Led " + String(_numLed) + " seleccionado\""); FF();
    break;
  }
  
}
void NextionSYNQ(byte a) {
Serial.print("NextionSYNQ = ");  Serial.println(a);
// SYNQ
  switch(a)
  {
  
    case 0:
      rs485Serial.print("pConnect.pic=9"); FF();
    break;
    case 1: // synq PageLedClaraboyaBasic &  Avance   ambas paginas con tm
      rs485Serial.print("click hpSYNQ,1"); FF();
    break;
    case 2:
      rs485Serial.print("pLedClarAvance.nInterval.val=" + String(ledSalonXcada)); FF();
      
Serial.print("ledSalonXcada = "); Serial.println(ledSalonXcada);
      rs485Serial.print("pConnect.pic=9"); FF();
    break;
  // synq PageLed write LEDS colors
    case 3:
Serial.println("  INIT synqPrintLEDS Salon");
      byte HUE, SAT;
      byte l, k;
      for(byte i = 0; i < ledSalonLEDS; i++)
        {
          HUE = ledSalonHSV[i][0], SAT = ledSalonHSV[i][1];
          int m, n;
          for (int j = 0; j < 32; j++)
          {
            if(j==0 || j==32-1)
            {
              m=j*8;
              n=(j+1)*8;
              if(HUE>=m && HUE<n)
              {
                if(SAT>=0 && SAT<85){
                  rs485Serial.print("pLedClarAvance.l" + String(i) + ".bco=" + String(colorRGB565[2][0])); FF();
                }
                if(SAT>=85 && SAT<170){
                  rs485Serial.print("pLedClarAvance.l" + String(i) + ".bco=" + String(colorRGB565[1][0])); FF();
                }
                if(SAT>=170 && SAT<256){
                  rs485Serial.print("pLedClarAvance.l" + String(i) + ".bco=" + String(colorRGB565[0][0])); FF(); 
                }
              }
            }else{
              m=j*16;
              n=(j+1)*16;
              if(HUE>=m && HUE<n)
              {
                if(SAT>=0 && SAT<85){
                  rs485Serial.print("pLedClarAvance.l" + String(i) + ".bco=" + String(colorRGB565[2][j])); FF(); 
                }
                if(SAT>=85 && SAT<170){
                  rs485Serial.print("pLedClarAvance.l" + String(i) + ".bco=" + String(colorRGB565[1][j])); FF(); 
                }
                if(SAT>=170 && SAT<256){
                  rs485Serial.print("pLedClarAvance.l" + String(i) + ".bco=" + String(colorRGB565[0][j])); FF(); 
                }
              }
            }
          }
          //delay(2);
        }
      NextionSYNQ(1);
    break;
#ifdef _DEBUG_Q_
    case 255:
    if(state==0)
    {
      rs485Serial.print("pageMain.n0.bco=WHITE"); FF();
      state=1;
    }else
    {
      rs485Serial.print("pageMain.n0.bco=GREEN"); FF();
      state=0;
    }
  
      rs485Serial.print("pageBlack.vaSYNQ.val=" + String(nextionPing++)); FF();
    break;
#endif
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
