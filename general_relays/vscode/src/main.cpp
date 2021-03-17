 /*
 * State testing, developer c2mismo 2019.
 * License GNU, see at the end.
 */

//           LIBRARY CONF

#include "Arduino.h"
#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsgValue230_12;
struct can_frame canMsgState230_12;
struct can_frame canMsg12_230;
MCP2515 mcp2515(53);

//  Read canBus 230box Values
byte val230Home;
byte val230In;
byte val230Right;
byte val230Left;
byte val230Amper;

//  Read canBus 230box States
byte state230Automatic;
byte state230LeftIn;
byte state230LeftOut;
byte state230RightIn;
byte state230RightOut;
byte state230CHR;
byte state230Home;
byte state230Inverter;

//  Send canBus 230box Signals
bool testSignal230In;
bool testSignal230Home;
bool testSignal230Right;
bool tsetSignal230Left;

//         END LIBRARY CONF

//             PINS

const byte pinTestSignal230In = 8;
const byte pinTestSignal230Home = 9;
const byte pinTestSignal230Left = 10; // Simul 230 Volt for test
const byte pinTestSignal230Right = 11;

const byte ledVerde = 13;



//            END PINS

//           VARIABLES


//         END VARIABLES

void setup(){
  while (!Serial);
  Serial.begin(115200);
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate (CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  canMsg12_230.can_id = 0x038;
  canMsg12_230.can_dlc = 4;

  pinMode(pinTestSignal230In, INPUT_PULLUP);
  pinMode(pinTestSignal230Home, INPUT_PULLUP);
  pinMode(pinTestSignal230Left, INPUT_PULLUP);
  pinMode(pinTestSignal230Right, INPUT_PULLUP);

  pinMode(ledVerde, OUTPUT);
}

void loop(){


  // Rear data from 230box Values
  val230Home=0;val230In=0;val230Right=0;val230Left=0;val230Amper=0;
  if (mcp2515.readMessage(&canMsgValue230_12) == MCP2515::ERROR_OK)
  {
    val230Home = canMsgValue230_12.data[0];
    val230In = canMsgValue230_12.data[1];
    val230Right = canMsgValue230_12.data[2];
    val230Left = canMsgValue230_12.data[3]; // >=255 ERROR
    val230Amper = canMsgValue230_12.data[4];
  }

  Serial.print(" val230Home: ");
  Serial.println(val230Home);
  Serial.print(" val230In: ");
  Serial.println(val230In);
  Serial.print(" val230Right: ");
  Serial.println(val230Right);
  Serial.print(" val230Left: ");
  Serial.println(val230Left);
  Serial.print(" val230Amper: ");
  Serial.println(val230Amper);
  Serial.println("***************");
  // END rear data from 230box Values

  // Rear data from 230box States
  // Poner valores a CERO
  state230Automatic=0;state230LeftIn=0;state230LeftOut=0;state230RightIn=0;state230RightOut=0;state230CHR=0;state230Home=0;state230Inverter=0;
  if (mcp2515.readMessage(&canMsgState230_12) == MCP2515::ERROR_OK)
  {
    state230Automatic = canMsgState230_12.data[0];
    state230LeftIn = canMsgState230_12.data[1];
    state230LeftOut = canMsgState230_12.data[2];
    state230RightIn = canMsgState230_12.data[3];
    state230RightOut = canMsgState230_12.data[4];
    state230CHR = canMsgState230_12.data[5];
    state230Home = canMsgState230_12.data[6];
    state230Inverter = canMsgState230_12.data[7];
  }

  Serial.print(" state230Automatic: ");
  Serial.println(state230Automatic);
  Serial.print(" state230LeftIn: ");
  Serial.println(state230LeftIn);
  Serial.print(" state230LeftOut: ");
  Serial.println(state230LeftOut);
  Serial.print(" state230RightIn: ");
  Serial.println(state230RightIn);
  Serial.print(" state230RightOut: ");
  Serial.println(state230RightOut);
  Serial.print(" state230CHR: ");
  Serial.println(state230CHR);
  Serial.print(" state230Home: ");
  Serial.println(state230Home);
  Serial.print(" state230Inverter: ");
  Serial.println(state230Inverter);
  Serial.println("***************");
  // END rear data from 230box States

  //  Send data to 230box
  canMsg12_230.data[0] = digitalRead(pinTestSignal230In);
  canMsg12_230.data[1] = digitalRead(pinTestSignal230Home);
  canMsg12_230.data[2] = digitalRead(pinTestSignal230Right);
  canMsg12_230.data[3] = digitalRead(pinTestSignal230Left);
  mcp2515.sendMessage(&canMsg12_230);
  // END send data to 230box


  if (state230Inverter)
  {
    digitalWrite(ledVerde, HIGH);
  }
  if (!state230Inverter)
  {
    digitalWrite(ledVerde, LOW);
  }

  delay(1500);
}

//           FUNCTIONS
//         END FUNCTIONS

/*
  License:

    Copyright (C) 2019  c2mismo.

    This file is part of 230box.

    230box is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This 230box is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this 230box, see COPYING.  If not, see <https://www.gnu.org/licenses/>.

    You can download a full copy of 230box at <https://github.com/c2mismo>.
 */
