#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 32;
const int LOADCELL_SCK_PIN = 33;

HX711 scale;

void setup() {
  Serial.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN, 64);
}

void loop() {
  if (scale.is_ready()) {
    long reading = scale.read();

    Serial.print("raw:");
    Serial.println(reading);
  }

  delay(50);
}
