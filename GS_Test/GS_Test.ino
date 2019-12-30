#include "Actuator.h"

// Basic definitions
const uint8_t NUM_ACTUATOR = 15;  // number of actuators
const uint8_t PIN_LD = 10;    // SPI SS pin
const uint8_t PIN_CLK = 13;   // SPI CLK pin
const uint8_t PIN_DAT = 11;   // SPI DATA (MOSI) pin

// Actuators object
//Actuator A(PIN_DAT, PIN_CLK, PIN_LD, NUM_ACTUATOR);
Actuator A(PIN_LD, NUM_ACTUATOR);

void setup(void)
{
  Serial.begin(57600);
  Serial.print(F("\n[Glockenspiel Test]"));

  A.begin();
}

void loop(void)
{
  static bool b = true;

  for (uint8_t i = 0; i < NUM_ACTUATOR; i++)
  {
    Serial.print(F("\nbit: ")); Serial.print(i);
    A.set(i, true);
    delay(12);
    A.set(i, false);
    delay(500);
  }
  b = !b;
}
