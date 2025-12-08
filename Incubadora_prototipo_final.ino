#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <DHT.h>
#include <WebServer.h> 
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>

const char* ssid = "Angelo ";
const char* password = "87654321";
const char* mqtt_server = "172.20.10.2";

WiFiClient espClient;
PubSubClient client(espClient);

const char* cam_ip = "http://172.20.10.3"; 
const char* CAM_CAPTURE_PATH = "/capture";
const char* scriptUrl = "https://script.google.com/macros/s/AKfycbwN6kYdEEObJ2acrY_j9liHxqL_TcWlKVoknVCKA3ve8lmJhW1CJyc049nIV6mLo3qKbA/exec";

WebServer server(80); 
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

const int DHTPIN = 23;
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

float tempAmbGlobal = 0.0;
float humGlobal = 0.0;
unsigned long lastDHT = 0;
const unsigned long intervalDHT = 60000;  

int seccion = 1;

//========= SERVOS ===================

//========= SERVO1 ===================

Servo servo1;
const int servoPin = 27;
const char* topicServo = "Servo1";
int servoAngle = 90;

//========= SERVO2 ===================

Servo servo2;
const int servoPin2 = 14;
const char* topicServo2 = "Servo2";
int servoAngle2 = 50;

//========= SERVO3 ===================

Servo servo3;
const int servoPin3 = 12;
const char* topicServo3 = "Servo3";
int servoAngle3 = 20;

const int motorPinA = 25; 
const char* topicMotorA = "MotorA";
const int motorPinR = 26; 
const char* topicMotorR = "MotorR";
const char* topicMotorP = "stop";


const int LED_PIN_SECTOR2 = 32; 
const char* topicLed = "led"; 
const int LED_PIN_SECTOR3 = 33; 
const char* topicLed2 = "led2";

// ======= TÓPICOS PARA ENVIAR DATOS =======
const char* topicTemp = "encuvadora/temperatura"; 
const char* topicHum  = "encuvadora/humedad";
const char* topicMlxObj = "encuvadora/mlx_objeto";


#define PROXIMITY_PIN1 35  
#define PROXIMITY_PIN2 18  
#define PROXIMITY_PIN3 19  
 
int sensorValue1 = HIGH;
int sensorValue2 = HIGH;
int sensorValue3 = HIGH;

int sectorActual = 1;
int estado = 0;
bool cicloTerminadoParaWeb = false; 

// --- VARIABLES PARA MANUAL ---
int objetivoManual = 0;         
String datosManualesListos = ""; 
bool manualTerminado = false;   

unsigned long lastMsg = 0; 
#define MSG_BUFFER_SIZE  (50) 
char msg[MSG_BUFFER_SIZE];

// ==========================================
//      FUNCIONES DE HARDWARE
// ==========================================
void encenderServos() {
  if (!servo1.attached()) servo1.attach(servoPin);
  if (!servo2.attached()) servo2.attach(servoPin2);
  if (!servo3.attached()) servo3.attach(servoPin3);
}
void apagarServos() {
  if (servo1.attached()) servo1.detach();
  if (servo2.attached()) servo2.detach();
  if (servo3.attached()) servo3.detach();
}

void adelante(){ 
  digitalWrite(motorPinR, LOW); 
  digitalWrite(motorPinA, HIGH); 
}
void atras(){ 
  digitalWrite(motorPinR, HIGH); 
  digitalWrite(motorPinA, LOW); 
}
void detenerMotor(){ 
  digitalWrite(motorPinR, LOW); 
  digitalWrite(motorPinA, LOW); 
}

void leerSector(){
  sensorValue1 = digitalRead(PROXIMITY_PIN1);
  sensorValue2 = digitalRead(PROXIMITY_PIN2);
  sensorValue3 = digitalRead(PROXIMITY_PIN3);
  if(sensorValue1 == LOW) sectorActual = 1;
  else if(sensorValue2 == LOW) sectorActual = 2;
  else if(sensorValue3 == LOW) sectorActual = 3;
  seccion = sectorActual;
}

// * AQUÍ ESTABA EL ERROR: QUITAMOS EL HANDLECLIENT *
void moverASector(int objetivo) {
  leerSector();
  while(sectorActual != objetivo){
    leerSector();
    if(sectorActual < objetivo) adelante();
    if(sectorActual > objetivo) atras();
    delay(50); 
    // ¡YA NO HAY handleClient() AQUÍ! 
    // El robot es sordo mientras se mueve para no chocar.
  }
  detenerMotor();
}

