#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h> 
#include <Adafruit_Sensor.h> 
#include <ESP32Servo.h> // <<< CORRECCIÓN 1: Librería correcta

// ======= CONFIGURACIÓN WIFI Y MQTT =======
const char* ssid = "iPhone de Santiago";
const char* password = "Santi2004";
const char* mqtt_server = "172.20.10.10"; 

WiFiClient espClient;
PubSubClient client(espClient);

// ======= CONFIGURACIÓN LED =======<
const int ledPin = 2;       // Pin GPIO2 (LED)
const char* topicLed = "led"; // Tópico MQTT para recibir

// ======= CONFIGURACIÓN DHT11 (SENSOR) =======
#define DHTPIN 4           // Pin donde conectaste el DHT11 (GPIO 4)
#define DHTTYPE DHT11      // Tipo de sensor
DHT dht(DHTPIN, DHTTYPE);  // Inicializar el objeto sensor

//========= CONFIGURACIÓN DE SENSORES DE PROXIMIDAD =========
#define PROXIMITY_PIN1 27 // Sensor de proxmidad 1
#define PROXIMITY_PIN2 26 // Sensor de proximidad 2
int sensorValue = 0;

//========= CONFIGURACIÓN DE MOTORREDUCTOR =========
const int motorPinA = 18;
const char* topicMotorA = "MotorA";
const int motorPinR = 19;
const char* topicMotorR = "MotorR";
//========= SERVOS ===================

//========= SERVO1 ===================
Servo servo1; // El objeto se llama servo1
const int servoPin = 14;
const char* topicServo = "Servo1";
int servoAngle = 90;

//========= SERVO2 ===================
Servo servo2; // El objeto se llama servo2
const int servoPin2 = 25;
const char* topicServo2UP = "Servo2UP";
int servoAngle2 = 90;





const char* topicTemp = "encuvadora/temperatura"; // Tópico para ENVIAR la temperatura
unsigned long lastMsg = 0; 
#define MSG_BUFFER_SIZE  (50) 
char msg[MSG_BUFFER_SIZE];



// ======= FUNCIÓN: CONECTAR WIFI =======
void setup_wifi() {
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");
  Serial.print("IP asignada: ");
  Serial.println(WiFi.localIP());
}

// ======= CALLBACK: RECIBIR MENSAJE MQTT =======
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Mensaje recibido en ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  // Lógica del LED
  if (String(topic) == topicLed) {
    if (message == "ON") {
      digitalWrite(ledPin, HIGH);
      Serial.println(" LED ENCENDIDO");
    } 
    else if (message == "OFF") {
      digitalWrite(ledPin, LOW);
      Serial.println(" LED APAGADO");
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
  
  // Lógica del Servos 
  
    // Lógica del Servo1 
  if (String(topic) == topicServo) {
    int angle = message.toInt(); 
    angle = constrain(angle, 0, 180);
    servo1.write(angle); 
    servoAngle = angle;
    Serial.print(" Servo movido a ");
    Serial.println(angle);
  }
  
  //logica del servo2
  if (String(topic) == topicServo2UP) {
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

  /*if (String(topic) == topicServo2) {
    int angle2 = message.toInt();
    angle2 = constrain(angle2, 0, 180);
    servo2.write(angle2); 
    servoAngle2 = angle2;
    Serial.print(" Servo movido a ");
    Serial.println(angle2);
  }*/
}

// ======= FUNCIÓN: RECONEXIÓN MQTT =======
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conectar al broker MQTT...");
    if (client.connect("ESP32_Sensor")) { 
      Serial.println(" Conectado al broker MQTT");
      
      // Suscribirse a AMBOS tópicos
      client.subscribe(topicLed);   
      client.subscribe(topicServo);
      client.subscribe(topicServo2UP);
      client.subscribe(topicMotorA);
      client.subscribe(topicMotorR);
      
      Serial.print("Suscrito a: ");
      Serial.println(topicLed);
      Serial.print("Suscrito a: ");
      Serial.println(topicServo);
      Serial.print("Suscrito a: ");
      Serial.println(topicServo2UP);
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

// ======= SETUP =======
void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(motorPinA, OUTPUT);
  pinMode(motorPinR, OUTPUT);
  pinMode(PROXIMITY_PIN1, INPUT);
  pinMode(PROXIMITY_PIN2, INPUT);


  
  servo1.attach(servoPin); 
  servo1.write(servoAngle); 
  servo2.attach(servoPin2); 
  servo2.write(servoAngle2); 

  dht.begin(); 

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// ======= LOOP PRINCIPAL =======
void loop() {
  sensorValue = digitalRead(PROXIMITY_PIN1);
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // IMPORTANTE: Esto procesa los mensajes entrantes

  // ===== Paro de Motor ======
  if(sensorValue == LOW ){
    digitalWrite(motorPinR, LOW);
    digitalWrite(motorPinA, LOW);
    Serial.println(" Motores parados ");
    delay(250);
    digitalWrite(motorPinA, HIGH);
    delay(250);
    digitalWrite(motorPinA, LOW);
  }

  // --- Sección para ENVIAR temperatura ---
  unsigned long now = millis();
  // Envía datos solo cada 5 segundos (5000 ms)
  if (now - lastMsg > 5000) {
    lastMsg = now;

    float t = dht.readTemperature();

    if (isnan(t)) {
      Serial.println("Error al leer del sensor DHT11");
      return;
    }

    snprintf(msg, MSG_BUFFER_SIZE, "%.1f", t); 

    Serial.print("Enviando temperatura: ");
    Serial.println(msg);
    client.publish(topicTemp, msg); 
  }
}