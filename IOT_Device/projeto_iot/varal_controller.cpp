#include <Arduino.h>
#include "varal_controller.h"
#include "rain_sensor.h"
#include "stepper_motor.h"

// Ângulos do varal (ajuste de acordo com o teu mecanismo)
static const float VARAL_ANGULO_FECHADO = 0.0f;
static const float VARAL_ANGULO_ABERTO  = 270.0f; // ex: bem aberto

// Estado lógico do varal
enum class VaralState {
  UNKNOWN,
  FECHADO,
  ABERTO
};

static VaralState varalState = VaralState::UNKNOWN;

// Modo de operação (AUTO/force)
static VaralMode currentMode = VaralMode::AUTO;

// Controle de frequência das decisões
static unsigned long lastDecisionMillis = 0;
static const unsigned long DECISION_INTERVAL_MS = 2000; // 2s

// =======================
// API
// =======================

void varalControllerInit() {
  varalState = VaralState::UNKNOWN;
  currentMode = VaralMode::AUTO;     // sempre começa em AUTO, como antes
  lastDecisionMillis = 0;
  Serial.println("[VARAL] Controller inicializado (modo AUTO).");
}

void varalControllerSetMode(VaralMode mode) {
  currentMode = mode;
  Serial.print("[VARAL] Modo alterado para: ");
  switch (mode) {
    case VaralMode::AUTO:        Serial.println("AUTO");        break;
    case VaralMode::FORCE_OPEN:  Serial.println("FORCE_OPEN");  break;
    case VaralMode::FORCE_CLOSE: Serial.println("FORCE_CLOSE"); break;
  }
}

VaralMode varalControllerGetMode() {
  return currentMode;
}

// =======================
// LOOP
// =======================

void varalControllerLoop() {
  unsigned long now = millis();

  // Só decide de tempos em tempos
  if (now - lastDecisionMillis < DECISION_INTERVAL_MS) {
    return;
  }
  lastDecisionMillis = now;

  // Se o motor ainda está em movimento, espera ele terminar
  if (stepperIsMoving()) {
    return;
  }

  // Se ainda não terminou o homing, não faz nada
  if (!stepperIsHomed()) {
    Serial.println("[VARAL] Aguardando homing...");
    return;
  }

  // Primeira vez depois do homing: assume que está FECHADO
  if (varalState == VaralState::UNKNOWN) {
    varalState = VaralState::FECHADO;
    Serial.println("[VARAL] Estado inicial assumido: FECHADO");
  }

  // ======== MODO AUTO (COMPORTAMENTO ANTIGO) ========
  if (currentMode == VaralMode::AUTO) {
    bool chovendo = rainIsRaining();

    if (chovendo) {
      // Chovendo -> fechamos o varal se ainda não estiver fechado
      if (varalState != VaralState::FECHADO) {
        Serial.println("[VARAL] AUTO: Chovendo -> FECHAR varal");
        stepperMoveToAngle(VARAL_ANGULO_FECHADO);
        varalState = VaralState::FECHADO;
      }
    } else {
      // Não está chovendo -> abrimos o varal se ainda não estiver aberto
      if (varalState != VaralState::ABERTO) {
        Serial.println("[VARAL] AUTO: Seco -> ABRIR varal");
        stepperMoveToAngle(VARAL_ANGULO_ABERTO);
        varalState = VaralState::ABERTO;
      }
    }
    return; // já tratou AUTO, sai
  }

  // ======== MODO FORCE_OPEN ========
  if (currentMode == VaralMode::FORCE_OPEN) {
    if (varalState != VaralState::ABERTO) {
      Serial.println("[VARAL] FORCE_OPEN: Abrindo varal (ignorando chuva)");
      stepperMoveToAngle(VARAL_ANGULO_ABERTO);
      varalState = VaralState::ABERTO;
    }
    return;
  }

  // ======== MODO FORCE_CLOSE ========
  if (currentMode == VaralMode::FORCE_CLOSE) {
    if (varalState != VaralState::FECHADO) {
      Serial.println("[VARAL] FORCE_CLOSE: Fechando varal (ignorando chuva)");
      stepperMoveToAngle(VARAL_ANGULO_FECHADO);
      varalState = VaralState::FECHADO;
    }
    return;
  }
}
