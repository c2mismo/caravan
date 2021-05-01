#ifndef Ammeter_H
#define Ammeter_H

class Ammeter {

	public:
		Ammeter(const int amperPin, const int freq, const int calib, const int intervalRead, const int amperMax);

		void get();
		float getValue();
		float getAverage();
		float getAmper();
		float getAmperSlow();
		bool getReady();

	private:
		int _amperPin;
		int _freq;
		float _calib;
		int _intervalRead;
		bool _activ;
		float _average;
		float _amperMax;

		float _valMax;
		float _valMin;
		unsigned long _tMax;
		unsigned long _tActu;
    float _amper;
		float _amperRef;

		int _countIntervalRead;
		unsigned long _intervalReadMin;
		unsigned long _intervalReadMax;
		float _amperIncrement;
		float _slowAmper;

		float _autoClean;
		float _aveDiff;
		float _valMaxClean;

		// Movil Average
		float _Y = 0.00;
		float _alpha = 0.24;
		float _S = _Y;
};

#endif