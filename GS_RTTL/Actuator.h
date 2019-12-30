// Actuator.h

#ifndef _ACTUATOR_h
#define _ACTUATOR_h

#include <Arduino.h>

class Actuator
{
public:
  Actuator(uint8_t dat, uint8_t ld, uint8_t clk, uint8_t size);
  Actuator(uint8_t ld, uint8_t size);
  ~Actuator();

  void begin(void);
  void set(uint8_t n, bool b = true);
  void reset(uint8_t n) { set(n, false); };
  void clear(void);
  void update(void) { sendSPI(); };

  void setAutoUpdate(bool b) { _autoUpdate = b; };

private:
  const uint8_t NO_PIN = 255;
  const uint8_t BITS_PER_DATA = 8;

  // Control variables
  uint8_t _dat;
  uint8_t _ld;
  uint8_t _clk;
  uint8_t _size;
  bool _hardwareSPI;
  bool _autoUpdate;

  // Actuator data space
  uint8_t _sizeData;  // size of the data array
  uint8_t* _data;     // allocated in begin();

  // Methods
  void sendSPI(void);
};


#endif

