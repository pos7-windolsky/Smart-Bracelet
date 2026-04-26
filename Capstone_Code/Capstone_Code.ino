#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
HardwareSerial sim(2);
MAX30105 particleSensor;

// ===== BPM =====
const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

// ===== Threshold =====
const long FINGER_THRESHOLD = 50000;

// ===== MPU =====
#define MPU_ADDR 0x68
int16_t gx, gy, gz;
int16_t prevGx = 0, prevGy = 0, prevGz = 0;

// ===== Buttons =====
const int button = 13;
const int button2 = 33;
const int bled = 12;
const int rled = 14;
const int gled = 25;

bool lastState = HIGH;
bool lastState2 = HIGH;

bool motionMode = false;
bool bpmMode = false;

// SMS cooldown
unsigned long lastBpmSMS = 0;
unsigned long lastMotionSMS = 0;

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  pinMode(bled, OUTPUT);
  pinMode(rled, OUTPUT);
  pinMode(gled, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);

  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  sim.begin(115200, SERIAL_8N1, 26, 27);

  // MAX30102 INIT AFTER I2C
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found");
    while (1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x2F);
  particleSensor.setPulseAmplitudeGreen(0);

  // MPU init
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void loop() {
  digitalWrite(gled,HIGH);
  // ===== GPS =====
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // ===== BUTTON TOGGLES =====
  bool currentState = digitalRead(button);
  bool currentState2 = digitalRead(button2);

  if (lastState == HIGH && currentState == LOW) {
    motionMode = !motionMode;
    delay(200);
  }

  if (lastState2 == HIGH && currentState2 == LOW) {
    bpmMode = !bpmMode;
    delay(200);
  }

  lastState = currentState;
  lastState2 = currentState2;

  // ===== BPM =====
  long irValue = particleSensor.getIR();

// ALWAYS check signal first
if (irValue < 20000) {
  Serial.println("No finger");
  return;
}

// BPM detection (must always run)
if (checkForBeat(irValue)) {

  long delta = millis() - lastBeat;
  lastBeat = millis();

  beatsPerMinute = 60.0 / (delta / 1000.0);

  if (beatsPerMinute > 30 && beatsPerMinute < 180) {

    rates[rateSpot++] = (byte)beatsPerMinute;
    rateSpot %= RATE_SIZE;

    int sum = 0;
    for (byte i = 0; i < RATE_SIZE; i++) sum += rates[i];
    beatAvg = sum / RATE_SIZE;

    Serial.print("BPM: ");
    Serial.println(beatAvg);
  }
}

  // ===== MPU =====
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  gx = Wire.read() << 8 | Wire.read();
  gy = Wire.read() << 8 | Wire.read();
  gz = Wire.read() << 8 | Wire.read();

  int diff = abs(gx - prevGx) + abs(gy - prevGy) + abs(gz - prevGz);

  prevGx = gx;
  prevGy = gy;
  prevGz = gz;

  // ===== MOTION MODE =====
  if (motionMode) {
    digitalWrite(bled,HIGH);
    if (diff < 400) {
      if (millis() - lastMotionSMS > 30000) {
        sendFallSMS();
        lastMotionSMS = millis();
      }
    }
  }
  else{
    digitalWrite(bled, LOW);
  }

  // ===== BPM ALERT MODE =====
  if (bpmMode) {
    digitalWrite(rled,HIGH);
    if (beatAvg < 50 || beatAvg > 120) {
      if (millis() - lastBpmSMS > 30000) {
        sendBPMSMS();
        lastBpmSMS = millis();
      }
    }
  }
  else{
    digitalWrite(rled, LOW);
  }

  Serial.print("BPM: ");
  Serial.print(beatAvg);
  Serial.print(" | Motion: ");
  Serial.println(diff);
}
void sendFallSMS() {
  String numbers[] = {"09606728341", "09512062811"};

  for (int i = 0; i < 2; i++) {

    sim.println("AT");
    delay(500);

    sim.println("AT+CMGF=1"); // text mode
    delay(500);

    sim.print("AT+CMGS=\"");
    sim.print(numbers[i]);
    sim.println("\"");
    delay(1000); // wait for '>' prompt

    sim.println("The device detected that the patient is stationary and not responding.");
  
    sim.print("Location: ");
    sim.println("9.306708, 123.299251");

    sim.print("BPM: ");
    sim.println(beatAvg);

    sim.println("SpO2: 95%");

    delay(500);

    sim.write(26); // CTRL+Z to send
    delay(5000); // wait for message to send before next number
  }

  Serial.println("Done");

}
void sendBPMSMS() {
  String numbers[] = {"09606728341", "09512062811"};

  for (int i = 0; i < 2; i++) {

    sim.println("AT");
    delay(500);

    sim.println("AT+CMGF=1"); // text mode
    delay(500);

    sim.print("AT+CMGS=\"");
    sim.print(numbers[i]);
    sim.println("\"");
    delay(1000); // wait for '>' prompt

    sim.println("This is an emergency! The patient has an irregular BPM");
    sim.print("Location: ");
    sim.println("9.306708, 123.299251");

    sim.print("BPM: ");
    sim.println(beatAvg);

    sim.println("SpO2: 95%");

    delay(500);
    sim.write(26); // CTRL+Z
    delay(5000); // wait for sending to complete
  }

  Serial.println("Done");

}
