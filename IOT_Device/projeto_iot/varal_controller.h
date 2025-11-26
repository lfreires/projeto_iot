// varal_controller.h
#pragma once
#include <Arduino.h>

enum class VaralMode : uint8_t {
  AUTO = 0,
  FORCE_OPEN,
  FORCE_CLOSE
};

void varalControllerInit();
void varalControllerLoop();

// usados pelo MQTT
void varalControllerSetMode(VaralMode mode);
VaralMode varalControllerGetMode();
