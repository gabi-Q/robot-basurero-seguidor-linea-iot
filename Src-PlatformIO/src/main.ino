// LIBRERÍAS
// =================================================================================
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <FirebaseESP32.h>
#include <VL53L0X.h>
#include <time.h>

// =================================================================================
// CONFIGURACIÓN DE RED Y FIREBASE
// =================================================================================
#define WIFI_SSID "H"
#define WIFI_PASSWORD "123456789"
#define DATABASE_URL "basurerointeligente-1d84d-default-rtdb.firebaseio.com"
#define DATABASE_SECRET "J06HLQqMk4ARbEhos98DC1Iv5nkt4F4dE1cTvVbG"

// =================================================================================
// PINES DE HARDWARE
// =================================================================================
// --- Sensores Tacho ---
#define TRIGGER_PIN 19
#define ECHO_PIN 18
#define I2C_SDA_PIN_VL53L0X 21 // Bus I2C para el sensor de distancia
#define I2C_SCL_PIN_VL53L0X 22

// --- Giroscopio (MPU6500) ---
#define SDA_PIN_MPU 4 // Bus I2C para el giroscopio
#define SCL_PIN_MPU 16
#define MPU_ADDR 0x68

// --- Actuadores Tacho (Servo) ---
#define SERVO_PIN 17
