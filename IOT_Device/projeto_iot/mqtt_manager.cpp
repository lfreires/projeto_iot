#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "mqtt_manager.h"
#include "wifi_manager.h"
#include "dht11_sensor.h"
#include "rain_sensor.h"
#include "varal_controller.h"

// =========================================
// CONFIGURAÇÃO AWS IOT CORE / MQTT
// =========================================

static const char* AWS_IOT_ENDPOINT = "a35ipjtxc4uo0u-ats.iot.us-east-1.amazonaws.com";
static const int   AWS_IOT_PORT     = 8883;

// IDs / tópicos
static const char* MQTT_CLIENT_ID        = "esp32_iot";
static const char* MQTT_TOPIC_HEARTBEAT  = "casa/varal1/heartbeat";
static const char* MQTT_TOPIC_STATUS     = "casa/varal1/status";
static const char* MQTT_TOPIC_CMD        = "casa/varal1/cmd";

// Intervalo do heartbeat (ms)
static const unsigned long HEARTBEAT_INTERVAL_MS = 30'000; // 30 segundos
static unsigned long lastHeartbeatMillis = 0;

// =========================================
// CERTIFICADOS (PLACEHOLDER)
// =========================================

static const char AWS_ROOT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

static const char AWS_CLIENT_CERT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUWTfM7P4zxc7DR817exsJvvAGzzowDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTEyMzIwMjAx
NloXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAK1j0qQAsGcIzGl9Qm31
sG2ozdUvyiAC7ba60bRMOrgYELaUN8ph/+vlM3HhmlHm0NiJN9g5dtoRzMdvBdxg
JUqqFVWOX4qyZq8HTxcOyRxK9lXjLyyGP54e+/LtxchVWPQH1Vudr7aUPW2V/ddB
R7UUarXcE0V8a2rwq3gBJPSSHk5iCd1NFJu7vze372eS2i2Z6r8Nz6ckI6d/eW9m
+DN/wxgMFoENVZ45FP+d0wv86OIxdHCikgyHHc3TYIu6B6WdKZI0MgRx/L9A1awH
RVHzDrwps9EztbGw0bzTnHyqMu103dEFJWcAQXm9sDo8kBeq8YIo6foOAmYOgMWo
susCAwEAAaNgMF4wHwYDVR0jBBgwFoAUKCmKJv6jDNsv7wcCwIqenJNSYCEwHQYD
VR0OBBYEFJ5/Va3wx+TmRTmjGDbwRNAVNJaRMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQAJk7CaRQU4/hbdcX3xxBt2a5zD
B7Z2LQUj+13iaViPTYP1yFwAvm4d7TM59uNbdrRq6u6ysDfqwy7S+qn1qVeFOcTN
usnJ8Ou2zUoWXHBZwQg9R/QIUapvJOcJhbn+nPWwxYrmWPtQrexf5XnTvSi/i6MF
8v5+6+qiT5cIsNKijJG8hX+pSO63rBws8MF+CZqFbKA3rkVkJpdHm/kruwqNlFif
ByjKNPD2Z68X8CAnesp4LNnESMGDNLHrO+JsyUgbVx2fLN93rPBWAin/mAyZeriZ
InK0RFkOHsfbIJma1m4Af2KKikd8onlAKfzSvSz7cclB5XcHAlhmNcBKRNLu
-----END CERTIFICATE-----
)EOF";

