#include "Arduino.h"
#include "Voltmeter.h"

Voltmeter::Voltmeter(const int sensorPin, const int freq)
{
  _sensor = sensorPin;
  _freq = freq;
  _debounce = 0;
}

void Voltmeter::get()
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
  return _maxVal;
}

float Voltmeter::getAverage()
{
  return _average;
}

float Voltmeter::getVoltage()
{
  _R = (_maxVal - _average)*10;
  _voltage = (float)_R/10;
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
