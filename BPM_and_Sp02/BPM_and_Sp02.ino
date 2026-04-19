#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

#define BUFFER_SIZE 100

uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);

  if (!particleSensor.begin(Wire)) {
    Serial.println("MAX30102 NOT FOUND");
    while (1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x2F);
  particleSensor.setPulseAmplitudeIR(0x2F);

  Serial.println("Place finger on sensor...");
}

void loop() {

  // Collect samples
  for (int i = 0; i < BUFFER_SIZE; i++) {
    while (!particleSensor.available())
      particleSensor.check();

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();

    particleSensor.nextSample();
  }

  // Calculate SpO2 + HR
  maxim_heart_rate_and_oxygen_saturation(
    irBuffer,
    BUFFER_SIZE,
    redBuffer,
    &spo2,
    &validSPO2,
    &heartRate,
    &validHeartRate
  );

  Serial.print("BPM: ");
  if (validHeartRate)
    Serial.print(heartRate);
  else
    Serial.print("Invalid");

  Serial.print(" | SpO2: ");
  if (validSPO2)
    Serial.print(spo2);
  else
    Serial.print("Invalid");

  Serial.println("%");

  delay(100);
}