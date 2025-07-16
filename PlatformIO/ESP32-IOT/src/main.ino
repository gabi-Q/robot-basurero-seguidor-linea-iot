#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <VL53L0X.h>
#include <time.h>  // 📆 Para timestamps reales

// 🔧 CONFIGURACIÓN
#define WIFI_SSID "H"
#define WIFI_PASSWORD "123456789"

#define DATABASE_URL "basurerointeligente-1d84d-default-rtdb.firebaseio.com"
#define DATABASE_SECRET "J06HLQqMk4ARbEhos98DC1Iv5nkt4F4dE1cTvVbG"

// 🔌 Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// 📏 Sensor VL53L0X
VL53L0X sensor;
const float distancia_lleno = 100.0;  // mm
const float distancia_vacio = 300.0;  // mm

// 🌐 Función para configurar hora por NTP
void configurarHoraNTP() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov"); // UTC
  Serial.print("Esperando sincronización NTP...");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {  // Esperar tiempo válido
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\n Hora sincronizada");
}

void setup() {
  Serial.begin(115200);

  // 🌐 Conectar WiFi antes de hacer cualquier otra cosa
  Serial.print("Conectando a WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n Conectado a WiFi!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  // 🕒 Configurar NTP
  configurarHoraNTP();

  // 🔌 Configurar Firebase
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096, 1024);
  Firebase.begin(&config, &auth);

  // 📏 Iniciar sensor VL53L0X
  Wire.begin(21, 22); // Pines I2C para ESP32
  Serial.println("🔄 Iniciando VL53L0X...");
  sensor.setTimeout(500);
  if (!sensor.init()) {
    Serial.println("❌ Sensor no detectado");
    while (true);
  }
  sensor.setMeasurementTimingBudget(200000);
  sensor.startContinuous();
}

void loop() {
  uint16_t distancia_mm = sensor.readRangeContinuousMillimeters();

  if (sensor.timeoutOccurred()) {
    Serial.println("⚠️ Timeout del sensor");
    return;
  }

  float porcentaje = (distancia_vacio - distancia_mm) * 100.0 / (distancia_vacio - distancia_lleno);
  porcentaje = constrain(porcentaje, 0, 100);

  Serial.print("📏 Distancia: ");
  Serial.print(distancia_mm);
  Serial.print(" mm | 💧 Porcentaje: ");
  Serial.print(porcentaje);
  Serial.println(" %");

  if (Firebase.ready()) {
    // Subir datos actuales
    Firebase.setInt(fbdo, "/sensor/distancia_mm", distancia_mm);
    Firebase.setFloat(fbdo, "/sensor/porcentaje", porcentaje);

    // Obtener timestamp actual
    time_t now = time(nullptr);

    // Subir historial
    FirebaseJson historial;
    historial.add("level", porcentaje);
    historial.add("distance", distancia_mm);
    historial.add("timestamp", (int)now);

    if (Firebase.pushJSON(fbdo, "/sensor/history", historial)) {
      Serial.println("✅ Historial subido con timestamp");
    } else {
      Serial.print("❌ Error al subir historial: ");
      Serial.println(fbdo.errorReason());
    }
  }

  delay(10000); // Esperar 10 segundos
}
