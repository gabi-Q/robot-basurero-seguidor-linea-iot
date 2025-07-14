#include <Arduino.h>

#include <ESP32Servo.h>

#define servo_PIN 16
Servo myservo;

void setup() {
    myservo.attach(servo_PIN, 500, 2500);  // Rango PWM seguro para servos
    myservo.write(80);                     // Fijar en 90 grados
}

void loop() {
    // Nada aquí: el servo se queda fijo en 80°
}