// ==========================================
//      FOTOS Y SHEETS
// ==========================================
void tomarFoto(){
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sin WiFi, no se puede llamar a la cámara.");
    return;
  }

  HTTPClient http;

  String url = String(cam_ip) + String(CAM_CAPTURE_PATH) + "?seccion=" + String(seccion);
  Serial.print("Llamando a cámara: ");
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.print("Respuesta cámara: ");
    Serial.println(httpCode);
    Serial.println(http.getString());
  } else {
    Serial.print("Error al llamar cámara: ");
    Serial.println(httpCode);
  }

  http.end();
  delay(300);
}

void enviarTemperaturaAGoogleSheets(double tempC) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, no se envía temp");
    return;
  }

  HTTPClient http;
  http.begin(scriptUrl);
  http.addHeader("Content-Type", "application/json");

  String jsonPayload = "{";
  jsonPayload += "\"tempC\":" + String(tempC, 2) + ",";
  jsonPayload += "\"seccion\":" + String(seccion);
  jsonPayload += "}";

  Serial.print("Enviando JSON temp: ");
  Serial.println(jsonPayload);

  int httpCode = http.POST(jsonPayload);

  if (httpCode > 0) {
    Serial.print("OK POST temp ");
    Serial.println(httpCode);
    Serial.println(http.getString());
  } else {
    Serial.print("Error POST temp ");
    Serial.println(httpCode);
  }
  http.end();
}

void enviarAmbienteAGoogleSheets(float tempAmb, float hum) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure();  
  HTTPClient http;
  http.setTimeout(8000);
  http.begin(client, scriptUrl);
  http.addHeader("Content-Type", "application/json");
  String jsonPayload = String("{\"tempAmb\":") + String(tempAmb, 2) + ",\"hum\":" + String(hum, 2) + "}";
  http.POST(jsonPayload);
  http.end();
}

float revisarHuevo(int ang1, int ang2, int ang3) {
  encenderServos();
  servo1.write(ang1); delay(200);
  servo3.write(ang3); delay(200);
  servo2.write(ang2); delay(300);

  double temp = mlx.readObjectTempC();
  enviarTemperaturaAGoogleSheets(temp);
  tomarFoto();
  delay(3500);
  return (float)temp;
}

// ==========================================
//      RUTAS WEB (HANDLERS)
// ==========================================
void addCorsHeader() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
}

// -- RUTAS MANUALES: Solo dicen "OK" y activan la bandera --
void handleSeccion1() {
  addCorsHeader();
  objetivoManual = 1; manualTerminado = false;
  server.send(200, "text/plain", "OK");
}
void handleSeccion2() {
  addCorsHeader();
  objetivoManual = 2; manualTerminado = false;
  server.send(200, "text/plain", "OK");
}
void handleSeccion3() {
  addCorsHeader();
  objetivoManual = 3; manualTerminado = false;
  server.send(200, "text/plain", "OK");
}

void handlePollManual() {
  addCorsHeader();
  if (manualTerminado) {
    server.send(200, "text/plain", datosManualesListos);
  } else {
    server.send(200, "text/plain", "ESPERA");
  }
}

void handleIniciar() {
  addCorsHeader();
  estado = 1; cicloTerminadoParaWeb = false; 
  server.send(200, "text/plain", "Iniciando");
}
void handleEstado() 
{
  addCorsHeader();
  if (cicloTerminadoParaWeb) {
    server.send(200, "text/plain", "FIN"); // Avisamos que ya acabó todo
  } else {
    // Enviamos el número de estado actual (1, 2, 3 o 4)
    server.send(200, "text/plain", String(estado)); 
  }
}

void handleAmbiente() {
  addCorsHeader();
  server.send(200, "text/plain", String(tempAmbGlobal) + "," + String(humGlobal));
}
void handleNotFound() {
  addCorsHeader();
  if (server.method() == HTTP_OPTIONS) server.send(200); else server.send(404, "text/plain", "Not Found");
}

