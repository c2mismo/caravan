
#include <SoftwareSerial.h>

SoftwareSerial rs485Serial(11, 12); // RX, TX

char nameObject;
byte pinVcc;
bool inverterState;


const byte inverterPin = 13;

void setup() {
  Serial.begin(9600);
  rs485Serial.begin(57600);
}

void loop() {
  rs485SerialRead();

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
 * <init> <check> <DIR> <CMD> <Pin Vcc>
 *    #      3      T    R
 *  0x23    0x03   0x47 0x52
 * 
  */
  if(rs485Serial.available()>2)
  {
    char initData = rs485Serial.read();
// Check the start of the data string
    if(initData=='#')
    {
      uint8_t checkData = rs485Serial.read();
Serial.print("checkData = "); Serial.println(checkData);
      unsigned long tmr1 = millis();
      bool minLength = true;
  // Verify that the entire data string has been received
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
  // DIR only this arduino
Serial.print("dir = "); Serial.println(dir);
        if(dir == 'T')
        {
          char cmd = rs485Serial.read();
Serial.print("cmd = "); Serial.println(cmd);

          switch(cmd)
          {
  // RELAYS
            case 'R':
              nameObject = rs485Serial.read();
Serial.print("nameObject = "); Serial.println(nameObject);
              if(nameObject == 'I')
              {
                inverterState = rs485Serial.read();
                digitalWrite(inverterPin, inverterState);
                if( inverterState )
                {
                  FF(); rs485Serial.print("pageMain.tInverter.picc=2"); FF();
                } else {
                  FF(); rs485Serial.print("pageMain.tInverter.picc=1"); FF();
                }
                sendStateInverter();
              }
            break;
  // SYNQ
            case 'Q':
              nameObject = rs485Serial.read();
              if(nameObject == 'M')
              {
                NextionSYNQ(0);
              }
            break;
          }
        }
      }
    }
  }
}

void NextionSYNQ(byte a) {
  switch(a)
  {
  // synq PageMain
    case 0:
Serial.print("NextionSYNQ = "); Serial.println(a);
      if( inverterState )
      {
        FF(); rs485Serial.print("pageMain.tInverter.picc=2"); FF();
      } else {
        FF(); rs485Serial.print("pageMain.tInverter.picc=1"); FF();
      }
      rs485Serial.print("click EndSynqHtensi,1"); FF();
    break;
  }
}

void sendStateInverter() {
  delay(1);
Serial.print("invarter status = "); Serial.println(String(digitalRead(inverterPin)));
  byte bufSend[] = { '#', 4, 'V', 'Q', 'I', digitalRead(inverterPin) };
  rs485Serial.write(bufSend, 6);
}

void FF(){
  rs485Serial.print("\xFF\xFF\xFF");
}
