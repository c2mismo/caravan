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
byte val230Left;
byte val230Right;
byte val230In;
byte val230Amper;

//  Read canBus 230box States
bool state230Automatic;
bool state230LeftIn;
bool state230LeftOut;
bool state230RightIn;
bool state230RightOut;
bool state230CHR;
bool state230Home;
bool state230Inverter;

//  Send canBus 230box Signals
bool signal230Left;
bool signal230Right;

//         END LIBRARY CONF

//             PINS

const int pinSignal230Left = 7;
const int pinSignal230Right = 8;

const int ledVerde = 2;
const int reles16_1 = A0;

//            END PINS

//           VARIABLES


//         END VARIABLES

void setup(){
  Serial.begin(115200);
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate (CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  canMsg12_230.can_id = 0x038;
  canMsg12_230.can_dlc = 2;

  pinMode(pinSignal230Left, INPUT_PULLUP);
  pinMode(pinSignal230Right, INPUT_PULLUP);

  pinMode(ledVerde, OUTPUT);
  pinMode(reles16_1, OUTPUT);
}

void loop(){


  // Rear data from 230box Values
  if (mcp2515.readMessage(&canMsgValue230_12) == MCP2515::ERROR_OK)
  {
    val230Left = canMsgValue230_12.data[0]; // >=255 ERROR
    val230Right = canMsgValue230_12.data[1];
    val230In = canMsgValue230_12.data[2];
    val230Amper = canMsgValue230_12.data[3];
  }
  Serial.print(" val230Left: ");
  Serial.println(val230Left);
  Serial.print(" val230Right: ");
  Serial.println(val230Right);
  Serial.print(" val230In: ");
  Serial.println(val230In);
  Serial.println("***************");
  // END rear data from 230box Values

  // Rear data from 230box States
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
  // END rear data from 230box States

  //  Send data to 230box
  if (digitalRead(pinSignal230Left))
  {
    digitalWrite(ledVerde, LOW);
    signal230Left = false;
    canMsg12_230.data[0] = signal230Left;
  } else {
    digitalWrite(ledVerde, HIGH);
    signal230Left = true;
    canMsg12_230.data[0] = signal230Left;
  }
  if (digitalRead(pinSignal230Right))
  {
    digitalWrite(reles16_1, HIGH);
    signal230Right = false;
    canMsg12_230.data[1] = signal230Right;
  } else {
    digitalWrite(reles16_1, LOW);
    signal230Right = true;
    canMsg12_230.data[1] = signal230Right;
  }
  mcp2515.sendMessage(&canMsg12_230);
  // END send data to 230box

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
