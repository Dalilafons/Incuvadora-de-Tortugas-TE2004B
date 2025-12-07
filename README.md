# 🐢 Tec Robotics: Incubadora Inteligente de Tortugas 

Este repositorio contiene la documentación final y el código fuente del proyecto **"Incubadora de Tortugas"**. Desarrollado como parte de la materia "Diseño de sistemas embebidos avanzados" (Gpo 601) del Tecnológico de Monterrey, este prototipo busca mitigar los efectos del cambio climático y el saqueo furtivo en la tortuga Golfina (*Lepidochelys olivacea*).

**Estado:** Finalizado ✅
**Fecha:** Diciembre 2025

## 🧑‍💻 Equipo de Desarrollo

* Victoria Lilian Robles Vargas - A01712297
* Santiago Isai González Arista - A01712184
* Dalila Fonseca Maya - A01711722
* Angelo Segura Ibarra - A01711723

**Profesor:** Josué González García

## 🎯 Objetivo y Misión

El objetivo principal es desarrollar un sistema robótico que aumente la tasa de eclosión y supervivencia de los embriones. El sistema monitorea variables críticas (temperatura y humedad) y realiza procesos de **ovoscopia automatizada** sin contacto humano, alineándose con los **ODS 13 (Acción por el Clima)** y **ODS 14 (Vida Submarina)**.

## 🛠️ Arquitectura de Hardware

El sistema integra dos microcontroladores principales y diversos periféricos:

### Microcontroladores
* **ESP32 (Main):** Coordina los sensores, el movimiento del robot y la comunicación WiFi/MQTT.
* **ESP32-CAM:** Dedicada exclusivamente a la captura de imágenes para la ovoscopia y documentación visual del embrión.

### Actuadores y Sensores
* **Motor DC + Driver L293D:** Controla el desplazamiento longitudinal del robot sobre el riel para visitar cada sector.
* **Servomotores:** Manejan el brazo robótico (Mano) para posicionar la cámara y los sensores sobre el huevo.
* **Sensores IR:** Detectan la posición exacta del brazo en cada uno de los 3 sectores.
* **Sensor MLX90614:** Mide la temperatura del huevo **sin contacto**.
* **Sensor DHT11:** Monitorea la temperatura y humedad ambiental dentro de la incubadora.
* **Iluminación:** Lámpara LED controlada por relé para realizar la ovoscopia (iluminación del huevo desde abajo).

## ☁️ Software e Interfaces de Usuario

El proyecto implementa una arquitectura IoT híbrida para control y monitoreo:

### 1. Interfaz Web (Usuario Final)
Página web alojada directamente en el ESP32.
* **Panel de Control:** Visualización en tiempo real de la temperatura de cada huevo (Huevo 3, 4, 5, etc.).
* **Automatización:** Permite programar la hora de inicio del ciclo automático.
* **Manual:** Permite elegir la selección a monitorear.
* **Registros:** Tabla histórica con fechas, secciones revisadas y enlaces a las fotos.

### 2. Dashboard Node-RED (Técnico)
Utilizado para mantenimiento y control manual vía MQTT.
* **Controles:** Botones para cada servomotor y dirección del motor DC.
* **Telemetría:** Gráficos tipo *gauge* para temperatura ambiente, humedad y temperatura del objeto.

### 3. Integración en la Nube (Google)
* **Google Drive:** Almacenamiento automático de las fotografías capturadas por la ESP32-CAM.
* **Gmail:** Envío automático de reportes PDF con el estado de la incubadora.

## 🔄 Funcionamiento del Sistema

El robot opera mediante un ciclo automático de 4 fases:
1.  **Fase 1:** Desplazamiento y revisión del primer sector (Sensores + Foto).
2.  **Fase 2:** Desplazamiento al segundo sector.
3.  **Fase 3:** Desplazamiento al tercer sector.
4.  **Fase 4:** Retorno a la posición inicial (Home).

## 📊 Resultados

El sistema logró:
* Validar la telemetría en tiempo real con precisión (ej. lecturas estables de ~23°C - 24°C).
* Generar una bitácora digital accesible desde la web con enlaces directos a las evidencias fotográficas.
* Controlar la temperatura y humedad para evitar el sesgo de sexo en las crías debido a temperaturas extremas.

---
*Tecnológico de Monterrey - Ingeniería en Robótica y Sistemas Digitales*