// ==========================================
//      LÓGICA DE MOVIMIENTO MANUAL
// ==========================================
void procesarManual() {
  // Cuando entramos aquí, EL ROBOT ES SORDO AL WEB
  int destino = objetivoManual;
  objetivoManual = 0; 

  if (destino == 1) {
    moverASector(1);
    delay(300);
    float t2 = revisarHuevo(5, 50, 25);
    servo1.write(90);
    delay(800);
    float t1 = revisarHuevo(165, 60, 0);
    servo1.write(90);
    delay(300);
    servo2.write(50);
    delay(300);
    servo3.write(20);
    delay(300);
    apagarServos();
    datosManualesListos = String(t1, 1) + "," + String(t2, 1);
    estado = 4;
  }
  else if (destino == 2) {
    moverASector(2); 
    delay(300); 
    digitalWrite(LED_PIN_SECTOR2, HIGH);
    float t4 = revisarHuevo(0, 45, 20); 
    float t6 = revisarHuevo(35, 30, 50);
    servo1.write(90); 
    delay(200); 
    servo2.write(50); 
    delay(200); 
    servo3.write(20); 
    delay(800);
    float t3 = revisarHuevo(170, 60, 0); 
    float t5 = revisarHuevo(140, 35, 35);
    servo1.write(90); 
    delay(300); 
    servo2.write(50); 
    delay(300); 
    servo3.write(20); 
    delay(300);
    apagarServos(); 
    digitalWrite(LED_PIN_SECTOR2, LOW);
    datosManualesListos = String(t3, 1) + "," + String(t4, 1) + "," + String(t5, 1) + "," + String(t6, 1);
    estado = 4;
  }
  else if (destino == 3) {
    moverASector(3);
    delay(300);
    digitalWrite(LED_PIN_SECTOR3, HIGH);
    float t8 = revisarHuevo(0, 50, 20);
    float t10 = revisarHuevo(35, 30, 40);
    servo1.write(90); 
    delay(200);
    servo2.write(50);
    delay(200);
    servo3.write(20);
    delay(800);
    float t7 = revisarHuevo(180, 60, 0);
    float t9 = revisarHuevo(140, 30, 30);
    servo1.write(90);
    delay(300);
    servo2.write(50);
    delay(300);
    servo3.write(20);
    delay(300);
    apagarServos();
    digitalWrite(LED_PIN_SECTOR3, LOW);
    datosManualesListos = String(t7, 1) + "," + String(t8, 1) + "," + String(t9, 1) + "," + String(t10, 1);
    estado = 4;
  }

  manualTerminado = true; 
}

// ==========================================
//      MODO AUTOMÁTICO
// ==========================================
void modoAutomatico() {
  leerSector();
  switch (estado)
  {
    case 1:
      if (sectorActual > 1) atras(); if (sectorActual < 1) adelante();
      if(sectorActual == 1){
        detenerMotor();
        delay(300);
        revisarHuevo(5, 50, 25);
        servo1.write(90);
        delay(800);
        revisarHuevo(165, 60, 0);
        servo1.write(90);
        delay(300);
        servo2.write(50);
        delay(300);
        servo3.write(20);
        delay(300);
        apagarServos();
        delay(5000);
        estado = 2;
      }
      break;
    case 2:
      if (sectorActual > 2) atras(); if (sectorActual < 2) adelante();
      if(sectorActual == 2){
        detenerMotor();
        delay(300);
        digitalWrite(LED_PIN_SECTOR2, HIGH);
        revisarHuevo(0, 45, 20);
        revisarHuevo(35, 30, 50);
        servo1.write(90);
        delay(200);
        servo2.write(50);
        delay(200);
        servo3.write(20);
        delay(800);
        revisarHuevo(170, 60, 0);
        revisarHuevo(140, 35, 35);
        servo1.write(90);
        delay(300);
        servo2.write(50);
        delay(300);
        servo3.write(20);
        delay(300);
        apagarServos();
        digitalWrite(LED_PIN_SECTOR2, LOW);
        delay(5000);
        estado = 3;
      }
      break;
    case 3:
      if (sectorActual > 3) atras(); if (sectorActual < 3) adelante();
      if(sectorActual == 3){
        detenerMotor();
        delay(300);
        digitalWrite(LED_PIN_SECTOR3, HIGH);
        revisarHuevo(0, 50, 20);
        revisarHuevo(35, 30, 40);
        servo1.write(90);
        delay(200);
        servo2.write(50);
        delay(200);
        servo3.write(20);
        delay(800);
        revisarHuevo(180, 60, 0);
        revisarHuevo(140, 30, 30);
        servo1.write(90);
        delay(300);
        servo2.write(50);
        delay(300);
        servo3.write(20);
        delay(300);
        apagarServos();
        digitalWrite(LED_PIN_SECTOR3, LOW);
        delay(5000); estado = 4;
      }
      break;
    case 4:
      if (sectorActual > 1) atras(); 
      if (sectorActual < 1) adelante();
      if(sectorActual == 1){
        detenerMotor();
        delay(200);
        servo1.write(90);
        delay(200);
        servo2.write(50);
        delay(200);
        servo3.write(20); 
        delay(1000); 
        estado = 0; 
        cicloTerminadoParaWeb = true; 
      }
      break;
  }
}

