const int pins[] = {0, 1, 2};
const int povDelay = 40;        // Time each LED stays on (ms)
const int displayDuration = 2000; // Duration for each number (ms)

// Map LED indices 0-5 to {Anode Pin, Cathode Pin}
const int ledMap[6][2] = {
  {0, 1}, {1, 0}, {1, 2}, {2, 1}, {0, 2}, {2, 0}
};

// Number Patterns (using 0-5 LED indices, -1 is end of list)
const int numPatterns[10][7] = {
  {2, 3, 4, -1, -1, -1, -1},       // 0
  {1, 2, -1, -1, -1, -1, -1},       // 1
  {5, 1, 2, 4, -1, -1, -1},       // 2
  {5, 1, 3, 4, -1, -1, -1},       // 3
  {0, 3, 1, -1, -1, -1, -1},       // 4
  {5, 0, 3, 4, -1, -1, -1},       // 5
  {1, 2, 4, 3, -1, -1, -1},       // 6
  {5, 1, 2, -1, -1, -1, -1},       // 7
  {5, 0, 3, 4, 2, 1, -1},       // 8
  {0, 5, 1, 2, -1, -1, -1}        // 9
};

void setup() { resetPins(); }

void resetPins() {
  for (int i = 0; i < 3; i++) {
    pinMode(pins[i], INPUT);
    digitalWrite(pins[i], LOW);
  }
}

void lightLED(int index) {
  resetPins();
  pinMode(ledMap[index][0], OUTPUT);
  pinMode(ledMap[index][1], OUTPUT);
  digitalWrite(ledMap[index][0], HIGH);
  digitalWrite(ledMap[index][1], LOW);
}

void loop() {
  for (int n = 0; n < 10; n++) {
    unsigned long timer = millis();
    while (millis() - timer < displayDuration) {
      for (int i = 0; i < 7; i++) {
        int ledIdx = numPatterns[n][i];
        if (ledIdx == -1) break;
        lightLED(ledIdx);
        delay(povDelay);
      }
    }
  }
}
