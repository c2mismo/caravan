#ifndef Voltmeter_H
#define Voltmeter_H

//#include "Arduino.h"

class Voltmeter {

	public:
		Voltmeter(const int sensorPin, const int freq);

		void get();
		float getValue();
		float getAverage();
		float getVoltage();
		bool getReady();
	private:
		int _sensor;
		int _freq;
		float _average;
		float _voltage;
		int _R;
		int _debounce;
		unsigned long _tMax;
		unsigned long _tActu;
		float _maxVal;
		float _minVal;
		// Movil Average
		float _Y = 0.00;
		float _alpha = 0.86;
		float _S = _Y;
};
#endif