void modoSemiAutomatico(int objetivo){
  while(sectorActual != objetivo){
    leerSector();
    if(sectorActual < objetivo) adelante();
    if(sectorActual > objetivo) atras();
    delay(80); 
    // Aquí puedes dejar el handleClient si quieres poder cancelar,
    // pero si falla el movimiento, quítalo.
    server.handleClient(); 
  }
  detenerMotor(); delay(200);
  if(objetivo==1) { revisarHuevo(5,50,25); servo1.write(90); delay(800); revisarHuevo(165,60,0); }
  apagarServos(); estado = 4; 
}

// ... (Todo el código anterior sigue igual) ...

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\nIP: "); 
  Serial.println(WiFi.localIP()); 
  WiFi.setSleep(false);
   
  server.on("/iniciar", handleIniciar);
  server.on("/estado", handleEstado);
  server.on("/ambiente", handleAmbiente);
  server.on("/seccion1", handleSeccion1); 
  server.on("/seccion2", handleSeccion2); 
  server.on("/seccion3", handleSeccion3); 
  server.on("/pollManual", handlePollManual);
  server.onNotFound(handleNotFound);
  server.begin(); 
   
  pinMode(LED_PIN_SECTOR2, OUTPUT);
  pinMode(LED_PIN_SECTOR3, OUTPUT);
  pinMode(motorPinA, OUTPUT);
  pinMode(motorPinR, OUTPUT);
  pinMode(PROXIMITY_PIN1, INPUT);
  pinMode(PROXIMITY_PIN2, INPUT);
  pinMode(PROXIMITY_PIN3, INPUT);
  servo1.write(servoAngle); 
  servo2.write(servoAngle2);
  servo3.write(servoAngle3); 

  Wire.begin(21, 22);
  dht.begin();
  if (!mlx.begin()) { 
    Serial.println("Error MLX"); 
  } else {
    Serial.println("MLX iniciado correctamente.");
  }
  
                      
  
  // LEEMOS INMEDIATAMENTE AL INICIAR (Para no esperar 1 minuto)
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    tempAmbGlobal = t;
    humGlobal = h;
    Serial.println("DHT11: Lectura inicial lista.");
  }
  // ---------------------------
  
  detenerMotor();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  /* ==================== MQTT ===========================
==========================================================
==========================================================
==========================================================
*/
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  encenderServos();
  // --- Sección para ENVIAR temperatura ---
  unsigned long now = millis();
  // Envía datos solo cada 5 segundos (5000 ms)
  if (now - lastMsg > 5000) {
    lastMsg = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    float mlxObj = mlx.readObjectTempC();

    // Validar lectura DHT
    if (isnan(t) || isnan(h)) {
      Serial.println("Error al leer del sensor DHT11");
    } else {
      // Enviar Temperatura DHT
      snprintf(msg, MSG_BUFFER_SIZE, "%.1f", t); 
      client.publish(topicTemp, msg);
      Serial.print("Temp Aire: "); Serial.print(msg);
      
      // Enviar Humedad DHT
      snprintf(msg, MSG_BUFFER_SIZE, "%.1f", h);
      client.publish(topicHum, msg); 
      Serial.print(" | Humedad: "); Serial.println(msg);
    }

    // Validar y Enviar MLX
    if (mlxObj > 1000 || mlxObj < -100) { // Verificación simple de error
       Serial.println("Error lectura MLX");
    } else {
       snprintf(msg, MSG_BUFFER_SIZE, "%.2f", mlxObj);
       client.publish(topicMlxObj, msg);
       Serial.print("Temp Objeto (MLX): "); Serial.println(msg);
    }
  }

