#include "esp_camera.h"
#include "base64.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>

// =======================
// Configuración de pines (AI-Thinker)
// =======================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// =======================
// Credenciales Wi-Fi
// =======================
const char *ssid     = "Angelo ";
const char *password = "87654321";

// =======================
// URL del Google Apps Script
// =======================
const char *googleAppScriptUrl =
  "https://script.google.com/macros/s/AKfycbwN6kYdEEObJ2acrY_j9liHxqL_TcWlKVoknVCKA3ve8lmJhW1CJyc049nIV6mLo3qKbA/exec";

// =======================
// Servidor HTTP local
// =======================
WebServer server(80);

// control de captura
bool shouldCapture = false;
bool capturing     = false;
int lastSeccion    = 1;   // por defecto Seccion 1

// =====================================================================
// Función para iniciar la cámara
// =====================================================================
void startCamera() {
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk      = XCLK_GPIO_NUM;
  config.pin_pclk      = PCLK_GPIO_NUM;
  config.pin_vsync     = VSYNC_GPIO_NUM;
  config.pin_href      = HREF_GPIO_NUM;
  config.pin_sscb_sda  = SIOD_GPIO_NUM;
  config.pin_sscb_scl  = SIOC_GPIO_NUM;
  config.pin_pwdn      = PWDN_GPIO_NUM;
  config.pin_reset     = RESET_GPIO_NUM;

  config.xclk_freq_hz  = 20000000;
  config.pixel_format  = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size   = FRAMESIZE_QVGA;   // 320x240
    config.jpeg_quality = 25;               // más compresión
    config.fb_count     = 2;
  } else {
    config.frame_size   = FRAMESIZE_QVGA;
    config.jpeg_quality = 30;
    config.fb_count     = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error al inicializar la cámara: %s\n", esp_err_to_name(err));
    return;
  }
  Serial.println("Cámara iniciada correctamente");
}

// =====================================================================
// Capturar imagen + convertir a Base64 + enviar por POST
// =====================================================================
void captureAndSend() {
  Serial.println("Iniciando captura y envío a Google...");

  camera_fb_t *fb = esp_camera_fb_get();  
  if (!fb) {
    Serial.println("Error al capturar la imagen");
    return;
  }

  String imageBase64 = base64::encode(fb->buf, fb->len);

  WiFiClientSecure client;
  client.setInsecure();  // Ignora certificado SSL

  HTTPClient http;
  http.setTimeout(15000);
  http.begin(client, googleAppScriptUrl);
  http.addHeader("Content-Type", "application/json");

  // JSON con imageData y seccion (desde lastSeccion)
  String jsonPayload = "{\"imageData\":\"" + imageBase64 + "\"";
  jsonPayload += ",\"seccion\":" + String(lastSeccion);
  jsonPayload += "}";

  Serial.print("JSON length: ");
  Serial.println(jsonPayload.length());

  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  } else {
    Serial.print("Error en la solicitud: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  esp_camera_fb_return(fb);
  Serial.println("Captura y envío terminados");
}

// =====================================================================
// Manejador de /capture → la ESP32 normal llamará aquí
// =====================================================================
void handleCapture() {
  Serial.println("Solicitud /capture recibida");

  if (server.hasArg("seccion")) {
    lastSeccion = server.arg("seccion").toInt();
    Serial.print("Seccion recibida: ");
    Serial.println(lastSeccion);
  } else {
    lastSeccion = 1; // por default
    Serial.println("Seccion NO recibida, usando 1");
  }

  shouldCapture = true;
  server.send(200, "text/plain", "OK, capturando");
}

// =====================================================================
// SETUP
// =====================================================================
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ¡Conectado!");
  Serial.print("IP de la cámara: ");
  Serial.println(WiFi.localIP());

  WiFi.setSleep(false);

  startCamera();

  // endpoint para ser controlada por la ESP32 normal
  server.on("/capture", handleCapture);
  server.begin();
  Serial.println("Servidor HTTP listo en /capture");
}

// =====================================================================
// LOOP
// =====================================================================
void loop() {
  server.handleClient();

  if (shouldCapture && !capturing) {
    capturing = true;
    captureAndSend();
    capturing   = false;
    shouldCapture = false;
  }
}
