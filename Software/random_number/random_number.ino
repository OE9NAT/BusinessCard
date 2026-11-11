// random_number.ino: Würfel 0-9 für die Visitenkarte (ATtiny85, 6 LEDs als Charlieplex an PB0/PB1/PB2)
// Flashen mit: Sketch -> Mit Programmer hochladen
//
// Ziffernmuster und LED-Zuordnung sind 1:1 aus Blink_number übernommen,
// die Pins werden per Registerzugriff geschaltet (schneller als digitalWrite).
//
// Ablauf:
//   1. Beim Einschalten oder Reset läuft ein Lauflicht einmal im Kreis um alle sechs LEDs
//      und es wird eine neue Zufallszahl 0-9 gewürfelt.
//   2. Diese Zahl bleibt SHOW_RESULT_MS lang stehen.
//   3. Danach laufen alle Ziffern 0-9 einzeln durch (je CYCLE_MS) und das wiederholt sich,
//      bis zum nächsten Reset. Mit CYCLE_AFTER_RESULT = false bleibt stattdessen die
//      gewürfelte Zahl dauerhaft stehen.
//
// Easter Egg: Wird der Reset-Taster dreimal schnell hintereinander gedrückt (jeweils innerhalb von
// RESET_WINDOW_MS nach dem vorigen Start), gibt die Karte das Rufzeichen als Morsecode auf allen
// sechs LEDs aus (MORSE_REPEATS Mal) und zeigt danach wie gewohnt die gewürfelte Zahl.

#include <avr/eeprom.h>
#include <avr/pgmspace.h>

// ---------------- Einstellungen ----------------
const uint16_t FRAME_US           = 8000;  // Zeit für einen kompletten Durchlauf aller leuchtenden LEDs (us)
                                           // 8000 us = 125 Hz, egal ob 3 oder 6 LEDs leuchten
const uint16_t SHOW_RESULT_MS     = 4000;  // Ergebnis anzeigen
const uint16_t CYCLE_MS           = 1000;  // Anzeigedauer pro Ziffer im Durchlauf
const bool     CYCLE_AFTER_RESULT = true;  // false = Ergebnis bleibt stehen

const char     CALLSIGN[]         = "OE9NAT";  // Nur A-Z und 0-9
const uint8_t  RESETS_FOR_EGG     = 3;     // so oft schnell Reset drücken
const uint16_t RESET_WINDOW_MS    = 2000;  // maximaler Abstand zwischen zwei Resets
const uint16_t MORSE_UNIT_MS      = 250;   // Länge eines Punktes (Strich = 3, Pause zwischen Zeichen = 3)
const uint8_t  MORSE_REPEATS      = 2;     // wie oft das Rufzeichen gesendet wird

// Lauflicht beim Start: LED-Nummern im Kreis, im Uhrzeigersinn von oben (D5, D1, D3, D4, D2, D0)
const uint8_t  RING[6]            = {5, 1, 3, 4, 2, 0};
const uint16_t RING_STEP_MS       = 100;   // Zeit pro LED im Lauflicht
const uint8_t  RING_ROUNDS        = 2;     // Anzahl Runden

const uint16_t EE_RANDOM  = 0;             // EEPROM-Adresse Zufallszustand (4 Byte)
const uint16_t EE_COUNTER = 4;             // EEPROM-Adresse Reset-Zähler (1 Byte)

// ---------------- Aus Blink_number übernommen ----------------
// Pins 0, 1, 2 sind beim ATtiny85 PB0, PB1, PB2 (Bit 0-2 in DDRB/PORTB)
const uint8_t LED_MASK = 0x07;

// Map LED indices 0-5 to {Anode Pin, Cathode Pin}
const int ledMap[6][2] = {
  {0, 1}, {1, 0}, {1, 2}, {2, 1}, {0, 2}, {2, 0}
};

// Number Patterns (using 0-5 LED indices, -1 is end of list)
const int numPatterns[10][7] = {
  {2, 3, 4, -1, -1, -1, -1},       // 0
  {1, 2, -1, -1, -1, -1, -1},      // 1
  {5, 1, 2, 4, -1, -1, -1},        // 2
  {5, 1, 3, 4, -1, -1, -1},        // 3
  {0, 3, 1, -1, -1, -1, -1},       // 4
  {5, 0, 3, 4, -1, -1, -1},        // 5
  {1, 2, 4, 3, -1, -1, -1},        // 6
  {5, 1, 2, -1, -1, -1, -1},       // 7
  {5, 0, 3, 4, 2, 1, -1},          // 8
  {0, 5, 1, 2, -1, -1, -1}         // 9
};

// Direkter Registerzugriff: deutlich schneller als pinMode()/digitalWrite()
void resetPins() {
  DDRB  &= ~LED_MASK;   // alle drei Pins hochohmig
  PORTB &= ~LED_MASK;   // Pull-ups aus
}

void lightLED(int index) {
  resetPins();
  DDRB  |= _BV(ledMap[index][0]) | _BV(ledMap[index][1]);   // nur die zwei Pins dieser LED treiben
  PORTB |= _BV(ledMap[index][0]);                            // Anode high, Kathode bleibt low
}

// ---------------- Morse-Tabellen ----------------
// obere 3 Bit = Länge, untere Bits = Muster von links nach rechts (1 = Strich)
const uint8_t MORSE_LETTERS[26] PROGMEM = {
  0x41, 0x88, 0x8A, 0x64, 0x20, 0x82, 0x66, 0x80, 0x40, 0x87, 0x65, 0x84, 0x43,
  0x42, 0x67, 0x86, 0x8D, 0x62, 0x60, 0x21, 0x61, 0x81, 0x63, 0x89, 0x8B, 0x8C
};
const uint8_t MORSE_DIGITS[10] PROGMEM = {
  0xBF, 0xAF, 0xA7, 0xA3, 0xA1, 0xA0, 0xB0, 0xB8, 0xBC, 0xBE
};

