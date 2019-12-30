
#include <SPI.h>
#include "Actuator.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define PRINT(s,v)  { Serial.print(F(s)); Serial.print(v); }
#define PRINTX(s,v) { Serial.print(F(s)); Serial.print("0x"); Serial.print(v, HEX); }
#define PRINTS(s)   { Serial.print(F(s)); }
#else
#define PRINT(s,v)
#define PRINTX(s,v)
#define PRINTS(s)
#endif

Actuator::Actuator(uint8_t dat, uint8_t ld, uint8_t clk, uint8_t size):
  _dat(dat), _ld(ld), _clk(clk), _size(size), _hardwareSPI(false)
{
  setAutoUpdate(true);
}

Actuator::Actuator(uint8_t ld, uint8_t size):
  _dat(NO_PIN), _ld(ld), _clk(NO_PIN), _size(size), _hardwareSPI(true)
{
  setAutoUpdate(true);
}

Actuator::~Actuator(void)
{
  free(_data);
}

void Actuator::begin(void)
{
  // set and initialise the SPI pins if they are defined
  if (_dat != NO_PIN) pinMode(_dat, OUTPUT);
  if (_clk != NO_PIN) pinMode(_clk, OUTPUT);
  pinMode(_ld, OUTPUT);
  digitalWrite(_ld, HIGH);

  // initialise SPI hardware if necessary
  if (_hardwareSPI) SPI.begin();

  // allocate the memory for the actuator bits
  _sizeData = _size / BITS_PER_DATA;
  if (_size % BITS_PER_DATA != 0) _sizeData++;
  _data = (uint8_t*)malloc(_sizeData);
  clear();
}

void Actuator::set(uint8_t n, bool b)
{
  if (n >= _size) // bounds checking
    return;

  uint8_t idx = n / BITS_PER_DATA;
  uint8_t mask = 1 << (n - (idx * BITS_PER_DATA));

  _data[idx] &= ~mask; // clear the bit
  if (b) _data[idx] |= mask;  // set if required

  if (_autoUpdate) sendSPI();
}

void Actuator::clear(void)
{
  for (uint8_t i = 0; i < _sizeData; i++)
    _data[i] = 0;

  if (_autoUpdate) sendSPI();
}

void Actuator::sendSPI(void)
{
  // initialize the SPI transaction
  if (_hardwareSPI)
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(_ld, LOW);

  // shift out the data
  if (_hardwareSPI)
  {
    PRINTS(" send:"); 
    for (uint8_t i = 0; i < _sizeData; i++)
    {
      PRINTX(" .", _data[i]);
      SPI.transfer(_data[i]);
    }
  }
  else  // not hardware SPI - bit bash it out
  {
    for (uint8_t i = 0; i < _sizeData; i++)
    {
      PRINTX(" 0x", _data[i]);
      shiftOut(_dat, _clk, MSBFIRST, _data[i]);
    }
  }

  // end the SPI transaction
  digitalWrite(_ld, HIGH);
  if (_hardwareSPI)
    SPI.endTransaction();
}
