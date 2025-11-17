/*
We need your help to stop forest fires and bake tasty cookies!

See `requirements.md` for how to help.

Then check `notes.md`.
*/
#include "api.h"
#include "ptx_logging.h"
#include "ptx_oven_control.h"
#include "ptx_actuator.h"

void setup() {
  ptx_log_init();
  ptx_oven_control_init();
  setup_api();

  PTX_LOGF("Elf oven 2000 starting up.");
}

// Interrupt-driven door handler (pin 3, HIGH = OPEN)
// Immediate safety: cut GAS & IGNITER if door opens.
void door_sensor_interrupt_handler(bool voltage_high)
{
  // TODO: add small filtering for stability if needed
  if (voltage_high) {
    ptx_actuator_emergency_stop();
  }
  // Propagate state to controller; controller loop will handle any logging.
  ptx_oven_set_door_state(voltage_high);
}


void loop() {
  // Run oven control loop
  ptx_oven_control_update();
  delay(50); // ~20 Hz control loop; module logs once per second
}
