// Determination Test - Arduino UNO R3
// LEDs: D2-D7; buttons: D8-D12,A0; active LOW with INPUT_PULLUP.
const byte N = 6;
const byte leds[N] = { 2, 3, 4, 5, 6, 7 };
const byte buttons[N] = { 8, 9, 10, 11, 12, A0 };
const char* names[N] = { "RED", "GREEN", "BLUE", "WHITE", "LEFT", "RIGHT" };
const unsigned long RESPONSE_US = 2000000UL;
const unsigned long DEBOUNCE_US = 20000UL;
const byte TOTAL = 20;
byte stable[N], rawLast[N];
unsigned long rawChanged[N];
byte trial = 0, target = 0;
unsigned long stimulusAt = 0, nextAt = 0;
enum Mode { IDLE,
            GAP,
            WAIT_RELEASE,
            ACTIVE,
            FINISHED };
Mode mode = IDLE;
String command = "";
void ledsOff() {
  for (byte i = 0; i < N; i++) digitalWrite(leds[i], LOW);
}
void resetButtons() {
  unsigned long t = micros();
  for (byte i = 0; i < N; i++) {
    stable[i] = rawLast[i] = digitalRead(buttons[i]);
    rawChanged[i] = t;
  }
}
bool allReleased() {
  for (byte i = 0; i < N; i++)
    if (stable[i] == LOW) return false;
  return true;
}
void scheduleGap() {
  ledsOff();
  mode = GAP;
  nextAt = millis() + random(800, 1501);
}
void beginTest() {
  trial = 0;
  resetButtons();
  scheduleGap();
  Serial.println(F("STATE,STARTED"));
}
void stopTest() {
  ledsOff();
  mode = IDLE;
  Serial.println(F("STATE,STOPPED"));
}
void finishTest() {
  ledsOff();
  mode = FINISHED;
  Serial.println(F("STATE,DONE"));
}
void sendResult(int response, unsigned long rt, bool omission) {
  Serial.print(F("RESULT,"));
  Serial.print(trial);
  Serial.print(',');
  Serial.print(names[target]);
  Serial.print(',');
  if (omission) Serial.print(F("NONE"));
  else Serial.print(names[response]);
  Serial.print(',');
  Serial.print(omission ? 0 : rt / 1000.0, 2);
  Serial.print(',');
  Serial.print(!omission && response == target ? 1 : 0);
  Serial.print(',');
  Serial.println(omission ? 1 : 0);
  ledsOff();
  if (trial >= TOTAL) finishTest();
  else scheduleGap();
}
void startTrial() {
  target = random(N);
  trial++;
  ledsOff();
  digitalWrite(leds[target], HIGH);
  stimulusAt = micros();
  mode = ACTIVE;
  Serial.print(F("STIM,"));
  Serial.print(trial);
  Serial.print(',');
  Serial.println(names[target]);
}
void pollInputs() {
  unsigned long now = micros();
  for (byte i = 0; i < N; i++) {
    byte raw = digitalRead(buttons[i]);
    if (raw != rawLast[i]) {
      rawLast[i] = raw;
      rawChanged[i] = now;
    }
    if (raw != stable[i] && (unsigned long)(now - rawChanged[i]) >= DEBOUNCE_US) {
      stable[i] = raw;
      if (raw == LOW && mode == ACTIVE) {
        unsigned long rt = micros() - stimulusAt;
        sendResult(i, rt, false);
      }
    }
  }
}
void processCommand(String s) {
  s.trim();
  s.toUpperCase();
  if (s == "START") beginTest();
  else if (s == "STOP") stopTest();
  else if (s == "PING") Serial.println(F("PONG,UNO"));
}
void setup() {
  Serial.begin(115200);
  for (byte i = 0; i < N; i++) {
    pinMode(leds[i], OUTPUT);
    pinMode(buttons[i], INPUT_PULLUP);
  }
  ledsOff();
  resetButtons();
  randomSeed(analogRead(A1));
  Serial.println(F("READY,UNO"));
}
void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processCommand(command);
      command = "";
    } else if (c != '\r' && command.length() < 24) command += c;
  }
  pollInputs();
  if (mode == GAP && (long)(millis() - nextAt) >= 0) { mode = WAIT_RELEASE; }
  if (mode == WAIT_RELEASE && allReleased()) startTrial();
  if (mode == ACTIVE && (unsigned long)(micros() - stimulusAt) >= RESPONSE_US) sendResult(-1, 0, true);
}
