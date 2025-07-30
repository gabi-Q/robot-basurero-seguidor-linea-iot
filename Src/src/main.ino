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


// --- Sensores Carro (Seguidor de línea) ---
#define LEFT_LINE_TRACKING   32
#define CENTER_LINE_TRACKING 35
#define RIGHT_LINE_TRACKING  34

// --- Actuadores Carro (Motores) ---
#define ENA_PIN 25
#define IN1     26
#define IN2     27
#define IN3     14
#define IN4     12
#define ENB_PIN 13

// =================================================================================
// PARÁMETROS DE FUNCIONAMIENTO
// =================================================================================
// --- Servo ---
const int PWM_CHANNEL_SERVO = 0;
const int PWM_FREQ = 50;
const int PWM_RESOLUTION = 16;
const int SERVO_ABIERTO = 80;
const int SERVO_CERRADO = 5;

// --- Motores y PID ---
const int ENA_CHANNEL_MOTOR = 3;
const int ENB_CHANNEL_MOTOR = 4;
const int MAX_SPEED = 220;
const float VEL_TO_PWM_SLOPE = 2.046;
const float VEL_TO_PWM_INTERCEPT = 12.31;
float Kp = 2.0;
float Ki = 0.001;
float Kd = 6.0;
int lastError = 0;
float integral = 0;


float BASE_LINEAR_VELOCITY = 50.0;
const float DISTANCE_BETWEEN_WHEELS = 103.0;

// --- Sensores ---
const float UMBRAL_LLENO = 99.0;
const float distancia_lleno = 70.0;
const float distancia_vacio = 200.0;
const float UMBRAL_ABRIR_CM = 36.0;
const float LECTURA_INVALIDA = -1.0;
int thresholdLeft = 0, thresholdCenter = 0, thresholdRight = 0;

// --- Temporizadores y Lógica ---

unsigned long tiempoUltimaDeteccion = 0; // <-- deteccion ultima
const long PERIODO_GRACIA_CIERRE_MS = 2000; // <--(2 segundos)

// --- Sensor Laser (TACHO) ---
const float UMBRAL_SALTO_ABRUPTO = 50.0; // Umbral en mm. Si la lectura salta más que esto, se ignora.
float ultimaDistanciaValida = distancia_vacio; // Almacena la última lectura considerada buena.



// --- Temporizadores y Lógica ---
const long DURACION_VACIADO_MS = 20000;
const long INTERVALO_NIVEL_MS = 2000;
const long INTERVALO_PERSONA_MS = 200;
const long INTERVALO_FIREBASE_MS = 5000;
unsigned long tiempoAnteriorNivel = 0;
unsigned long tiempoAnteriorPersona = 0;
unsigned long tiempoAnteriorFirebase = 0;
unsigned long tiempoLlegadaADestino = 0;

// --- Variables para el Giroscopio (MPU) ---
float gyroZ_offset = 0.0;
float anguloActualZ = 0.0;
unsigned long tiempoPrevioGiro;

// =================================================================================
// VARIABLES GLOBALES DE ESTADO
// =================================================================================
enum EstadoRobot { CALIBRANDO, ESPERANDO_EN_BASE, VIAJE_A_DESTINO, LLEGADA_A_DESTINO, ESPERANDO_VACIADO, VIAJE_DE_REGRESO, LLEGADA_A_BASE };
EstadoRobot estadoActual = CALIBRANDO;

bool enMovimiento = false;
bool lleno = false;
bool isCalibrated = false;
bool isGyroCalibrated = false; 

bool tapaAbierta = false;
bool personaDetectada = false;
float porcentajeLlenado = 0;

// =================================================================================
// OBJETOS DE SOFTWARE
// =================================================================================
TwoWire I2C_MPU = TwoWire(1); //segundo objeto I2C en el bus 1
VL53L0X sensor;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
WebServer server(80);

// =================================================================================
// DECLARACIÓN DE FUNCIONES
// =================================================================================
void moverServoA(int angulo);
String estadoAString();
void conectarWiFi();
void configurarHoraNTP();
void setupFirebase();
void setupServer();
void seguirLineaPID();
bool detectaParada();
void stopMotors();
void medirNivelLlenado();
void gestionarTapa(unsigned long t);
float medirDistanciaUltrasonido();
String calibrateSensors();
void actualizarFirebase();
String calibrarGiroscopio(); // Devuelve String
void girarConIMU(float gradosObjetivo);