/*

======================== MQTT FINAL =========================

*/

  // ATENCIÓN WEB
  if (estado == 0 && objetivoManual == 0) {
    server.handleClient(); 
  }

  if (objetivoManual > 0) {
    procesarManual(); 
  }

  if (estado >= 1 && estado <= 4) {
    modoAutomatico();
  }

  // --- LECTURA AMBIENTAL CADA MINUTO ---
  if (millis() - lastDHT >= intervalDHT) {
    lastDHT = millis();
    float t = dht.readTemperature();  
    float h = dht.readHumidity();     
    
    // Solo actualizamos si la lectura es válida
    if (!isnan(t) && !isnan(h)) {
      tempAmbGlobal = t; 
      humGlobal = h;
      enviarAmbienteAGoogleSheets(t, h);
      Serial.println("DHT11: Actualizado (1 min)");
    } 
  }
  // -------------------------------------

  leerSector();
  if (Serial.available() > 0) {
    char c = Serial.read(); 
    if (c == 'A' && estado == 0) { estado = 1; cicloTerminadoParaWeb = false; }
    if (c == 'B' && estado == 0) { estado = 10; modoSemiAutomatico(2); }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Mensaje recibido en ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  // Lógica de los REFLECTORES
  if (String(topic) == topicLed) {
    if (message == "ON") {
      digitalWrite(LED_PIN_SECTOR2, HIGH);
      Serial.println(" LED ENCENDIDO");
    } 
    else if (message == "OFF") {
      digitalWrite(LED_PIN_SECTOR2, LOW);
      Serial.println(" LED APAGADO");
    }
  }
  if (String(topic) == topicLed2) {
    if (message == "ON") {
      digitalWrite(LED_PIN_SECTOR3, HIGH);
      Serial.println(" LED 2 ENCENDIDO");
    } 
    else if (message == "OFF") {
      digitalWrite(LED_PIN_SECTOR3, LOW);
      Serial.println(" LED  2 APAGADO");
      }
    }

  // Lógica de Motores
  if(String(topic) == topicMotorA){
    if (message == "adelante"){
      digitalWrite(motorPinA, HIGH);
      digitalWrite(motorPinR, LOW);
      Serial.println(" Motor hacia adelante ");
    }
  }

  if(String(topic) == topicMotorR){
    if (message == "reversa"){
      digitalWrite(motorPinR, HIGH);
      digitalWrite(motorPinA, LOW);
      Serial.println(" Motor hacia atrás "); 
    }
  }
  if(String(topic) == topicMotorP){
    if (message == "stop"){
      digitalWrite(motorPinR, LOW);
      digitalWrite(motorPinA, LOW);
      Serial.println(" Motor detenido"); 
    }
  }
  
  // Lógica del Servos 
  
  if (String(topic) == topicServo) {
  if (message == "UP") {
    servoAngle += 10; // gira 10 grados
  } else if (message == "DOWN") {
    servoAngle -= 10; // gira 10 grados
  }
  servoAngle = constrain(servoAngle, 0, 180);
  servo1.write(servoAngle);

  Serial.print(" Servo 1 girado a ");
  Serial.println(servoAngle);
}
  
  //logica del servo2
  if (String(topic) == topicServo2) {
  if (message == "UP") {
    servoAngle2 += 10; // sube 10 grados
  } else if (message == "DOWN") {
    servoAngle2 -= 10; // baja 10 grados
  }
  servoAngle2 = constrain(servoAngle2, 0, 180);
  servo2.write(servoAngle2);

  Serial.print(" Servo2 movido a ");
  Serial.println(servoAngle2);
}
  //logica del servo3
  if (String(topic) == topicServo3) {
  if (message == "UP") {
    servoAngle3 += 10; // sube 10 grados
  } else if (message == "DOWN") {
    servoAngle3 -= 10; // baja 10 grados
  }
  servoAngle3 = constrain(servoAngle3, 0, 180);
  servo3.write(servoAngle3);

  Serial.print(" Servo 3 movido a ");
  Serial.println(servoAngle3);
}


}

// ======= FUNCIÓN: RECONEXIÓN MQTT =======
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conectar al broker MQTT...");
    if (client.connect("ESP32_Sensor")) { 
      Serial.println(" Conectado al broker MQTT");
      
      // Suscribirse a AMBOS tópicos
      client.subscribe(topicLed);
      client.subscribe(topicLed2);  
      client.subscribe(topicServo);
      client.subscribe(topicServo2);
      client.subscribe(topicServo3);
      client.subscribe(topicMotorA);
      client.subscribe(topicMotorR);
      client.subscribe(topicMotorP);
      
      Serial.print("Suscrito a: ");
      Serial.println(topicLed);
      Serial.print("Suscrito a: ");
      Serial.println(topicServo);
      Serial.print("Suscrito a: ");
      Serial.println(topicServo2);
      Serial.print("Suscrito a: ");
      Serial.println(topicMotorA);
      Serial.print("Suscrito a: ");
      Serial.println(topicMotorR);

      
    } else {
      Serial.print(" Error, código: ");
      Serial.print(client.state());
      Serial.println(" | Reintentando en 5s...");
      delay(5000);
    }
  }
}


