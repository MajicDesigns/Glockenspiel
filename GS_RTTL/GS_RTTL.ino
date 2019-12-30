#include <MD_RTTTLParser.h>
#include "Actuator.h"
#include "GS_RTTL_Data.h"

// Arduino sketch to play RTTTL tunes on the Glockenspiel
// 
// This implementation is based around a modified child's toy and controls 15 
// solenoids to 'hammer' the glockenspiel according to the notes read from the
// RTTTL string.
// 
// Parsing the RTTTL string is done by the MD_RTTTLParse library, with this code
// focused on coordinating the glockenspiel and solenoids to produce a sound.
//
// Dependencies
// MD_RTTTLParser can be found at https://github.com/MajicDesigns/MD_RTTTLParser
//

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define PRINT(s,v)  { Serial.print(F(s)); Serial.print(v); }
#define PRINTX(s,v) { Serial.print(F(s)); Serial.print("0x"); Serial.print(v, HEX); }
#define PRINTS(s)   { Serial.print(F(s)); }
#define PRINTC(c)   { Serial.print(c); }
#else
#define PRINT(s,v)
#define PRINTX(s,v)
#define PRINTS(s)
#define PRINTC(c)
#endif

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

// Basic definitions
const uint16_t PAUSE_TIME = 1500;  // pause time between melodies
const uint8_t GLOCK_BASE_OCTAVE = 5;
const uint8_t HAMMER_IN_OCTAVE = 7;
const uint8_t MAX_NOTES = 15;  // number of notes in the glockenspiel
const uint8_t HAMMER_ACTIVE_TIME = 10;    // in milliseconds

const uint8_t PIN_LD = 10;    // SPI SS pin
const uint8_t PIN_CLK = 13;   // SPI CLK pin
const uint8_t PIN_DAT = 11;   // SPI DATA (MOSI) pin

const uint8_t PROGMEM hammers[] =
// As the glockenspiel does not have have sharp notes,
// are downgraded to the ordinary note in this table.
{
  /*C*/ 0, /*C#*/ 0,
  /*D*/ 1, /*D#*/ 1,
  /*E*/ 2,
  /*F*/ 3, /*F#*/ 3,
  /*G*/ 4, /*G#*/ 4,
  /*A*/ 5, /*A#*/ 5,
  /*B*/ 6,
};

// Global object
//Actuator A(PIN_DAT, PIN_CLK, PIN_LD, NUM_ACTUATOR); // any pins
Actuator G(PIN_LD, MAX_NOTES);  /// SPI pins
MD_RTTTLParser Tune;

void printDuration(uint32_t t)
// print t milliseconds as minute:second.ms
{
  uint16_t min, sec, ms;

  ms = (t % 1000);
  min = t / (60UL * 1000UL);
  sec = (t - (min * 60UL * 1000UL)) / 1000;

  Serial.print(min);
  Serial.print(F(":"));
  if (sec < 10) Serial.print(F("0"));
  Serial.print(sec);
  Serial.print(F("."));
  Serial.print(ms);
}

void setup(void)
{
  Serial.begin(57600);
  PRINTS("\n[Glockenspiel RTTTL]");

  G.begin();
  Tune.begin();
}

void loop(void)
{
  // Note on/off FSM variables
  static enum { NEXT, START, PLAYING, PAUSE, WAIT_FOR_HAMMER, WAIT_BETWEEN } state = START; // current state
  static uint32_t timeStart = 0;    // millis() timing marker
  static int8_t octaveAdj;          // Octave adjustment based on current song - glockenspiel is limited
  static uint8_t octave;            // next octave note to play
  static int8_t noteId;             // index number of the note to play
  static uint16_t duration;         // note duration
  static uint8_t hammer;            // the glockenspiel hammer to be used for this note
  static uint8_t idxTable = 0;      // index of next song to play

  // Manage reading and playing the note
  switch (state)
  {
  case NEXT:      // set up for next song
    PRINTS("\n-->NEXT");
    idxTable++;
    if (idxTable == ARRAY_SIZE(songTable))
      idxTable = 0;
    state = START;
    break;

  case START: // starting a new melody
    {
      uint8_t octaveLow;      // low watermark

      PRINTS("\n-->START");

      // parse this song and adjust any limits
      Tune.setTune_P(songTable[idxTable]);
      octaveLow = Tune.getOctaveDefault();   // assume all one octave for now
      while (Tune.nextNote(octave, noteId, duration))
        if (octave < octaveLow) octaveLow = octave;

      octaveAdj = GLOCK_BASE_OCTAVE - octaveLow;
      PRINT("\nOctave Adjust ", octaveAdj);
      PRINT(" (lowest ", octaveLow);
      PRINTS(")");

      // now reset to start for the actual playing
      Tune.setTune_P(songTable[idxTable]);
      Serial.print(F("\n")); 
      Serial.print(Tune.getTitle());
      Serial.print(F("  "));
      printDuration(Tune.getTimeToEnd());
      state = PLAYING;
    }
    break;

  case PLAYING:     // playing a melody - get next note
    PRINTS("\n-->PLAYING");
    if (Tune.nextNote(octave, noteId, duration))
    {
      if (noteId == -1)     // this is just a pause
      {
        PRINT("\nPAUSE for ", duration);
        PRINTS("ms");
        timeStart = millis();
        PRINTS("\n-->PLAYING to PAUSE");
        state = PAUSE;
      }
      else
      {
        octave += octaveAdj;      // adjust based on previous parsing

        // get the note hammer and adjust up for the octave played
        hammer = pgm_read_byte(&hammers[noteId]);
        hammer += ((octave - GLOCK_BASE_OCTAVE) * HAMMER_IN_OCTAVE);

        PRINT("\nNOTE_ON hammer ", hammer);
        PRINT(" for ", duration);
        PRINTS("ms");
        G.set(hammer, true);
        PRINTS("\n-->PLAYING to WAIT_FOR_HAMMER");
        timeStart = millis();
        state = WAIT_FOR_HAMMER;
      }
    }
    else
      state = WAIT_BETWEEN;
    break;

  case WAIT_FOR_HAMMER: // wait for the hammer strike time
    if (millis() - timeStart >= HAMMER_ACTIVE_TIME)
    {
      G.set(hammer, false);
      PRINTS("\n-->WAIT_FOR_HAMMER to NEXT STATE");
      state = Tune.isEnded() ? WAIT_BETWEEN : PAUSE;
    }
    break;

  case PAUSE:  // pause note during melody
    if (millis() - timeStart >= duration)
    {
      PRINTS("\n-->PAUSE to NEXT STATE");
      state = Tune.isEnded() ? WAIT_BETWEEN : PLAYING;
    }
    break;

  case WAIT_BETWEEN:  // wait at the end of a melody
    if (millis() - timeStart >= PAUSE_TIME)
    {
      PRINTS("\n-->WAIT_BETWEEN to NEXT");
      state = NEXT;     // start the next melody
    }
    break;
  }
}
