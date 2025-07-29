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
          Serial.println("🗑️ ¡Contenedor lleno! Iniciando viaje a destino.");

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
        Serial.println("🏁 Detectada línea de destino. Iniciando secuencia de llegada.");
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
        Serial.println("🏠 Detectada línea de base. Iniciando secuencia final.");
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
