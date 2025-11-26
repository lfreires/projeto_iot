#include <Arduino.h>
#include "stepper_motor.h"

// ==========================
// CONFIGURAÇÃO DE PINOS
// ==========================

// Ligue IN1..IN4 do ULN2003 nesses pinos:
static const int STEPPER_IN1_PIN = 14; // IN1
static const int STEPPER_IN2_PIN = 27; // IN2
static const int STEPPER_IN3_PIN = 26; // IN3
static const int STEPPER_IN4_PIN = 33; // IN4

static const int ENDSTOP_PIN = 32; // ex: 32 se você colocar um fim de curso

// 28BYJ-48 em half-step ~4096 passos por volta
static const long STEPS_PER_REV = 4096;

// Velocidade padrão (half-steps/s)
static float stepperSpeedStepsPerSec = 400.0f;

// ==========================
// ESTADO INTERNO
// ==========================

// Posição atual (sempre 0..STEPS_PER_REV-1)
static long currentSteps = 0;
// Alvo (0..STEPS_PER_REV-1)
static long targetSteps  = 0;

// Índice da fase (0..7) da sequência half-step
static uint8_t phaseIndex = 0;

static unsigned long lastStepMicros     = 0;
static unsigned long stepIntervalMicros = 0;

static bool homed = false;

// Simples “estado” do motor
enum class StepperMode {
  IDLE,
  MOVING,
  HOMING
};

static StepperMode mode = StepperMode::IDLE;

// Sequência de half-step (8 fases)
static const uint8_t HALFSTEP_SEQ[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};

// ==========================
// FUNÇÕES INTERNAS
// ==========================

static void updateStepInterval() {
  if (stepperSpeedStepsPerSec <= 0) {
    stepIntervalMicros = 0;
  } else {
    stepIntervalMicros = (unsigned long)(1'000'000.0f / stepperSpeedStepsPerSec);
  }
}

static void applyPhase(uint8_t idx) {
  idx &= 0x07;
  digitalWrite(STEPPER_IN1_PIN, HALFSTEP_SEQ[idx][0]);
  digitalWrite(STEPPER_IN2_PIN, HALFSTEP_SEQ[idx][1]);
  digitalWrite(STEPPER_IN3_PIN, HALFSTEP_SEQ[idx][2]);
  digitalWrite(STEPPER_IN4_PIN, HALFSTEP_SEQ[idx][3]);
}

// Anda 1 passo em uma direção
static void stepOnce(bool clockwise) {
  if (clockwise) {
    phaseIndex = (phaseIndex + 1) & 0x07;
    currentSteps++;
  } else {
    phaseIndex = (phaseIndex + 7) & 0x07; // -1 mod 8
    currentSteps--;
  }

  // Mantém dentro de 0..STEPS_PER_REV-1
  if (currentSteps >= STEPS_PER_REV) currentSteps -= STEPS_PER_REV;
  if (currentSteps < 0)              currentSteps += STEPS_PER_REV;

  applyPhase(phaseIndex);
}

// Decide o menor caminho (horário ou anti) pra ir de currentSteps a targetSteps
static void moveOneStepTowardTarget() {
  if (currentSteps == targetSteps) {
    mode = StepperMode::IDLE;
    return;
  }

  // Se o alvo é maior que a posição -> gira em um sentido
  // Se o alvo é menor -> gira no outro
  bool clockwise = (targetSteps > currentSteps);
  stepOnce(clockwise);
}

// ==========================
// FUNÇÕES PÚBLICAS
// ==========================

