
// #include "arduino.h"
#include <SoftwareSerial.h>
SoftwareSerial rs485Serial(16, 14); // RX, TX

#include <FastLED.h>
#define LED_PIN     5
#define NUM_LEDS    130
byte valHue = 50, valSat = 255, valVal = 255;
CRGB ledSalon[NUM_LEDS];  // Con el constructor creamos el array*/


void setup() {
  Serial.begin(9600);
  rs485Serial.begin(9600);
  
  FastLED.addLeds<WS2813, LED_PIN, GRB>(ledSalon, NUM_LEDS);
  LEDS.setBrightness(20);

}

void loop() {
for (int i = 0; i < NUM_LEDS; i = i + 1)
        { ledSalon[i] = CRGB::Black; }

  // uint8_t getLedStatus=leds[1], getR888=leds[1][0], getG888=leds[1][1], getB888=leds[1][2];

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
    char data1 = rs485Serial.read();
Serial.println("***********************"); Serial.println("***********************");
Serial.print("INIT = "); Serial.println(data1);
    if(data1=='#')
    {
      int checkData = rs485Serial.read();
      unsigned long tmr1 = millis();
      bool minLength = true;
Serial.print("checkData = "); Serial.println(checkData);
      while(rs485Serial.available()<checkData)
      {
        if((millis()-tmr1)>100)
        {
          minLength=false;
          break;
        }
      }
      char dir;
// Verify that the entire data string that has been received does not exceed the expected
      if(minLength==true && rs485Serial.available()==checkData)
      {
        dir = rs485Serial.read();
        if(dir == 'S')
        {
          char cmd = rs485Serial.read();
Serial.print("cmd = "); Serial.println(cmd);
// TEMP send ONLY repetidor <DIR> 'G' <CMD> 'N'   (cambiar al Nextion)
          switch(cmd)
          {
            case 'L':
              //
            break;
          }
        }
      }
    }
  }
}

void ledShow(){
  for (int i = 0; i < NUM_LEDS; i = i + 1) { ledSalon[i] = CHSV( valHue, valSat, valVal); }
}
