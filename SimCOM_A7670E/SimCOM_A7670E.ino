#include <HardwareSerial.h>

HardwareSerial sim(2); // UART2

void setup() {
  Serial.begin(115200);

  // RX, TX
  sim.begin(115200, SERIAL_8N1, 16, 17);

  delay(3000);

  Serial.println("Sending SMS...");

  sendSMS();
}

void loop() {
}

void sendSMS() {
  sim.println("AT");
  delay(1000);

  sim.println("AT+CMGF=1"); // text mode
  delay(1000);

  sim.println("AT+CMGS=\"09526152903\""); // number
  delay(1000);

  sim.print("Hello John Wenjel Anqui this is your project"); // message
  delay(500);

  sim.write(26); // CTRL+Z to send
  delay(5000);

  Serial.println("Done");
}