// =================================================================================
// ===                           SETUP                                           ===
// =================================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n Iniciando Sistema de Carro-Tacho Autónomo Unificado");

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LEFT_LINE_TRACKING, INPUT);
  pinMode(CENTER_LINE_TRACKING, INPUT);
  pinMode(RIGHT_LINE_TRACKING, INPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  ledcSetup(PWM_CHANNEL_SERVO, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(SERVO_PIN, PWM_CHANNEL_SERVO);
  ledcSetup(ENA_CHANNEL_MOTOR, 5000, 8);
  ledcAttachPin(ENA_PIN, ENA_CHANNEL_MOTOR);
  ledcSetup(ENB_CHANNEL_MOTOR, 5000, 8);
  ledcAttachPin(ENB_PIN, ENB_CHANNEL_MOTOR);

  moverServoA(SERVO_CERRADO);
  stopMotors();

  // --- Inicializar Sensores en sus buses I2C correspondientes ---
  Wire.begin(I2C_SDA_PIN_VL53L0X, I2C_SCL_PIN_VL53L0X);
  I2C_MPU.begin(SDA_PIN_MPU, SCL_PIN_MPU);

  // Inicializar MPU6500 (sin calibrar)
  I2C_MPU.beginTransmission(MPU_ADDR);
  I2C_MPU.write(0x6B); // Registro PWR_MGMT_1
  I2C_MPU.write(0);
  if (I2C_MPU.endTransmission(true) != 0) {
      Serial.println(" MPU no detectado. El sistema se detendrá.");
      while(true);
  }

  // Inicializar VL53L0X
  sensor.setTimeout(500);
  if (!sensor.init()) {
    Serial.println(" Sensor VL53L0X no detectado. El sistema se detendrá.");
    while (true);
  }
  sensor.startContinuous();
  
  conectarWiFi();
  configurarHoraNTP();
  setupFirebase();
  setupServer();
  
  Serial.println(" Sistema listo. Esperando calibración desde la página web...");
}

// =================================================================================
// ===                           LOOP                                            ===
// =================================================================================
void loop() {
  server.handleClient();
  unsigned long t = millis();

  switch (estadoActual) {
    case CALIBRANDO:
      stopMotors();
      break;

    case ESPERANDO_EN_BASE:
      gestionarTapa(t);
      if (t - tiempoAnteriorNivel >= INTERVALO_NIVEL_MS) {
        tiempoAnteriorNivel = t;
        medirNivelLlenado();
      }
      
      // MODIFICADO: Se comprueba el estado de ambas calibraciones
      if (porcentajeLlenado >= UMBRAL_LLENO) {
        if (isCalibrated && isGyroCalibrated) {
          Serial.println("¡Contenedor lleno! Iniciando viaje a destino.");

           // -- CERRAR TAPA --
          moverServoA(SERVO_CERRADO);
          tapaAbierta = false;
          // 
          lleno = true;
          enMovimiento = true;
          estadoActual = VIAJE_A_DESTINO;
        } else {
          // Este mensaje aparecerá si se intenta mover sin calibrar todo
          // Se puede ver en el monitor serie para depuración.
          if (!isCalibrated) Serial.println(" Esperando calibración de sensores de línea.");
          if (!isGyroCalibrated) Serial.println(" Esperando calibración de giroscopio.");
          delay(2000); // Pausa
        }
      }
      break;

    case VIAJE_A_DESTINO:
      seguirLineaPID();
      if (detectaParada()) {
        Serial.println("Detectada línea de destino. Iniciando secuencia de llegada.");
        stopMotors();
        enMovimiento = false;
        estadoActual = LLEGADA_A_DESTINO;
      }
      break;

    case LLEGADA_A_DESTINO:

      // -- AÑADIR ESTAS DOS LÍNEAS AL INICIO --
      Serial.println("Asegurando tapa cerrada antes de girar...");
      moverServoA(SERVO_CERRADO);
      tapaAbierta = false;
      // -------------------------------------

      Serial.println("Girando 180 grados para encarar la base...");
      delay(1000); 
      girarConIMU(180.0);
      
      Serial.println("Giro completado. Esperando vaciado...");
      tiempoLlegadaADestino = millis();
      estadoActual = ESPERANDO_VACIADO;
      break;

    case ESPERANDO_VACIADO:
      if (millis() - tiempoLlegadaADestino >= DURACION_VACIADO_MS) {
        medirNivelLlenado(); 
        if (porcentajeLlenado < UMBRAL_LLENO) {
          Serial.println(" Contenedor vaciado. Regresando a la base.");
          lleno = false;
          enMovimiento = true;
          estadoActual = VIAJE_DE_REGRESO;
        } else {
          Serial.println(" Sigue lleno. Esperando otros 20 segundos.");
          tiempoLlegadaADestino = millis(); 
        }
      }
      break;

    case VIAJE_DE_REGRESO:
      seguirLineaPID();
      if (detectaParada()) {
        Serial.println("Detectada línea de base. Iniciando secuencia final.");
        stopMotors();
        enMovimiento = false;
        estadoActual = LLEGADA_A_BASE;
      }
      break;

    case LLEGADA_A_BASE:
      Serial.println("Girando 180 grados para quedar listo para el próximo ciclo.");
      delay(1000);
      girarConIMU(180.0);
    
      Serial.println(" Ciclo completado. Robot en espera en la base.");
      estadoActual = ESPERANDO_EN_BASE;
      break;
  }
  if (t - tiempoAnteriorFirebase >= INTERVALO_FIREBASE_MS) {
    tiempoAnteriorFirebase = t;
    actualizarFirebase();
  }
}

// =================================================================================
// ===                   FUNCIONES DE MOVIMIENTO                                 ===
// =================================================================================
String calibrarGiroscopio() {
  const int num_muestras = 2000;
  float suma = 0;
  int16_t gz_raw;

  Serial.println("Iniciando calibración del giroscopio.");

  for (int i = 0; i < num_muestras; i++) {
    I2C_MPU.beginTransmission(MPU_ADDR);
    I2C_MPU.write(0x47); // Puntero a GYRO_ZOUT_H
    I2C_MPU.endTransmission(false);
    I2C_MPU.requestFrom((uint8_t)MPU_ADDR, (size_t)2, true);
    gz_raw = I2C_MPU.read() << 8 | I2C_MPU.read();
    suma += (gz_raw / 131.0); 
    delay(2);
  }
  gyroZ_offset = suma / num_muestras;
  
    // DENTRO DE calibrarGiroscopio()
  isGyroCalibrated = true; // Activamos la bandera
  Serial.println("Calibración de giroscopio completada.");

  // Si AMBOS están calibrados, inicia el modo de espera.
  if (isCalibrated && isGyroCalibrated) {
    estadoActual = ESPERANDO_EN_BASE;
    Serial.println("Ambas calibraciones completas. Entrando en modo de espera.");
  }

  String results = "<h3>Calibración de Giroscopio Completa</h3>";
  results += "<p><b>Offset Z calculado:</b> " + String(gyroZ_offset, 4) + " °/s</p>";
  return results;
}

void girarConIMU(float gradosObjetivo) {
  if (!isGyroCalibrated) {
    Serial.println("ERROR: Imposible girar. El giroscopio no está calibrado.");
    return;
  }

  Serial.println("Iniciando giro inteligente.");

  // --- Parámetros del Giro Inteligente ---
  // El robot gira al menos este ángulo antes de buscar la línea.
  const float gradosMinimos = 150.0; 
  // Si el robot gira más de este ángulo, se detiene por seguridad (incluso si no encontró la línea).
  const float gradosMaximos = 210.0; 
  const int velocidadGiro = 100;

  // --- Inicialización de variables ---
  anguloActualZ = 0.0;
  tiempoPrevioGiro = millis();
  int16_t gz_raw;
  float gyroZ_dps;

  // --- Iniciar el movimiento de giro (rotación sobre su propio eje) ---
  if (gradosObjetivo > 0) {
    // Giro a la derecha (sentido horario)
    digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  } else {
    // Giro a la izquierda (sentido anti-horario)
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  }
  ledcWrite(ENA_CHANNEL_MOTOR, velocidadGiro);
  ledcWrite(ENB_CHANNEL_MOTOR, velocidadGiro);

  // --- Bucle principal del giro ---
  // El bucle continúa mientras no hayamos alcanzado el ángulo máximo de seguridad.
  while (abs(anguloActualZ) < gradosMaximos) {
    unsigned long ahora = millis();
    float dt = (ahora - tiempoPrevioGiro) / 1000.0;
    tiempoPrevioGiro = ahora;

    // 1. Leer el giroscopio y actualizar el ángulo actual
    I2C_MPU.beginTransmission(MPU_ADDR);
    I2C_MPU.write(0x47);
    I2C_MPU.endTransmission(false);
    I2C_MPU.requestFrom((uint8_t)MPU_ADDR, (size_t)2, true);
    gz_raw = I2C_MPU.read() << 8 | I2C_MPU.read();
    gyroZ_dps = (gz_raw / 131.0) - gyroZ_offset;

    if (abs(gyroZ_dps) > 0.5) {
      anguloActualZ += gyroZ_dps * dt;
    }

    // 2. Comprobar la condición de parada (LA LÓGICA CLAVE)
    // Se empieza a buscar la línea después de haber girado los grados mínimos.
    if (abs(anguloActualZ) >= gradosMinimos) {
      // Leemos el sensor central
      int centerSensorValue = analogRead(CENTER_LINE_TRACKING);

      // Si el sensor central detecta la línea negra.
      if (centerSensorValue < thresholdCenter) {
        Serial.print("¡Línea re-adquirida en ");
        Serial.print(anguloActualZ);
        Serial.println(" grados!");
        
        stopMotors(); // Detiene los motores inmediatamente
        delay(500);   // Una pequeña pausa para estabilizar
        return;       // Salimos de la función porque hemos completado el objetivo
      }
    }
  }

  // --- Failsafe (Plan B) ---
  // Si el bucle termina, significa que alcanzamos los grados máximos sin encontrar la línea.
  stopMotors();
  Serial.print("Giro completado por alcanzar ángulo máximo (");
  Serial.print(anguloActualZ);
  Serial.println(" grados) sin encontrar la línea.");
  delay(500);
}

void seguirLineaPID() {
  if (!enMovimiento) {
    stopMotors();
    return;
  }
  
  int left = analogRead(LEFT_LINE_TRACKING);
  int center = analogRead(CENTER_LINE_TRACKING);
  int right = analogRead(RIGHT_LINE_TRACKING);

  int error = 0;
  if (left < thresholdLeft && center >= thresholdCenter && right >= thresholdRight) error = -2;
  else if (left < thresholdLeft && center < thresholdCenter && right >= thresholdRight) error = -1;
  else if (left >= thresholdLeft && center < thresholdCenter && right >= thresholdRight) error = 0;
  else if (left >= thresholdLeft && center < thresholdCenter && right < thresholdRight) error = 1;
  else if (left >= thresholdLeft && center >= thresholdCenter && right < thresholdRight) error = 2;
  else error = lastError;
  
  integral += error;
  int derivative = error - lastError;
  float omega = Kp * error + Ki * integral + Kd * derivative;
  lastError = error;

  float vL = BASE_LINEAR_VELOCITY - (DISTANCE_BETWEEN_WHEELS / 2.0) * omega;
  float vR = BASE_LINEAR_VELOCITY + (DISTANCE_BETWEEN_WHEELS / 2.0) * omega;
  
  int pwmLeft  = constrain((int)(vL * VEL_TO_PWM_SLOPE + VEL_TO_PWM_INTERCEPT), 0, MAX_SPEED);
  int pwmRight = constrain((int)(vR * VEL_TO_PWM_SLOPE + VEL_TO_PWM_INTERCEPT), 0, MAX_SPEED);

  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  ledcWrite(ENA_CHANNEL_MOTOR, pwmLeft);
  ledcWrite(ENB_CHANNEL_MOTOR, pwmRight);
  delay(10);
}

bool detectaParada() {
  return (analogRead(LEFT_LINE_TRACKING) < thresholdLeft &&
          analogRead(CENTER_LINE_TRACKING) < thresholdCenter &&
          analogRead(RIGHT_LINE_TRACKING) < thresholdRight);
}

void stopMotors() {

  integral = 0;
  lastError = 0;
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  ledcWrite(ENA_CHANNEL_MOTOR, 0);
  ledcWrite(ENB_CHANNEL_MOTOR, 0);
}

String calibrateSensors() {
  int whiteLeft = 0, whiteCenter = 0, whiteRight = 0;
  int blackLeft = 4095, blackCenter = 4095, blackRight = 4095;
  
  Serial.println("Iniciando calibración de sensores de línea...");
  for (int i = 0; i < 500; i++) {
    int l = analogRead(LEFT_LINE_TRACKING);
    int c = analogRead(CENTER_LINE_TRACKING);
    int r = analogRead(RIGHT_LINE_TRACKING);
    if (l < blackLeft) blackLeft = l;
    if (c < blackCenter) blackCenter = c;
    if (r < blackRight) blackRight = r;
    if (l > whiteLeft) whiteLeft = l;
    if (c > whiteCenter) whiteCenter = c;
    if (r > whiteRight) whiteRight = r;
    delay(10);
  }
  
  thresholdLeft   = (whiteLeft + blackLeft) / 2;
  thresholdCenter = (whiteCenter + blackCenter) / 2;
  thresholdRight  = (whiteRight + blackRight) / 2;
  
  isCalibrated = true;
  Serial.println("Calibración de sensores de línea completada.");

  // Si AMBOS están calibrados, inicia el modo de espera.
  if (isCalibrated && isGyroCalibrated) {
    estadoActual = ESPERANDO_EN_BASE;
    Serial.println("Calibraciones completas. Entrando en modo de espera.");
  }
  
  String results = "<h3>Calibración de Sensores de Línea Completo</h3>";
  results += "<p><b>Sensor Izquierdo:</b> Negro=" + String(blackLeft) + ", Blanco=" + String(whiteLeft) + " -> <b>Umbral=" + String(thresholdLeft) + "</b></p>";
  results += "<p><b>Sensor Central:</b> Negro=" + String(blackCenter) + ", Blanco=" + String(whiteCenter) + " -> <b>Umbral=" + String(thresholdCenter) + "</b></p>";
  results += "<p><b>Sensor Derecho:</b> Negro=" + String(blackRight) + ", Blanco=" + String(whiteRight) + " -> <b>Umbral=" + String(thresholdRight) + "</b></p>";
  return results;
}
// =================================================================================
// ===                   FUNCIONES DE SENSORES Y ACTUADORES                      ===
// =================================================================================
void medirNivelLlenado() {
  uint16_t distancia_mm = sensor.readRangeContinuousMillimeters();
  if (sensor.timeoutOccurred()) {
    Serial.println("Timeout del VL53L0X al medir nivel.");
    return;
  }

  porcentajeLlenado = (distancia_vacio - distancia_mm) * 100.0 / (distancia_vacio - distancia_lleno);
  porcentajeLlenado = constrain(porcentajeLlenado, 0, 100);
}

void gestionarTapa(unsigned long t) {
  if (t - tiempoAnteriorPersona >= INTERVALO_PERSONA_MS) {
    tiempoAnteriorPersona = t;

    float dist_persona = medirDistanciaUltrasonido();
    bool personaCerca = (dist_persona != LECTURA_INVALIDA && dist_persona <= UMBRAL_ABRIR_CM);
    personaDetectada = personaCerca;

    if (personaCerca && porcentajeLlenado < 100.0) {
      if (!tapaAbierta) {
        moverServoA(SERVO_ABIERTO);
        tapaAbierta = true;
      }
      // Si detectamos a alguien, reiniciamos el temporizador de gracia
      tiempoUltimaDeteccion = t;
    } 
    // MODIFICAR LA CONDICIÓN DE CIERRE
    else if (tapaAbierta && !personaCerca) {
      // Solo cierra la tapa si ha pasado el tiempo de gracia desde la última vez que vimos a alguien
      if (t - tiempoUltimaDeteccion >= PERIODO_GRACIA_CIERRE_MS) {
        moverServoA(SERVO_CERRADO);
        tapaAbierta = false;
      }
    }
  }
}

float medirDistanciaUltrasonido() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  long duracion = pulseIn(ECHO_PIN, HIGH, 25000);
  if (duracion == 0) return LECTURA_INVALIDA;
  float distancia = duracion * 0.0343 / 2.0;
  if (distancia < 5.0 || distancia > 80.0) return LECTURA_INVALIDA;
  
  return distancia;
}

