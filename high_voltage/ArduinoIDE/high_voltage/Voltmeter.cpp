#include "Arduino.h"
#include "Voltmeter.h"

Voltmeter::Voltmeter(const int sensorPin, const int freq, const int samples)
{
  _sensor = sensorPin;
  _freq = freq;
  _samples = samples;
  _debounce = 0;
}

void Voltmeter::get()
{
  _Vmax=0;
  _Vmid=0;
  _Vmin=0;
  for(_i=0;_i<_samples;_i++){
    calc();
    _Vmax+=_maxVal;
    _Vmid+=_average;
    _Vmin+=_minVal;
  }
  _Vmax=_Vmax/_samples;
  _Vmid=_Vmid/_samples;
  _Vmin=_Vmin/_samples;
  //Serial.print(_minVal); Serial.print("      "); Serial.println(_maxVal);
}

void Voltmeter::calc()
{
  _maxVal = 0.00;
  _minVal = 1024.00;
  _tActu = (unsigned long)millis();
  _tMax = _tActu + _freq;

  for (_tActu = _tActu; _tMax > _tActu; _tActu = (unsigned long)millis()){
    _Y = (float)analogRead(_sensor);
    _S = (_alpha*_Y)+((1-_alpha)*_S);
    if (_S > _maxVal)_maxVal = _S;
    if (_S < _minVal) _minVal = _S;
  }
  _average = (_maxVal - _minVal) / 2 + _minVal;
}

float Voltmeter::getValue()
{
  return _Vmax;
}

float Voltmeter::getAverage()
{
  return _Vmid;
}

float Voltmeter::getVoltage()
{
//  _R = (_Vmax - _Vmid)*10;
//  _voltage = (float)_R/10;
//  _voltage = (_Vmax - _Vmid);
  _voltage = (_Vmid - _Vmin);
  return _voltage;
}

bool Voltmeter::getReady()
{
  getVoltage();
  if (_voltage > 190 && _voltage < 240)
  {
    if (_debounce < 3)
    {
      _debounce++;
      return false;
    } else {
      return true;
    }
  } else {
    _debounce = 0;
    return false;
  }
}