void stepperInit() {
  pinMode(STEPPER_IN1_PIN, OUTPUT);
  pinMode(STEPPER_IN2_PIN, OUTPUT);
  pinMode(STEPPER_IN3_PIN, OUTPUT);
  pinMode(STEPPER_IN4_PIN, OUTPUT);

  digitalWrite(STEPPER_IN1_PIN, LOW);
  digitalWrite(STEPPER_IN2_PIN, LOW);
  digitalWrite(STEPPER_IN3_PIN, LOW);
  digitalWrite(STEPPER_IN4_PIN, LOW);

  if (ENDSTOP_PIN >= 0) {
    pinMode(ENDSTOP_PIN, INPUT_PULLUP); // ajuste se usar outro esquema
  }

  currentSteps = 0;
  targetSteps  = 0;
  phaseIndex   = 0;
  applyPhase(phaseIndex);

  homed = (ENDSTOP_PIN < 0);  // se não tem fim de curso, assume homed lógico

  stepperSetSpeed(stepperSpeedStepsPerSec);
  lastStepMicros = micros();
  mode = StepperMode::IDLE;

  Serial.println("[STEPPER] 28BYJ-48 inicializado.");
}

// Chamar no loop()
void stepperLoop() {
  if (stepIntervalMicros == 0) {
    return; // velocidade 0
  }

  unsigned long now = micros();
  if (now - lastStepMicros < stepIntervalMicros) {
    return; // ainda não é hora do próximo passo
  }
  lastStepMicros = now;

  if (mode == StepperMode::MOVING) {
    moveOneStepTowardTarget();
  } else if (mode == StepperMode::HOMING) {
    if (ENDSTOP_PIN < 0) {
      // não tem fim de curso, aborta homing
      mode = StepperMode::IDLE;
      return;
    }

    // Anda sempre na direção do fim de curso, por exemplo “fechar”
    // aqui vou assumir anti-horário (clockwise=false), ajuste se precisar
    stepOnce(false);

    // Travinha de segurança: se der mais de 3 voltas, aborta
    static long stepsHoming = 0;
    stepsHoming++;
    if (stepsHoming > STEPS_PER_REV * 3) {
      Serial.println("[STEPPER] Homing falhou (não achou fim de curso).");
      mode = StepperMode::IDLE;
      stepsHoming = 0;
      return;
    }

    int endstopState = digitalRead(ENDSTOP_PIN);
    // Supondo fim de curso para GND: LOW = acionado
    if (endstopState == LOW) {
      Serial.println("[STEPPER] Homing OK. Zero definido.");
      currentSteps = 0;
      targetSteps  = 0;
      homed        = true;
      mode         = StepperMode::IDLE;
      stepsHoming  = 0;
    }
  }
}

void stepperMoveToSteps(long newTargetSteps) {
  // Normaliza alvo pra 0..STEPS_PER_REV-1
  while (newTargetSteps < 0)            newTargetSteps += STEPS_PER_REV;
  while (newTargetSteps >= STEPS_PER_REV) newTargetSteps -= STEPS_PER_REV;

  targetSteps = newTargetSteps;
  mode = StepperMode::MOVING;
}

void stepperMoveRelativeSteps(long deltaSteps) {
  long newTarget = currentSteps + deltaSteps;
  stepperMoveToSteps(newTarget);
}

void stepperSetSpeed(float stepsPerSecond) {
  if (stepsPerSecond <= 0) {
    stepperSpeedStepsPerSec = 0;
  } else {
    stepperSpeedStepsPerSec = stepsPerSecond;
  }
  updateStepInterval();
}

bool stepperIsMoving() {
  return mode != StepperMode::IDLE;
}

long stepperGetCurrentSteps() {
  return currentSteps;
}

// ===== ÂNGULO =====

long stepperAngleToSteps(float degrees) {
  // normaliza ângulo
  while (degrees < 0.0f)   degrees += 360.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;

  float ratio = degrees / 360.0f;
  long steps  = (long)round(ratio * (float)STEPS_PER_REV);
  if (steps >= STEPS_PER_REV) steps = 0;
  return steps;
}

void stepperMoveToAngle(float degrees) {
  long target = stepperAngleToSteps(degrees);
  stepperMoveToSteps(target);
}

// ===== HOMING =====

void stepperHome() {
  if (ENDSTOP_PIN < 0) {
    Serial.println("[STEPPER] Homing chamado mas ENDSTOP_PIN = -1.");
    return;
  }
  Serial.println("[STEPPER] Iniciando homing...");
  mode  = StepperMode::HOMING;
  homed = false;
}

bool stepperIsHomed() {
  return homed;
}
