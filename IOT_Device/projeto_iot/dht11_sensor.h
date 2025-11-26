#pragma once

// Status da última leitura do DHT11
enum class DhtStatus {
  NOT_READ_YET,
  OK,
  ERROR_TIMEOUT,
  ERROR_CHECKSUM
};

// Inicializa o sensor DHT11
void dht11Init();

// Chamar periodicamente no loop (respeita intervalo mínimo de leitura)
void dht11Loop();

// Consulta dos últimos valores lidos
bool  dht11HasValidData();
float dht11GetTemperatureC();   // em Celsius
float dht11GetHumidity();       // em %

DhtStatus dht11GetStatus();