static const char AWS_PRIVATE_KEY[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEArWPSpACwZwjMaX1CbfWwbajN1S/KIALttrrRtEw6uBgQtpQ3
ymH/6+UzceGaUebQ2Ik32Dl22hHMx28F3GAlSqoVVY5firJmrwdPFw7JHEr2VeMv
LIY/nh778u3FyFVY9AfVW52vtpQ9bZX910FHtRRqtdwTRXxravCreAEk9JIeTmIJ
3U0Um7u/N7fvZ5LaLZnqvw3PpyQjp395b2b4M3/DGAwWgQ1VnjkU/53TC/zo4jF0
cKKSDIcdzdNgi7oHpZ0pkjQyBHH8v0DVrAdFUfMOvCmz0TO1sbDRvNOcfKoy7XTd
0QUlZwBBeb2wOjyQF6rxgijp+g4CZg6Axaiy6wIDAQABAoIBAQCnK4Eg4hExpcf/
ROdRQPnTIFcjXaoQ5wHtlX0tyfkrGPi0L38Cvy2RcDADcjHpGRLuUo3nCLBQW60F
80LBUGlj7UwYauYuwQZDZlaYsoavoo6SDDGlSeH4P4NGmnyAQ9k5nnvUktmgvJfl
GiHc8XGDYqXnpMFn0yd1uYh4cRICQW/ZWy0LuiJqGIEBniTO5LYkTRNgeCn721YV
CO5PbFEX3unwyr2a/2YCICCwU7YaakiImObfnhRE0UFXO+p2zOQmFed+sJP9AR5j
rjoyBTLu2l999wCq6YYxiyMXi+6xDFGeulnp/7xe3gQszQWefOWYrd+Ti4sNgTrf
QsU4GRshAoGBAN6qUMblg4smS4Ip3b650PmdR0bKgPu1yTLBkWAyDdYZqyVL+Co+
oUzpHTuF1ttFdLR8g/0bX2lglJfciX8v58MWpXXtSPgLN5LWTJhNP+N/fg1rNT37
xVfI6itaakKDPXapX/7q71M3TWxRW9q2hc2BQJeXuajsphqjvO5J4PNJAoGBAMdZ
BVUO8vm1oqCAorcBz/TEyQLibOh50lcA2jl0uuUAyLCAD2yQe5qhGg4xPr7eAUsA
S11R89KABciWlgBuVuconAiT/cN1uitFI9ymrgQDQ7y0zIBkcPajuXKN8fOvdsni
CNv/F7tbiNiyCkuDZVLx/5WSpku4SDZ0OQjAlQCTAoGBAN5P2/CJjKmspW0jEcNw
UMY4EscKPneSC3zCqLwLnabq3+aQEAlVmMRqZwZb3aX1lczyGPHnl35lsFZjGWDE
Wrh8btzr+ZO111mi3rydPDGOxDLvnCvzqRe5gJuiXT9lO0OzXiXtON+z8ng4TuZy
n+sLpor0MKkJBdrJKkQbvR4JAoGBAIgGP3Q12if3/7tDa0QzEWJ0sLCuLiBHl/ZU
F5QjCbBfY5HEIEI1KJJvoWbTxXvZ4sn5rnNEC3q0br3buwkaQWREOoxD52FrQJhr
+jeC6LTGQX9PY0eswcQaHd2D6/ga35SMDv2Gx/vJTsyGWC4Ev6vkg0Ddq2l5mpTF
o3FMb5/PAoGAIeq3yVoywrG2I5J7JKjBmeqlnZaEOOfX8kRzDlpvakownQRsHey8
rZJlPipY5EGw7WElq8EzwQ0cx/3Udih23JTHLavgAai85MvtJ0258Bu+WYNgcT4O
xWwS7/YN26mFEy9WuWENMoRQ2sDk45JHQDCjtRUTMGE3osKaPz1MCVw=
-----END RSA PRIVATE KEY-----
)EOF";

// =========================================
// CLIENTES MQTT / TLS
// =========================================

static WiFiClientSecure secureClient;
static PubSubClient mqttClient(secureClient);

// =========================================
// HELPERS PARA COMANDOS
// =========================================

static void handleMqttCommand(const String& cmdRaw) {
  String cmd = cmdRaw;
  cmd.trim();
  cmd.toUpperCase();

  Serial.print("[MQTT] Comando recebido: '");
  Serial.print(cmd);
  Serial.println("'");

  if (cmd == "OPEN") {
    varalControllerSetMode(VaralMode::FORCE_OPEN);
  } else if (cmd == "CLOSE") {
    varalControllerSetMode(VaralMode::FORCE_CLOSE);
  } else if (cmd == "AUTO") {
    varalControllerSetMode(VaralMode::AUTO);
  } else {
    Serial.println("[MQTT] Comando desconhecido (ignorado).");
  }
}

