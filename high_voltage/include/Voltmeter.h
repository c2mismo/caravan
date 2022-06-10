#ifndef Voltmeter_H
#define Voltmeter_H

//#include "Arduino.h"

class Voltmeter {

  public:
    Voltmeter(const int sensorPin, const int freq, const int samples);

    void get();
    void calc();
    float getValue();
    float getAverage();
    float getVoltage();
    bool getReady();
  private:
    int _i;
    int _sensor;
    int _freq;
    float _average;
    int _samples;
    float _voltage;
    int _R;
    int _debounce;
    unsigned long _tMax;
    unsigned long _tActu;
    float _maxVal;
    float _minVal;
    float _Vmax;
    float _Vmid;
    float _Vmin;
    // Movil Average
    float _Y = 0.00;
    float _alpha = 0.86;
    float _S = _Y;
};
#endif
