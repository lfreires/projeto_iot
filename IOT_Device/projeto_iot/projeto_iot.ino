#include <Arduino.h>
#include "wifi_manager.h"
#include "rain_sensor.h"
#include "stepper_motor.h"
#include "varal_controller.h"
#include "dht11_sensor.h"
#include "mqtt_manager.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== Inicializando ESP32 ===");

  // --- Conectividade ---
  initWiFiManager();
  mqttInit();          // MQTT + AWS IoT Core

  // --- Sensores ---
  rainSensorInit();
  dht11Init();

  // --- Atuadores ---
  stepperInit();
  stepperSetSpeed(400.0f);
  stepperHome();

  // --- Regras de negócio ---
  varalControllerInit();
}

void loop() {
  // Infraestrutura
  handleWiFi();
  mqttLoop();       // mantém conexão MQTT + heartbeat

  // Sensores
  rainSensorLoop();
  dht11Loop();

  // Atuadores
  stepperLoop();

  // Lógica de negócio
  varalControllerLoop();

  // nada de delayzão :)
}
