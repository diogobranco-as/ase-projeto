#ifndef FAN_PWM_H
#define FAN_PWM_H

void fan_pwm_init(void);
void temperature_pwm_control(float temperature, float temp_min, float temp_max);

#endif