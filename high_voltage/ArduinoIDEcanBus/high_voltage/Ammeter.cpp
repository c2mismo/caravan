#include "Arduino.h"
#include "Ammeter.h"

Ammeter::Ammeter(const int sensorPin, const int freq, const int calib, const int intervalRead, const int amperMax)
{
  _amperPin = sensorPin;
  _freq = freq;
  _calib = calib;
  _activ = false;
  _intervalRead = intervalRead;
  _average = 512;
  _amperMax = amperMax;
}

void Ammeter::get()
{
  _valMax = _average;
  _valMin = _average;
  _tActu = millis();
  _tMax = _tActu + _freq;

  for (_tActu = _tActu; _tMax > _tActu; _tActu = millis()){
    _Y = (float)analogRead(_amperPin);
    _S = (_alpha*_Y)+((1-_alpha)*_S);
    if (_S > _valMax)_valMax = _S;
    if (_S < _valMin) _valMin = _S;
  }
  _average = (_valMax - _valMin) / 2 + _valMin;

  float _aveDiff = _valMax - _average;
  if ( _aveDiff < 4 )  // Clean noise
  {
    _autoClean = _aveDiff;
    _valMaxClean = _valMax - _autoClean;
  } else {
    _valMaxClean = _valMax - _autoClean;
  }

  _amperRef = _average+_calib;
  //    map(_valMax, _averN, _amperRef, 0, 1)
  _amper = (_valMaxClean-_average)*(1-0)/(_amperRef-_average)+0;

  // SlowAmmper
  if (!_activ)
  {
    _countIntervalRead = 0;
    _intervalReadMin = millis();
    _intervalReadMax = _intervalReadMin+_intervalRead;
    _amperIncrement = 0;
    _activ = true;
  }
  if (_activ) {
    _intervalReadMin = millis();
    if (_intervalReadMin <= _intervalReadMax)
    {
      _amperIncrement += _amper;
      _countIntervalRead++;
    } else {
      _slowAmper = _amperIncrement/_countIntervalRead;
      _activ = false;
    }
  }
}

float Ammeter::getValue()
{
  _valMax = _valMax;
  return _valMax;
}

float Ammeter::getAverage()
{
  _average = _average;
  return _average;
}

float Ammeter::getAmper()
{
  _amper = _amper;
  return _amper;
}

float Ammeter::getAmperSlow()
{
  _slowAmper = _slowAmper;
  return _slowAmper;
}

bool Ammeter::getReady()
{
  get();
  _amper = _amper;
  _amperMax = _amperMax;
  if (_amper <= _amperMax){
    return true;
  } else {
    return false;
  }
}