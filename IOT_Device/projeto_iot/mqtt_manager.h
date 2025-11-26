#pragma once

// Inicializa MQTT (configura TLS, endpoint, etc.)
void mqttInit();

// Chamar sempre no loop principal
void mqttLoop();
