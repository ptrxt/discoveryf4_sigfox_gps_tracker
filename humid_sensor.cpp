//  Implements the humidity sensor for BME280. Caller must use a semaphore to
//  prevent concurrent access to BME280 module, which runs on a single I2C Bus.
#include <Arduino.h>
#include <Wire.h>
#include <BME280I2C.h>
#include <stdlib.h>
#include <string.h>
#include "cocoos_cpp.h"  //  TODO: Workaround for cocoOS in C++
#include "display.h"
#include "sensor.h"
#include "bme280.h"
#include "humid_sensor.h"

//  These are the sensor functions that we will implement in this file.
static void init_sensor(void);
static uint8_t poll_sensor(float *data, uint8_t size);

//  Number of floats that this sensor returns as sensor data.
#define sensorDataSize 1

//  Construct a sensor object with the sensor functions.
static Sensor sensor(
  "hmd",                //  Name of sensor. The Structured Message field will use this name.
  &init_sensor,         //  Function for initialising the sensor.
  &poll_sensor          //  Function for polling sensor data.
);

static SensorContext sensorContext;  //  Remembers the sensor context.
static float sensorData[sensorDataSize];  //  Array of floats for remembering the sensor data.

static void init_sensor(void) {
  //  Initialise the sensor if necessary. sensor and sensorContext objects have been populated.
  bme280_setup();  //  Set up the BME280 API.
}

static uint8_t poll_sensor(float *data, uint8_t size) {
  //  Poll the sensor for new data.  Copy the received sensor data into the provided data buffer.
  //  Return the number of floats copied.  If no data is available, return 0.
  debug(sensor.info.name, " >> poll_sensor"); ////
  
  //  Read sensor data from BME280.
  sensorData[0] = bme.hum();  //  Get relative humidity as percentage.

  //  Simulated sensor.
  //// sensorData[0] = 12.3 + rand() % 10;

  //  Copy the received sensor data and return the number of floats copied.
  return receive_sensor_data(sensorData, sensorDataSize, data, size);
}

SensorContext *setup_humid_sensor(
  uint16_t pollInterval,
  uint8_t displayTaskID
  ) {
  //  Set up and return the sensor context.
  setup_sensor_context(&sensorContext, &sensor, pollInterval, displayTaskID);
  return &sensorContext;
}