#include <Arduino.h>
#include <DHT.h>
#include "dht11_sensor.h"

// ==================================
// CONFIGURAÇÃO DO PINO / TIPO
// ==================================

static const int DHT_PIN  = 13;
static const int DHT_TYPE = DHT11;

static DHT dht(DHT_PIN, DHT_TYPE);

// Intervalo mínimo entre leituras (ms)
// DHT11 aguenta algo na casa de 1 leitura a cada 1–2 segundos
static const unsigned long DHT_READ_INTERVAL_MS = 2000; // 2s

// Estado interno
static unsigned long lastReadMillis = 0;
static float lastTempC      = NAN;
static float lastHumidity   = NAN;
static DhtStatus lastStatus = DhtStatus::NOT_READ_YET;

// ==================================
// FUNÇÕES PÚBLICAS
// ==================================

void dht11Init() {
  dht.begin();
  lastReadMillis = 0;
  lastTempC      = NAN;
  lastHumidity   = NAN;
  lastStatus     = DhtStatus::NOT_READ_YET;

  Serial.print("[DHT11] Iniciado no pino ");
  Serial.println(DHT_PIN);
}

void dht11Loop() {
  unsigned long now = millis();
  if (now - lastReadMillis < DHT_READ_INTERVAL_MS) {
    return; // ainda não é hora de ler novamente
  }
  lastReadMillis = now;

  // Faz a leitura (a lib cuida do timing interno)
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Celsius

  if (isnan(h) || isnan(t)) {
    // Leitura falhou
    lastStatus = DhtStatus::ERROR_TIMEOUT;
    Serial.println("[DHT11] Falha na leitura (NaN)");
    return;
  }

  // Leitura OK
  lastTempC    = t;
  lastHumidity = h;
  lastStatus   = DhtStatus::OK;

  // Debug opcional
  Serial.print("[DHT11] Temp: ");
  Serial.print(lastTempC);
  Serial.print(" °C | Umid: ");
  Serial.print(lastHumidity);
  Serial.println(" %");
}

bool dht11HasValidData() {
  return lastStatus == DhtStatus::OK;
}

float dht11GetTemperatureC() {
  return lastTempC;
}

float dht11GetHumidity() {
  return lastHumidity;
}

DhtStatus dht11GetStatus() {
  return lastStatus;
}
