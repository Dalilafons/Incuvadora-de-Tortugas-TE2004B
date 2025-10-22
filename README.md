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

El objetivo principal es documentar el prototipo inicial de una incubadora automatizada diseñada para huevos de tortuga.
Este avance cubre tanto el ensamblaje físico del hardware como la configuración de la red de comunicación MQTT para el control y monitoreo remotos.

## 🛠️ Componentes y Ensamblaje

El sistema está centralizado en un microcontrolador **ESP32** que se conecta con los siguientes componentes:

* **Motor DC:** Controlado a través de un *driver* L293D.
* **Servomotores (x2):** Conectados directamente a pines del ESP32 para movimientos de precisión.
* **Módulos de Sensor IR (x2):** Utilizados para detección, conectados a pines digitales.
* **Sensores de Temperatura:**
    * Sensor DHT11.
    * Sonda DS18B20.
* **Relé (Relay):** Conectado a una fuente de 32V y controlado por el ESP32 para gestionar la...
* **Luz de 50W:** Conectada al pin común del relé.

Puedes consultar el diagrama de conexión completo en el documento (Página 4).

## ☁️ Arquitectura de Red MQTT

El prototipo utiliza el protocolo MQTT para establecer una comunicación bidireccional entre el hardware (ESP32) y una interfaz de usuario (Node-RED).

* **Broker:** Se utiliza **Mosquitto** como broker MQTT, corriendo de forma local (`localhost`) en una laptop.
* **Hardware (Cliente MQTT):** El **ESP32** se conecta a la misma red WiFi y al broker.
    * **Publica:** Los datos del sensor de temperatura.
    * **Suscribe:** A los tópicos de control para recibir órdenes.
* **Interfaz de Usuario (Cliente MQTT):** **Node-RED** sirve como panel de control.
    * **Publica:** Comandos para activar los actuadores (motores, led).
    * **Suscribe:** A los tópicos de telemetría para mostrar los datos, como la temperatura en un medidor *gauge*.

### Tópicos MQTT

#### Tópicos de Control (Node-RED → ESP32)

* `led`: Envía mensajes `ON` y `OFF`.
* `Servo1`: Controla el primer servomotor (brazo robótico) con mensajes `SERVO 1_L` y `SERVO 1_R`.
* `Servo2-UP`: Mueve el segundo servomotor hacia arriba.
* `Servo2-DOWN`: Mueve el segundo servomotor hacia abajo.
* `MotorA`: Activa el motor DC hacia adelante (`MOTOR_ADELANTE`).
* `MotorR`: Activa el motor DC en reversa (`MOTOR_REVERSA`).

#### Tópicos de Telemetría (ESP32 → Node-RED)

* `encuvadora/temperatura`: El ESP32 lee el sensor cada 5 segundos y publica el valor en este tópico.

## 📊 Dashboard (Node-RED)

Se creó un dashboard en Node-RED llamado "Encuvadora" que presenta la siguiente interfaz:

* **Sensores:** Un medidor *gauge* para la "Temperatura" y botones "ON"/"OFF".
* **SERVOS:** Botones para controlar ambos servomotores (SERVO1 L/R, SERVO2 UP/DOWN).
* **MOTOR DC:** Botones para "MOTOR ADELANTE" y "MOTOR REVERSA".

## 🚀 Planes de Integración Futura

El prototipo actual es la base para las siguientes mejoras planificadas:

1.  **Riel Lineal:** Implementar un riel sobre el cual se desplazará el motor DC, usando el sensor IR para posicionarse frente a cada huevo.
2.  **Estaciones Fijas:** Reemplazar las bases de cartón por estaciones de incubación fijas, cada una con su propia luz de ovoscopia en la parte inferior.
3.  **Mano Robótica:** El módulo que se moverá en el riel será una mano robótica.
4.  **Visión Artificial:** Se integrará una **ESP32-CAM** para capturar imágenes del interior de los huevos, aprovechando la luz de ovoscopia inferior.
5.  **Medición de Contacto:** La sonda DS18B20 se usará para medir la temperatura superficial de cada huevo al hacer contacto.
6.  **Inspección 180°:** El servomotor de la mano robótica le permitirá girar 180° para inspeccionar filas de huevos a ambos lados del riel.
