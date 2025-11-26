#include <Arduino.h>
#include "rain_sensor.h"

// ==========================
// CONFIGURAÇÃO DE PINOS
// ==========================

// Pino ANALÓGICO ligado na saída "A0" do módulo de chuva
static const int RAIN_ANALOG_PIN  = 34;  // TODO: troque conforme sua ligação

// Pino DIGITAL ligado na saída "D0" do módulo de chuva
static const int RAIN_DIGITAL_PIN = 25;  // TODO: troque conforme sua ligação

// Intervalo entre leituras (ms)
static const unsigned long RAIN_READ_INTERVAL_MS = 1000; // 1 segundo

// ==========================
// ESTADO INTERNO
// ==========================

static unsigned long lastReadMillis = 0;

static int        lastAnalogValue   = 0;
static bool       lastDigitalValue  = false;
static RainLevel  lastLevel         = RainLevel::NONE;

// ==========================
// FUNÇÕES INTERNAS
// ==========================

// Muitos módulos de chuva funcionam assim:
// - Mais água → menor valor analógico
// Então a gente "inverte" a leitura para ficar intuitivo.
static RainLevel computeRainLevel(int analogRaw) {
  // Analógico ESP32: 0–4095
  int inverted = 4095 - analogRaw;

  // Esses thresholds são exemplos. Dá pra ajustar
  // depois vendo no Serial os valores em dia seco / chuvoso.
  if (inverted < 300) {
    return RainLevel::NONE;
  } else if (inverted < 1200) {
    return RainLevel::LIGHT;
  } else if (inverted < 2400) {
    return RainLevel::MODERATE;
  } else {
    return RainLevel::HEAVY;
  }
}

static void debugPrint() {
  Serial.print("[RAIN] Analog:");
  Serial.print(lastAnalogValue);
  Serial.print(" | Digital:");
  Serial.print(lastDigitalValue ? "CHUVA" : "SECO");
  Serial.print(" | Level: ");

  switch (lastLevel) {
    case RainLevel::NONE:     Serial.println("NONE");     break;
    case RainLevel::LIGHT:    Serial.println("LIGHT");    break;
    case RainLevel::MODERATE: Serial.println("MODERATE"); break;
    case RainLevel::HEAVY:    Serial.println("HEAVY");    break;
  }
}

// ==========================
// FUNÇÕES "PÚBLICAS"
// ==========================

void rainSensorInit() {
  pinMode(RAIN_DIGITAL_PIN, INPUT);
  // No ESP32, pinos analógicos já vêm prontos para analogRead()
  // mas se quiser tunar, pode usar analogSetAttenuation() etc.

  // Leitura inicial
  lastAnalogValue  = analogRead(RAIN_ANALOG_PIN);
  lastDigitalValue = (digitalRead(RAIN_DIGITAL_PIN) == LOW); 
  // Muitos módulos: D0 = LOW quando molhado (depende do ajuste do trimpot)

  lastLevel = computeRainLevel(lastAnalogValue);

  Serial.println("[RAIN] Sensor de chuva inicializado.");
  debugPrint();
}

void rainSensorLoop() {
  unsigned long now = millis();
  if (now - lastReadMillis < RAIN_READ_INTERVAL_MS) {
    return;
  }
  lastReadMillis = now;

  int  analogValue  = analogRead(RAIN_ANALOG_PIN);
  bool digitalValue = (digitalRead(RAIN_DIGITAL_PIN) == LOW);
  // Se no seu módulo for LOW = seco, é só inverter aqui.

  lastAnalogValue  = analogValue;
  lastDigitalValue = digitalValue;
  lastLevel        = computeRainLevel(analogValue);

  debugPrint();
}

// Getters

int rainGetAnalog() {
  return lastAnalogValue;
}

bool rainGetDigital() {
  return lastDigitalValue;
}

bool rainIsRaining() {
  // Aqui escolho "chuva" como qualquer nível diferente de NONE
  return lastLevel != RainLevel::NONE;
}

RainLevel rainGetLevel() {
  return lastLevel;
}
