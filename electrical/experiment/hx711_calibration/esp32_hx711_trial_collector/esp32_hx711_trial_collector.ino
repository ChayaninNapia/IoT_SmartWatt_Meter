#include "HX711.h"

// HX711 wiring for ESP32 Dev Module / NodeMCU-32S.
const int HX711_DOUT_PIN = 32;
const int HX711_SCK_PIN = 33;
const byte HX711_GAIN = 64;

const int TRIAL_SAMPLES = 100;
const int TARE_SAMPLES = 20;
const unsigned long READY_TIMEOUT_MS = 5000;

HX711 scale;

long zeroRaw = 0;
bool isTared = false;
bool reportingEnabled = false;

void printHelp() {
  Serial.println();
  Serial.println("HX711 trial collector commands:");
  Serial.println("  h          : show help");
  Serial.println("  s          : show current tare/status");
  Serial.println("  t          : tare no load");
  Serial.println("  c          : collect one 100-sample trial");
  Serial.println("  r1         : start live raw report");
  Serial.println("  r0         : stop live raw report");
  Serial.println();
  Serial.println("Trial output format:");
  Serial.println("BEGIN_TRIAL,zero_raw=<zero>,samples=<N>");
  Serial.println("sample_idx,time_ms,raw,delta_raw");
  Serial.println("...");
  Serial.println("END_TRIAL");
  Serial.println();
  Serial.println("Live report format:");
  Serial.println("LIVE,time_ms,raw,delta_raw");
  Serial.println();
}

bool waitForReady(unsigned long timeoutMs) {
  const unsigned long startMs = millis();
  while (!scale.is_ready()) {
    if (millis() - startMs >= timeoutMs) {
      return false;
    }
    delay(1);
  }
  return true;
}

bool readValidRaw(long &rawValue) {
  if (!waitForReady(READY_TIMEOUT_MS)) {
    return false;
  }

  rawValue = scale.read();
  return true;
}

bool readAverageRaw(int sampleCount, long &averageRaw) {
  long sum = 0;

  for (int i = 0; i < sampleCount; i++) {
    long raw = 0;
    if (!readValidRaw(raw)) {
      return false;
    }
    sum += raw;
  }

  averageRaw = sum / sampleCount;
  return true;
}

void tareNoLoad() {
  Serial.println("Taring... remove all load and keep setup still.");

  long averageRaw = 0;
  if (!readAverageRaw(TARE_SAMPLES, averageRaw)) {
    Serial.println("ERROR:HX711_NOT_READY_DURING_TARE");
    return;
  }

  zeroRaw = averageRaw;
  isTared = true;

  Serial.print("TARE_COMPLETE,zero_raw=");
  Serial.println(zeroRaw);
}

void printStatus() {
  Serial.print("STATUS,is_tared=");
  Serial.print(isTared ? "1" : "0");
  Serial.print(",zero_raw=");
  Serial.println(zeroRaw);
}

void collectTrial() {
  reportingEnabled = false;

  if (!isTared) {
    Serial.println("ERROR: Run tare first with command t.");
    return;
  }

  Serial.print("BEGIN_TRIAL,zero_raw=");
  Serial.print(zeroRaw);
  Serial.print(",samples=");
  Serial.println(TRIAL_SAMPLES);

  Serial.println("sample_idx,time_ms,raw,delta_raw");

  for (int sampleIndex = 1; sampleIndex <= TRIAL_SAMPLES; sampleIndex++) {
    long raw = 0;
    if (!readValidRaw(raw)) {
      Serial.println("ERROR:HX711_NOT_READY_DURING_TRIAL");
      break;
    }

    const long deltaRaw = raw - zeroRaw;

    Serial.print(sampleIndex);
    Serial.print(",");
    Serial.print(millis());
    Serial.print(",");
    Serial.print(raw);
    Serial.print(",");
    Serial.println(deltaRaw);
  }

  Serial.println("END_TRIAL");
}

void reportLiveSample() {
  if (!scale.is_ready()) {
    return;
  }

  const long raw = scale.read();
  const long deltaRaw = isTared ? raw - zeroRaw : raw;

  Serial.print("LIVE,");
  Serial.print(millis());
  Serial.print(",");
  Serial.print(raw);
  Serial.print(",");
  Serial.println(deltaRaw);
}

void handleCommand(String line) {
  line.trim();
  if (line.length() == 0) {
    return;
  }

  const char command = line.charAt(0);

  if (command == 'h') {
    printHelp();
  } else if (command == 's') {
    printStatus();
  } else if (command == 't') {
    reportingEnabled = false;
    tareNoLoad();
  } else if (command == 'c') {
    collectTrial();
  } else if (command == 'r') {
    if (line == "r1") {
      reportingEnabled = true;
      Serial.println("LIVE_REPORT=ON");
    } else if (line == "r0") {
      reportingEnabled = false;
      Serial.println("LIVE_REPORT=OFF");
    } else {
      reportingEnabled = !reportingEnabled;
      Serial.print("LIVE_REPORT=");
      Serial.println(reportingEnabled ? "ON" : "OFF");
    }
  } else {
    Serial.print("ERROR: Unknown command: ");
    Serial.println(command);
    Serial.println("Type h for help.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN, HX711_GAIN);

  Serial.println();
  Serial.println("HX711 trial collector started.");
  Serial.println("Board target: ESP32 Dev Module");
  Serial.println("HX711 DOUT -> GPIO32, SCK -> GPIO33, gain 32");
  printHelp();
}

void loop() {
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    handleCommand(line);
  }

  if (reportingEnabled) {
    reportLiveSample();
  }
}