void moverServoA(int angulo) {
  int minDuty = (int)(pow(2, PWM_RESOLUTION) * 0.025);
  int maxDuty = (int)(pow(2, PWM_RESOLUTION) * 0.125);
  int duty = map(angulo, 0, 180, minDuty, maxDuty);
  ledcWrite(PWM_CHANNEL_SERVO, duty);
}

// =================================================================================
// ===                   FUNCIONES DE RED Y SERVIDORES                           ===
// =================================================================================

void conectarWiFi() {
  IPAddress local_IP(192, 168, 137, 26);
  IPAddress gateway(192, 168, 137, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(8, 8, 8, 8);
  IPAddress secondaryDNS(8, 8, 4, 4);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Falló la configuración de IP estática");
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("📡 Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi conectado");
  Serial.print("IP Estática Asignada: "); Serial.println(WiFi.localIP());
}

void configurarHoraNTP() {
  configTime(-18000, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "PET5", 1);
  tzset();
}

void setupFirebase() {
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096, 1024);
  Firebase.begin(&config, &auth);
}

void actualizarFirebase() {
  if (Firebase.ready() && WiFi.status() == WL_CONNECTED) {
    FirebaseJson currentStatus;
    currentStatus.add("estado", estadoAString());
    currentStatus.add("porcentajeLlenado", porcentajeLlenado);
    currentStatus.add("tapaAbierta", tapaAbierta);
    currentStatus.add("personaDetectada", personaDetectada);
    currentStatus.add("enMovimiento", enMovimiento);
    currentStatus.add("estaLleno", lleno);
    currentStatus.add("calibrado", isCalibrated);
    currentStatus.add("giroCalibrado", isGyroCalibrated);
    currentStatus.add("timestamp", time(nullptr));

    if (!Firebase.setJSON(fbdo, "/sensor/currentStatus", currentStatus)) {
      Serial.print("Error subiendo estado a Firebase: ");
      Serial.println(fbdo.errorReason());
    }

    FirebaseJson entradaHistorial;
    time_t timestamp = time(nullptr);
    entradaHistorial.add("timestamp", timestamp);
    entradaHistorial.add("porcentajeLlenado", porcentajeLlenado);
    if (!Firebase.pushJSON(fbdo, "/sensor/historialLlenado", entradaHistorial)) {
      Serial.print("Error guardando historial de llenado: ");
      Serial.println(fbdo.errorReason());
    }
  }
}

