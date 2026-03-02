/* env_sensor.h - Environmental sensor header */

#ifndef ENV_SENSOR_H
#define ENV_SENSOR_H

void EnvSensor_Init(void);
float EnvSensor_ReadTemperature(void);
float EnvSensor_ReadHumidity(void);

#endif /* ENV_SENSOR_H */
