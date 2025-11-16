#include "api.h"
#include "Arduino.h"
#include <stdarg.h>

// NOTE!!!
// Don't assume the below API follows good coding or PTx conventions.
// Follow your own best judgement for your own code.
// Feel free to rework this API if you like (not required).

static void door_sensor_IRQ_handler();


void setup_api()
{
  pinMode(2, OUTPUT); // digital pin 2
  pinMode(7, OUTPUT); // digital pin 7
  pinMode(3, INPUT_PULLUP); // door sensor on pin 3: use internal pull-up; connect switch to GND

  door_sensor_interrupt_handler(digitalRead(3) == HIGH); // we may not get an interrupt at startup, so we call the handler manually
  attachInterrupt(digitalPinToInterrupt(3), door_sensor_IRQ_handler, CHANGE); // digital pin 3
}

static void door_sensor_IRQ_handler()
{
  door_sensor_interrupt_handler(digitalRead(3) == HIGH);
}

// returns voltage in millivolts
uint16_t read_voltage(input_t input)
{
  if (input == TEMPERATURE_SENSOR)
  {
    return (uint32_t)analogRead(A0) * 5000 / 1023;
  }
  else if (input == TEMPERATURE_SENSOR_REFERENCE)
  {
    return (uint32_t)analogRead(A1) * 5000 / 1023;
  }

  return 0;
}

// true for on, false for off
void set_output(output_t output, bool output_state)
{
  if (output == GAS_VALVE)
  {
    digitalWrite(2, output_state);
  }
  else if (output == IGNITER)
  {
    digitalWrite(7, output_state);
  }
}

// read current output state
bool read_output(output_t output)
{
  if (output == GAS_VALVE)
  {
    return digitalRead(2) == HIGH;
  }
  else if (output == IGNITER)
  {
    return digitalRead(7) == HIGH;
  }
  return false;
}

uint32_t get_millis()
{
  return millis();
}

void serial_printf(const char * format, ...)
{
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, 256, format, args);
  va_end (args);

  Serial.print(buffer);
}