void setupServer() {
  server.on("/status", HTTP_GET, []() {
    String json = "{";
    json += "\"estadoActual\":\"" + estadoAString() + "\",";
    json += "\"enMovimiento\":" + String(enMovimiento ? "true" : "false") + ",";
    json += "\"lleno\":" + String(lleno ? "true" : "false") + ",";
    json += "\"isCalibrated\":" + String(isCalibrated ? "true" : "false") + ",";
    json += "\"isGyroCalibrated\":" + String(isGyroCalibrated ? "true" : "false") + ",";
    json += "\"porcentajeLlenado\":" + String(porcentajeLlenado, 1) + ",";
    json += "\"tapaAbierta\":" + String(tapaAbierta ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/calibrate", HTTP_GET, []() {
    String calibrationResults = calibrateSensors();
    String html = "<html><head><title>Calibracion Completa</title>";
    html += "<style>body{font-family:sans-serif; text-align:center; padding-top: 50px; background-color:#f0f0f0;}</style></head><body>";
    html += "<div style='text-align:left; display:inline-block; margin-top:20px; padding: 20px; background-color:white; border-radius:8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1);'>" + calibrationResults + "</div>";
    html += "<br><br><a href='/' style='text-decoration:none; color:white; background-color:#007bff; padding:10px 20px; border-radius:5px;'>Volver</a>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  //Endpoint para calibrar el giroscopio
  server.on("/calibrategyro", HTTP_GET, []() {
    String calibResults = calibrarGiroscopio();
    String html = "<html><head><title>Calibracion Giroscopio</title>";
    html += "<style>body{font-family:sans-serif; text-align:center; padding-top: 50px; background-color:#f0f0f0;}</style></head><body>";
    html += "<div style='text-align:left; display:inline-block; margin-top:20px; padding: 20px; background-color:white; border-radius:8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1);'>" + calibResults + "</div>";
    html += "<br><br><a href='/' style='text-decoration:none; color:white; background-color:#007bff; padding:10px 20px; border-radius:5px;'>Volver</a>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/", HTTP_GET, []() {
    String html = "<html><head><title>Monitor Robot Autónomo</title>";
    html += "<meta http-equiv='refresh' content='5'>";
    html += "<style>body{font-family:sans-serif; text-align:center; background-color:#f0f0f0; margin:0; padding:20px;} h1{color:#333;} .grid-container{display:grid; grid-template-columns: 1fr 1fr; gap: 20px; max-width:800px; margin: 20px auto;} .container{background-color:white; text-align:left; border:1px solid #ccc; padding:20px; border-radius:10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1);} h3{color:#0056b3; border-bottom: 2px solid #eee; padding-bottom:10px; margin-top:0;} p{font-size:1.1em; margin:10px 0;} b{color:#555;} input[type=submit]{width:100%; padding:15px; font-size:1.2em; border:none; border-radius:5px; cursor:pointer;}</style>";
    html += "</head><body>";
    html += "<h1>Monitor del Carro-Tacho Autónomo</h1>";
    html += "<div class='grid-container'>";
    html += "<div class='container'>";
    html += "<h3>Estado del Sistema</h3>";
    html += "<p id='estadoRobot'><b>Estado General:</b> ...</p>";
    html += "<p id='estadoContenedor'><b>Contenedor Lleno:</b> ...</p>";
    html += "<p id='estadoCalibracion'><b>Sensores de Línea:</b> ...</p>";
    html += "<p id='estadoGiro'><b>Giroscopio:</b> ...</p>";
    html += "<p id='estadoMovimiento'><b>Movimiento:</b> ...</p>";
    html += "</div>";
    html += "<div class='container'>";
    html += "<h3>Estado de Sensores</h3>";
    html += "<p id='llenado'><b>Nivel de Llenado:</b> ...</p>";
    html += "<p id='tapa'><b>Tapa:</b> ...</p>";
    html += "</div>";
    html += "<div class='container' style='grid-column: span 2;'>";
    html += "<h3>Acciones </h3>";
    html += "<form action='/calibrate' method='get' style='margin-bottom:20px;'>";
    html += "<p><b>1. Calibrar Sensores de Línea:</b> Coloca el robot sobre la línea para que detecte blanco/negro.</p>";
    html += "<input type='submit' value='Iniciar Calibración de Línea' style='background-color:#28a745; color:white;'>";
    html += "</form>";
    html += "<hr style='border: 1px solid #eee;'>";
    html += "<form action='/calibrategyro' method='get' style='margin-top:20px;'>";
    html += "<p><b>2. Calibrar Giroscopio:</b> Coloca el robot totalmente quieto en una superficie plana.</p>";
    html += "<input type='submit' value='Iniciar Calibración de Giroscopio' style='background-color:#ffc107; color:black;'>";
    html += "</form>";
    html += "</div>";
    html += "</div>";
    html += "<script>";
    html += "setInterval(function() {";
    html += "  fetch('/status').then(response => response.json()).then(data => {";
    html += "    document.getElementById('estadoRobot').innerHTML = '<b>Estado General:</b> ' + data.estadoActual;";
    html += "    document.getElementById('estadoContenedor').innerHTML = '<b>Contenedor Lleno:</b> ' + (data.lleno ? 'Sí' : 'No');";
    html += "    document.getElementById('estadoCalibracion').innerHTML = '<b>Sensores de Línea:</b> ' + (data.isCalibrated ? 'Calibrados' : 'NO Calibrados');";
    html += "    document.getElementById('estadoGiro').innerHTML = '<b>Giroscopio:</b> ' + (data.isGyroCalibrated ? 'Calibrado' : 'NO Calibrado');";
    html += "    document.getElementById('estadoMovimiento').innerHTML = '<b>Movimiento:</b> ' + (data.enMovimiento ? 'En Curso' : 'Detenido');";
    html += "    document.getElementById('llenado').innerHTML = '<b>Nivel de Llenado:</b> ' + data.porcentajeLlenado + ' %';";
    html += "    document.getElementById('tapa').innerHTML = '<b>Tapa:</b> ' + (data.tapaAbierta ? 'Abierta' : 'Cerrada');";
    html += "  });";
    html += "}, 2000);";
    html += "</script>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.begin();
}

// =================================================================================
// ===                   FUNCIONES AUXILIARES                                    ===
// =================================================================================
String estadoAString() {
  switch (estadoActual) {
    case CALIBRANDO: return "ESPERANDO_CALIBRACION";
    case ESPERANDO_EN_BASE: return "ESPERANDO_EN_BASE";
    case VIAJE_A_DESTINO: return "VIAJE_A_DESTINO";
    case LLEGADA_A_DESTINO: return "LLEGANDO_A_DESTINO";
    case ESPERANDO_VACIADO: return "ESPERANDO_VACIADO";
    case VIAJE_DE_REGRESO: return "VIAJE_DE_REGRESO";
    case LLEGADA_A_BASE: return "LLEGANDO_A_LA_BASE";
    default: return "DESCONOCIDO";
  }
}
