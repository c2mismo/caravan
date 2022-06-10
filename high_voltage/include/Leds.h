#ifndef Leds_H
#define Leds_H

//#include "Arduino.h"

//const int

class Leds {

	public:
		Leds(const int LeftIn, const int LeftOut, const int RightIn, const int RightOut, const int Inverter, const int CHR, const int Home, const int Automatic);

		void on();
		void off();
		void onDelay();
		void offDelay();
		void blinkOn();
		void blinkOff();
	private:
		int _Led;
		int _LeftIn;
		int _LeftOut;
		int _RightIn;
		int _RightOut;
		int _Inverter;
		int _CHR;
		int _Home;
		int _Automatic;
		int _Leds[9] = {_LeftIn, _LeftOut, _RightIn, _RightOut, _Inverter, _CHR, _Home, _Automatic};
		int _i;
		bool m_state;
};

#endif