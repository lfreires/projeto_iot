#pragma once

// Inicializa motor (pinos, estado, etc.)
void stepperInit();

// Chamar no loop (não bloqueante)
void stepperLoop();

// === Movimento em STEPS (half-steps) ===
void stepperMoveToSteps(long targetSteps);     // alvo absoluto (0..steps por volta)
void stepperMoveRelativeSteps(long deltaSteps); // movimento relativo

void stepperSetSpeed(float stepsPerSecond);    // half-steps por segundo
bool stepperIsMoving();
long stepperGetCurrentSteps();

// === Movimento em ÂNGULO (0–360) ===
long stepperAngleToSteps(float degrees);       // conversão
void stepperMoveToAngle(float degrees);        // move pro ângulo alvo (0–360)

// === Homing (opcional, com fim de curso) ===
// Se você não tiver fim de curso, pode deixar implementado
// mas não chamar, ou marcar ENDSTOP_PIN = -1 no .cpp
void stepperHome();
bool stepperIsHomed();
