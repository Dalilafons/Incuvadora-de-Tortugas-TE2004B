# 🐢 Prototipo de Incubadora Automatizada de Huevos de Tortuga

Este repositorio contiene la documentación y el desarrollo de la **Actividad 5: Integración Inicial del Prototipo y Configuración de la Red MQTT**. El proyecto forma parte de la materia "Diseño de sistemas embebidos avanzados" (Gpo 601) del Tecnológico de Monterrey.

**Fecha:** 22 de Octubre de 2025

## 🧑‍💻 Equipo de Desarrollo

* Victoria Lilian Robles Vargas - A01712297
* Santiago Isai González Arista - A01712184
* Dalila Fonseca Maya - A01711722
* Angelo Segura Ibarra - A01711723

**Profesor:** Josué González García

## 🎯 Objetivo del Proyecto

[cite_start]El objetivo principal es documentar el prototipo inicial de una incubadora automatizada diseñada para huevos de tortuga[cite: 7]. [cite_start]Este avance cubre tanto el ensamblaje físico del hardware [cite: 8] [cite_start]como la configuración de la red de comunicación MQTT para el control y monitoreo remotos[cite: 8].

## 🛠️ Componentes y Ensamblaje

[cite_start]El sistema está centralizado en un microcontrolador **ESP32** que se conecta con los siguientes componentes[cite: 13, 16, 20, 23, 26, 28]:

* [cite_start]**Motor DC:** Controlado a través de un *driver* L293D[cite: 12, 13].
* [cite_start]**Servomotores (x2):** Conectados directamente a pines del ESP32 para movimientos de precisión[cite: 22, 23].
* [cite_start]**Módulos de Sensor IR (x2):** Utilizados para detección, conectados a pines digitales[cite: 15, 16].
* **Sensores de Temperatura:**
    * [cite_start]1x Sensor DHT11[cite: 25, 26].
    * [cite_start]1x Sonda DS18B20[cite: 27, 28].
* [cite_start]**Relé (Relay):** Conectado a una fuente de 32V [cite: 19] [cite_start]y controlado por el ESP32 [cite: 20] para gestionar la...
* [cite_start]**Luz de 50W:** Conectada al pin común del relé[cite: 30].

[cite_start]Puedes consultar el diagrama de conexión completo en el documento (Página 4)[cite: 38].

## ☁️ Arquitectura de Red MQTT

[cite_start]El prototipo utiliza el protocolo MQTT para establecer una comunicación bidireccional entre el hardware (ESP32) y una interfaz de usuario (Node-RED)[cite: 52].

* [cite_start]**Broker:** Se utiliza **Mosquitto** como broker MQTT, corriendo de forma local (`localhost`) en una laptop[cite: 53].
* [cite_start]**Hardware (Cliente MQTT):** El **ESP32** se conecta a la misma red WiFi y al broker[cite: 84].
    * [cite_start]**Publica:** Los datos del sensor de temperatura[cite: 55, 87].
    * [cite_start]**Suscribe:** A los tópicos de control para recibir órdenes[cite: 55, 85].
* [cite_start]**Interfaz de Usuario (Cliente MQTT):** **Node-RED** sirve como panel de control[cite: 54].
    * [cite_start]**Publica:** Comandos para activar los actuadores (motores, led)[cite: 54].
    * [cite_start]**Suscribe:** A los tópicos de telemetría para mostrar los datos, como la temperatura en un medidor *gauge*[cite: 54, 65].

### Tópicos MQTT

#### Tópicos de Control (Node-RED → ESP32)

* [cite_start]`led`: Envía mensajes `ON` y `OFF`[cite: 58].
* [cite_start]`Servo1`: Controla el primer servomotor (brazo robótico) con mensajes `SERVO 1_L` y `SERVO 1_R`[cite: 59].
* [cite_start]`Servo2-UP`: Mueve el segundo servomotor hacia arriba[cite: 60].
* [cite_start]`Servo2-DOWN`: Mueve el segundo servomotor hacia abajo[cite: 61].
* [cite_start]`MotorA`: Activa el motor DC hacia adelante (`MOTOR_ADELANTE`)[cite: 62].
* [cite_start]`MotorR`: Activa el motor DC en reversa (`MOTOR_REVERSA`)[cite: 63].

#### Tópicos de Telemetría (ESP32 → Node-RED)

* [cite_start]`encuvadora/temperatura`: El ESP32 lee el sensor cada 5 segundos y publica el valor en este tópico[cite: 65, 87].

## 📊 Dashboard (Node-RED)

[cite_start]Se creó un dashboard en Node-RED llamado "Encuvadora" [cite: 89] que presenta la siguiente interfaz:

* [cite_start]**Sensores:** Un medidor *gauge* para la "Temperatura" [cite: 92] [cite_start]y botones "ON"/"OFF"[cite: 100, 101].
* [cite_start]**SERVOS:** Botones para controlar ambos servomotores (SERVO1 L/R, SERVO2 UP/DOWN)[cite: 91, 93, 95, 97, 99].
* [cite_start]**MOTOR DC:** Botones para "MOTOR ADELANTE" y "MOTOR REVERSA"[cite: 94, 96, 98].

## 🚀 Planes de Integración Futura

[cite_start]El prototipo actual es la base para las siguientes mejoras planificadas[cite: 103]:

1.  [cite_start]**Riel Lineal:** Implementar un riel sobre el cual se desplazará el motor DC, usando el sensor IR para posicionarse frente a cada huevo[cite: 104].
2.  [cite_start]**Estaciones Fijas:** Reemplazar las bases de cartón por estaciones de incubación fijas, cada una con su propia luz de ovoscopia en la parte inferior[cite: 105].
3.  [cite_start]**Mano Robótica:** El módulo que se moverá en el riel será una mano robótica[cite: 106].
4.  [cite_start]**Visión Artificial:** Se integrará una **ESP32-CAM** para capturar imágenes del interior de los huevos, aprovechando la luz de ovoscopia inferior[cite: 107].
5.  [cite_start]**Medición de Contacto:** La sonda DS18B20 se usará para medir la temperatura superficial de cada huevo al hacer contacto[cite: 108].
6.  [cite_start]**Inspección 180°:** El servomotor de la mano robótica le permitirá girar 180° para inspeccionar filas de huevos a ambos lados del riel[cite: 109].