// ---------------- Anzeige ----------------
bool counterCleared = false;

// Löscht den Reset-Zähler, sobald das Zeitfenster für den nächsten Reset abgelaufen ist
void housekeeping() {
  if (!counterCleared && millis() >= RESET_WINDOW_MS) {
    eeprom_update_byte((uint8_t *)EE_COUNTER, 0);
    counterCleared = true;
  }
}

void showDigit(uint8_t n, uint16_t ms) {
  uint8_t count = 0;                               // Anzahl der LEDs dieser Ziffer
  while (count < 7 && numPatterns[n][count] != -1) count++;
  uint16_t slice = FRAME_US / count;               // je mehr LEDs, desto kürzer pro LED
  unsigned long timer = millis();
  while (millis() - timer < ms) {
    for (uint8_t i = 0; i < count; i++) {
      lightLED(numPatterns[n][i]);
      delayMicroseconds(slice);
    }
    housekeeping();
  }
  resetPins();
}

// Lauflicht: jede LED einzeln der Reihe nach im Kreis
void ringAnimation() {
  for (uint8_t r = 0; r < RING_ROUNDS; r++) {
    for (uint8_t i = 0; i < 6; i++) {
      lightLED(RING[i]);
      delay(RING_STEP_MS);
    }
  }
  resetPins();
}

// ---------------- Morsecode ----------------
// Alle sechs LEDs für ms Millisekunden leuchten lassen (schnelles Durchschalten)
void lightAll(uint16_t ms) {
  unsigned long timer = millis();
  while (millis() - timer < ms) {
    for (uint8_t i = 0; i < 6; i++) {
      lightLED(i);
      delayMicroseconds(FRAME_US / 6);
    }
  }
  resetPins();
}

void pauseMs(uint16_t ms) {
  resetPins();
  delay(ms);
}

void sendChar(char c) {
  uint8_t code;
  if (c >= 'A' && c <= 'Z')      code = pgm_read_byte(&MORSE_LETTERS[c - 'A']);
  else if (c >= '0' && c <= '9') code = pgm_read_byte(&MORSE_DIGITS[c - '0']);
  else return;
  uint8_t len = code >> 5;
  for (uint8_t k = 0; k < len; k++) {
    bool dah = (code >> (len - 1 - k)) & 1;
    lightAll(dah ? 3 * MORSE_UNIT_MS : MORSE_UNIT_MS);
    if (k < len - 1) pauseMs(MORSE_UNIT_MS);       // Pause zwischen Punkten und Strichen
  }
}

void playMorse(const char *s) {
  for (; *s; s++) {
    sendChar(*s);
    if (s[1]) pauseMs(3 * MORSE_UNIT_MS);          // Pause zwischen Zeichen
  }
}

// ---------------- Zufall ----------------
uint32_t xorshift32(uint32_t x) {
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

// 32 Bit Rauschen aus dem niedrigsten ADC-Bit am schwebenden Pin PB2 (ADC1)
uint32_t adcNoise() {
  ADMUX  = 0x01;                                  // Referenz VCC, Kanal ADC1 (PB2)
  ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1);   // ADC an, Prescaler 64
  uint32_t n = 0;
  for (uint8_t i = 0; i < 32; i++) {
    ADCSRA |= _BV(ADSC);
    while (ADCSRA & _BV(ADSC)) {}
    n = (n << 1) | (ADC & 1);
  }
  ADCSRA = 0;                                     // ADC wieder aus
  return n;
}

uint8_t rollDigit() {
  uint32_t s = eeprom_read_dword((const uint32_t *)EE_RANDOM);  // leeres EEPROM = 0xFFFFFFFF, ist ok
  s ^= adcNoise();
  if (s == 0) s = 0x1234ABCDUL;                    // xorshift darf nie 0 sein
  for (uint8_t i = 0; i < 4; i++) s = xorshift32(s);
  eeprom_update_dword((uint32_t *)EE_RANDOM, s);   // neuer Zustand für den nächsten Reset
  return (uint8_t)((s >> 16) % 10);
}

// ---------------- Hauptprogramm ----------------
uint8_t result;

void setup() {
  resetPins();

  // Schnelle Resets zählen: der Zähler wird nach RESET_WINDOW_MS im Normalbetrieb wieder gelöscht
  uint8_t cnt = eeprom_read_byte((const uint8_t *)EE_COUNTER);
  if (cnt == 0xFF) cnt = 0;                        // leeres EEPROM
  cnt++;
  eeprom_update_byte((uint8_t *)EE_COUNTER, cnt);

  result = rollDigit();

  ringAnimation();                                 // Lauflicht vor dem Start

  if (cnt >= RESETS_FOR_EGG) {                     // Easter Egg: Rufzeichen in Morsecode
    eeprom_update_byte((uint8_t *)EE_COUNTER, 0);
    counterCleared = true;
    for (uint8_t r = 0; r < MORSE_REPEATS; r++) {
      playMorse(CALLSIGN);
      pauseMs(7 * MORSE_UNIT_MS);
    }
  }
}

void loop() {
  if (!CYCLE_AFTER_RESULT) {
    showDigit(result, 1000);       // Ergebnis dauerhaft anzeigen
    return;
  }
  showDigit(result, SHOW_RESULT_MS);
  for (;;) {                       // danach alle Ziffern einzeln durchlaufen
    for (uint8_t n = 0; n < 10; n++) showDigit(n, CYCLE_MS);
  }
}
