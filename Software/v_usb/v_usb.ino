#include <SoftwareSerial.h>

// TX only on PB4
SoftwareSerial mySerial(3, 4);

const int adcPin = A1;

void setup() {
  mySerial.begin(300);
}

void loop() {
  int adcValue = analogRead(adcPin);

  mySerial.print("ADC Value: ");
  mySerial.println(adcValue);
  

  delay(500);
}