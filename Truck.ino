#include <Arduino.h>

// === Konfigurasi PIN Digital Input ===
const int digitalInputPins[19] = {
  2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 
  12, 13, 14, 15, 16, 17, 18, 19, 20
};

// === Nama Input ===
const char* digitalInputNames[19] = {
  "seatbelt", "keyStarter", "handbrake", "horn", "hazard",
  "keyACC", "keyOn", "ambientLamp", "mainLamp", "fogLamp",
  "leftSign", "rightSign", "exhaustBrake", "intWiper", "highBeamLamp",
  "wiperOff", "highWiper", "wiperFluid", "lowWiper"
};

// === Konfigurasi PIN Digital Output (Relay) ===
const int digitalOutputPins[10] = {
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31
};

// === Nama Output Sesuai Format dari Unity ===
const char* digitalOutputNames[10] = {
  "SignL",     // DO1 - Relay Left Sign
  "AxleLock",  // DO2
  "ExBrake",   // DO3
  "OilMal",    // DO4
  "Handbrk",   // DO5
  "BatMal",    // DO6
  "AmbLamp",   // DO7
  "HiBeam",    // DO8
  "SignR",     // DO9 - Relay Right Sign
  "FuelMal"    // DO10
};

// === Variabel status ===
bool digitalOutputStates[10] = {false};
bool leftSignBlink = false;
bool rightSignBlink = false;
bool hazardActive = false;

unsigned long lastBlinkTime = 0;
bool blinkState = false;
const unsigned long blinkInterval = 500;  // ms

// Tambahkan array untuk menyimpan status sebelumnya
int lastInputStates[19] = {0};

void setup() {
  Serial.begin(115200);

  // Inisialisasi input
  for (int i = 0; i < 19; i++) {
    pinMode(digitalInputPins[i], INPUT_PULLUP);
  }

  // Inisialisasi output
  for (int i = 0; i < 10; i++) {
    pinMode(digitalOutputPins[i], OUTPUT);
    digitalWrite(digitalOutputPins[i], HIGH); // Relay aktif LOW
  }
}

void loop() {
  handleSerial();
  updateBlinkingRelays();
  reportInputs();
  delay(100);
}

void handleSerial() {
  while (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("SignL:")) {
      leftSignBlink = (input.endsWith("1"));
      if (!leftSignBlink) digitalWrite(digitalOutputPins[0], HIGH);
    } else if (input.startsWith("SignR:")) {
      rightSignBlink = (input.endsWith("1"));
      if (!rightSignBlink) digitalWrite(digitalOutputPins[8], HIGH);
    } else if (input.startsWith("SignLR:")) {
      hazardActive = (input.endsWith("1"));
      if (!hazardActive) {
        digitalWrite(digitalOutputPins[0], HIGH);
        digitalWrite(digitalOutputPins[8], HIGH);
      }
    } else if (input.startsWith("DO")) {
      // Format: DOx:1 atau DOx:0
      int idx = input.substring(2, input.indexOf(':')).toInt() - 1;
      if (idx >= 0 && idx < 10 && idx != 0 && idx != 8) { // Kecuali SignL (0) dan SignR (8)
        bool state = input.endsWith("1");
        digitalOutputStates[idx] = state;
        digitalWrite(digitalOutputPins[idx], state ? LOW : HIGH); // Aktif LOW
      }
    }
  }
}

void updateBlinkingRelays() {
  if (millis() - lastBlinkTime >= blinkInterval) {
    lastBlinkTime = millis();
    blinkState = !blinkState;

    if (hazardActive) {
      digitalWrite(digitalOutputPins[0], blinkState ? LOW : HIGH); // SignL
      digitalWrite(digitalOutputPins[8], blinkState ? LOW : HIGH); // SignR
    } else {
      if (leftSignBlink) {
        digitalWrite(digitalOutputPins[0], blinkState ? LOW : HIGH);
      }
      if (rightSignBlink) {
        digitalWrite(digitalOutputPins[8], blinkState ? LOW : HIGH);
      }
    }
  }
}

void reportInputs() {
  bool changed = false;
  for (int i = 0; i < 19; i++) {
    int state = digitalRead(digitalInputPins[i]) == LOW ? 1 : 0;
    if (state != lastInputStates[i]) {
      Serial.print("DI");
      Serial.print(i + 1);
      Serial.print("=");
      Serial.print(state);
      Serial.print("  ");
      lastInputStates[i] = state;
      changed = true;
    }
  }
  if (changed) Serial.println();
}
