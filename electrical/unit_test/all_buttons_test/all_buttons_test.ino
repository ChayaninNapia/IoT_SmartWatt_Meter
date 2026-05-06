// Five-button input unit test for ESP32-S2.
//
// Wiring from project GPIO Mapping:
//   Button 1 -> GPIO25
//   Button 2 -> GPIO26
//   Button 3 -> GPIO27
//   Button 4 -> GPIO14
//   Button 5 -> GPIO16
//
// Current enclosure wiring appears to be active-high:
//   released = LOW and pressed = HIGH.
// If your wiring is GPIO -> button -> GND, change BUTTON_PIN_MODE to
// INPUT_PULLUP and BUTTON_PRESSED_LEVEL to LOW.

struct ButtonTest {
  const char *name;
  const char *unitTestId;
  uint8_t pin;
  bool stablePressed;
  bool lastRawPressed;
  unsigned long lastRawChangeMs;
  unsigned long pressedAtMs;
  uint32_t pressCount;
  bool longPressReported;
};

const unsigned long DEBOUNCE_MS = 35;
const unsigned long LONG_PRESS_MS = 1500;
const unsigned long STATUS_INTERVAL_MS = 5000;
const uint8_t BUTTON_PIN_MODE = INPUT_PULLDOWN;
const uint8_t BUTTON_PRESSED_LEVEL = HIGH;

ButtonTest buttons[] = {
  {"BTN1", "UT-S-007", 25, false, false, 0, 0, 0, false},
  {"BTN2", "UT-S-008", 26, false, false, 0, 0, 0, false},
  {"BTN3", "UT-S-009", 27, false, false, 0, 0, 0, false},
  {"BTN4", "UT-S-010", 14, false, false, 0, 0, 0, false},
  {"BTN5_ZERO_CAL", "UT-S-017", 16, false, false, 0, 0, 0, false},
};

const uint8_t BUTTON_COUNT = sizeof(buttons) / sizeof(buttons[0]);

unsigned long lastStatusMs = 0;

bool readPressed(uint8_t pin) {
  return digitalRead(pin) == BUTTON_PRESSED_LEVEL;
}

const char *rawLevelName(uint8_t pin) {
  return digitalRead(pin) == HIGH ? "HIGH" : "LOW";
}

void printButtonMapping() {
  Serial.println();
  Serial.println("Five-button unit test started");
  Serial.print("Pin mode: ");
  Serial.println(BUTTON_PIN_MODE == INPUT_PULLUP ? "INPUT_PULLUP" : "INPUT_PULLDOWN");
  Serial.print("Pressed level: ");
  Serial.println(BUTTON_PRESSED_LEVEL == HIGH ? "HIGH" : "LOW");
  Serial.println("Press and release each button one at a time.");
  Serial.println();
  Serial.println("Mapping:");

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    Serial.print("  ");
    Serial.print(buttons[i].unitTestId);
    Serial.print(" ");
    Serial.print(buttons[i].name);
    Serial.print(" -> GPIO");
    Serial.println(buttons[i].pin);
  }

  Serial.println();
}

void printEvent(const ButtonTest &button, const char *eventName, unsigned long now) {
  Serial.print("[");
  Serial.print(now);
  Serial.print(" ms] ");
  Serial.print(button.unitTestId);
  Serial.print(" ");
  Serial.print(button.name);
  Serial.print(" GPIO");
  Serial.print(button.pin);
  Serial.print(" ");
  Serial.print(eventName);
  Serial.print(" count=");
  Serial.println(button.pressCount);
}

void printStatus(unsigned long now) {
  Serial.print("[");
  Serial.print(now);
  Serial.print(" ms] current states: ");

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    Serial.print(buttons[i].name);
    Serial.print("=");
    Serial.print(buttons[i].stablePressed ? "PRESSED" : "released");
    Serial.print("(");
    Serial.print(rawLevelName(buttons[i].pin));
    Serial.print(")");

    if (i + 1 < BUTTON_COUNT) {
      Serial.print(", ");
    }
  }

  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  const unsigned long now = millis();
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    pinMode(buttons[i].pin, BUTTON_PIN_MODE);
    buttons[i].stablePressed = readPressed(buttons[i].pin);
    buttons[i].lastRawPressed = buttons[i].stablePressed;
    buttons[i].lastRawChangeMs = now;
    buttons[i].pressedAtMs = buttons[i].stablePressed ? now : 0;
  }

  printButtonMapping();
  printStatus(now);
}

void loop() {
  const unsigned long now = millis();

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    ButtonTest &button = buttons[i];
    const bool rawPressed = readPressed(button.pin);

    if (rawPressed != button.lastRawPressed) {
      button.lastRawPressed = rawPressed;
      button.lastRawChangeMs = now;
    }

    if ((now - button.lastRawChangeMs) >= DEBOUNCE_MS &&
        rawPressed != button.stablePressed) {
      button.stablePressed = rawPressed;

      if (button.stablePressed) {
        button.pressCount++;
        button.pressedAtMs = now;
        button.longPressReported = false;
        printEvent(button, "PRESSED", now);
      } else {
        printEvent(button, "RELEASED", now);
        button.pressedAtMs = 0;
        button.longPressReported = false;
      }
    }

    if (button.stablePressed &&
        !button.longPressReported &&
        button.pressedAtMs != 0 &&
        (now - button.pressedAtMs) >= LONG_PRESS_MS) {
      button.longPressReported = true;
      printEvent(button, "LONG_PRESS", now);
    }
  }

  if ((now - lastStatusMs) >= STATUS_INTERVAL_MS) {
    printStatus(now);
    lastStatusMs = now;
  }
}