// =========================================
// CALLBACK DE MENSAGENS
// =========================================

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[MQTT] Mensagem recebida em [");
  Serial.print(topic);
  Serial.print("]: ");

  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println(msg);

  // Tratar comandos no tópico de comando
  if (String(topic) == MQTT_TOPIC_CMD) {
    handleMqttCommand(msg);
  }

  // Se quiser tratar outros tópicos no futuro, é aqui
}

// =========================================
// FUNÇÕES INTERNAS
// =========================================

static void mqttConnect() {
  if (mqttClient.connected()) {
    return;
  }

  Serial.print("[MQTT] Conectando ao broker: ");
  Serial.println(AWS_IOT_ENDPOINT);

  // Tenta até conectar
  while (!mqttClient.connected()) {
    if (!wifiIsConnected()) {
      Serial.println("[MQTT] Wi-Fi não está conectado, abortando tentativa MQTT.");
      return;
    }

    bool ok = mqttClient.connect(MQTT_CLIENT_ID);
    if (ok) {
      Serial.println("[MQTT] Conectado!");

      // Inscreve nos tópicos de comando
      if (mqttClient.subscribe(MQTT_TOPIC_CMD)) {
        Serial.print("[MQTT] Inscrito em: ");
        Serial.println(MQTT_TOPIC_CMD);
      } else {
        Serial.println("[MQTT] Falha ao inscrever em tópico de comando");
      }

      // Publica um "online" no tópico de STATUS (não mais no heartbeat)
      mqttClient.publish(MQTT_TOPIC_STATUS, "online");

    } else {
      Serial.print("[MQTT] Falha na conexão, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" | nova tentativa em 5s...");
      delay(5000);
    }
  }
}

static const char* modeToString(VaralMode mode) {
  switch (mode) {
    case VaralMode::AUTO:        return "AUTO";
    case VaralMode::FORCE_OPEN:  return "FORCE_OPEN";
    case VaralMode::FORCE_CLOSE: return "FORCE_CLOSE";
    default:                     return "UNKNOWN";
  }
}

static void mqttPublishHeartbeat() {
  if (!mqttClient.connected()) {
    return;
  }

  String payload = "{";

  // DHT
  if (dht11HasValidData()) {
    payload += "\"temp_c\":";
    payload += String(dht11GetTemperatureC(), 1);
    payload += ",\"humidity\":";
    payload += String(dht11GetHumidity(), 1);
  } else {
    payload += "\"temp_c\":null,\"humidity\":null";
  }

  // Chuva
  bool chovendo = rainIsRaining();
  payload += ",\"rain\":";
  payload += (chovendo ? "true" : "false");

  // Modo atual do varal (seu controlador)
  VaralMode mode = varalControllerGetMode();
  payload += ",\"mode\":\"";
  payload += modeToString(mode);
  payload += "\"";

  // Timestamp local (millis)
  payload += ",\"uptime_ms\":";
  payload += String(millis());

  payload += "}";

  Serial.print("[MQTT] Heartbeat -> ");
  Serial.println(payload);

  mqttClient.publish(MQTT_TOPIC_HEARTBEAT, payload.c_str());
}

// =========================================
// API PÚBLICA
// =========================================

void mqttInit() {
  // Configura TLS
  secureClient.setCACert(AWS_ROOT_CA);
  secureClient.setCertificate(AWS_CLIENT_CERT);
  secureClient.setPrivateKey(AWS_PRIVATE_KEY);

  // Configura broker e callback
  mqttClient.setServer(AWS_IOT_ENDPOINT, AWS_IOT_PORT);
  mqttClient.setCallback(mqttCallback);

  // Primeira tentativa de conexão
  mqttConnect();
}

void mqttLoop() {
  if (!wifiIsConnected()) {
    return; // sem Wi-Fi, sem MQTT
  }

  if (!mqttClient.connected()) {
    mqttConnect();
  }

  mqttClient.loop();

  // Heartbeat periódico
  unsigned long now = millis();
  if (now - lastHeartbeatMillis >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeatMillis = now;
    mqttPublishHeartbeat();
  }
}
