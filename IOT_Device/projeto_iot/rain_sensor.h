#pragma once

// Níveis "qualitativos" de chuva
enum class RainLevel {
  NONE,
  LIGHT,
  MODERATE,
  HEAVY
};

// Inicializa pinos do sensor de chuva
void rainSensorInit();

// Deve ser chamada periodicamente no loop()
// Faz a leitura (com intervalo) e atualiza o estado interno
void rainSensorLoop();

// Últimos valores lidos (para quem quiser usar)
int  rainGetAnalog();     // 0–4095 no ESP32
bool rainGetDigital();    // true = chuva detectada? (depende do módulo)

// Interpretação em nível qualitativo
bool      rainIsRaining();
RainLevel rainGetLevel();
