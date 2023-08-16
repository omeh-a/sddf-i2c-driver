// tester-mega2560.ino
// Arduino sketch for testing the I2C driver, targeting MEGA2560 boards (rev3+).
// When receiving a packet, send it back to master.
// Matt Rossouw (matthew.rossouw@unsw.edu.au)
// 08/2023

#include <Wire.h>

#define I2C_ADDR 0x08

void setup() {
  Wire.begin(I2C_ADDR);
  Wire.onReceive(callback);
  Serial.begin(9600);
  Serial.println("I2C Tester started");
}


void loop() {
  delay(50);
}

void callback() {
  int count = 0;
  Serial.println("Master has requested packet!")
  while (Wire.available()) {
    char c = Wire.read();
    Serial.print("Echoing ");
    Serial.prinln(c);
    Wire.write(c);
    count++;
  }
  Serial.println("Transaction over. Echoed %d bytes.", count);
}