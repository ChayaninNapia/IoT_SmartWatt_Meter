// Passive buzzer unit test for ESP32-S2.
// Wiring: buzzer signal pin -> GPIO13, buzzer GND -> GND.

const uint8_t BUZZER_PIN = 13;

const int TEMPO_MS = 300;
const char MELODY[] = "ccggaagffeeddc ";
const uint8_t BEATS[] = {
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  4
};
const uint8_t MELODY_LENGTH = sizeof(BEATS) / sizeof(BEATS[0]);

int noteToFrequency(char note) {
  switch (note) {
    case 'c': return 262;
    case 'd': return 294;
    case 'e': return 330;
    case 'f': return 349;
    case 'g': return 392;
    case 'a': return 440;
    case 'b': return 494;
    case 'C': return 523;
    default: return 0;
  }
}

void playNote(char note, unsigned long durationMs) {
  const int frequency = noteToFrequency(note);

  if (frequency == 0) {
    noTone(BUZZER_PIN);
    delay(durationMs);
    return;
  }

  tone(BUZZER_PIN, frequency, durationMs);
  delay(durationMs);
  noTone(BUZZER_PIN);
}

void playStartupBeep() {
  tone(BUZZER_PIN, 1000, 120);
  delay(160);
  tone(BUZZER_PIN, 1500, 120);
  delay(160);
  noTone(BUZZER_PIN);
}

void playMelody() {
  for (uint8_t i = 0; i < MELODY_LENGTH; i++) {
    const unsigned long noteDuration = BEATS[i] * TEMPO_MS;
    playNote(MELODY[i], noteDuration);
    delay(TEMPO_MS / 3);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("Passive buzzer unit test started");
  Serial.println("Buzzer pin: GPIO13");

  playStartupBeep();
}

void loop() {
  Serial.println("Playing buzzer test melody");
  playMelody();

  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);
  delay(2000);
}
