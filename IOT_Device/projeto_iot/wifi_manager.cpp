#include <Arduino.h>
#include <WiFi.h>
#include "wifi_manager.h"

// =======================
// Configurações de Wi-Fi
// =======================
static const char* WIFI_SSID     = "iPhone de Lucas";   // TODO: troque aqui
static const char* WIFI_PASSWORD = "Google123";  // TODO: troque aqui

// Intervalo para checar o Wi-Fi (em ms)
static const unsigned long WIFI_CHECK_INTERVAL = 10'000; // 10 segundos
static unsigned long lastWiFiCheck = 0;

// =======================
// Funções internas (apenas neste arquivo)
// =======================

static void printWiFiStatus() {
  Serial.print("[WiFi] Status: ");
  wl_status_t status = WiFi.status();

  switch (status) {
    case WL_CONNECTED:
      Serial.print("Conectado | IP: ");
      Serial.println(WiFi.localIP());
      break;
    case WL_IDLE_STATUS:
      Serial.println("Idle");
      break;
    case WL_DISCONNECTED:
      Serial.println("Desconectado");
      break;
    case WL_NO_SSID_AVAIL:
      Serial.println("SSID não encontrado");
      break;
    case WL_CONNECT_FAILED:
      Serial.println("Falha na conexão (senha incorreta?)");
      break;
    default:
      Serial.print("Código: ");
      Serial.println(status);
      break;
  }
}

static bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.println("\n[WiFi] Iniciando conexão...");
  Serial.print("[WiFi] SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const unsigned long WIFI_TIMEOUT = 15'000; // 15 segundos
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < WIFI_TIMEOUT) {
    Serial.print('.');
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Conectado com sucesso!");
    printWiFiStatus();
    return true;
  } else {
    Serial.println("[WiFi] Falha ao conectar (timeout)");
    printWiFiStatus();
    WiFi.disconnect(true);
    return false;
  }
}

// =======================
// Funções expostas no header
// =======================

void initWiFiManager() {
  bool ok = connectWiFi();

  if (!ok) {
    Serial.println("[WiFi] Não conectou no setup, tentará no loop()");
  }
}

void handleWiFi() {
  if (millis() - lastWiFiCheck < WIFI_CHECK_INTERVAL) {
    return;
  }
  lastWiFiCheck = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Conexão perdida, tentando reconectar...");
    connectWiFi();
  }
}

bool wifiIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}
