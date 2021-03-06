//  Common code for all sensors.
//#define DISABLE_DEBUG_LOG  //  Disable debug logging.
#include "platform.h"
#include <string.h>
#include <cocoos.h>
#include "sensor.h"


static uint8_t nextSensorID = 1;  //  Next sensor ID to be allocated.  Running sequence number.

void setup_sensor_context( SensorContext *context, Sensor *sensor, uint16_t pollInterval, uint8_t taskID) {
  //  Set up the sensor context and call the sensor to initialize itself.
  //  Allocate a unique sensor ID and create the event.
  uint8_t sensorID =  nextSensorID++;
  Evt_t event = event_create();

  //  Initialise the sensor values.
  sensor->info.id = sensorID; 
  sensor->info.event = event;
  sensor->info.poll_interval = pollInterval;

  //  Set the context.
  context->sensor = sensor;
  context->receive_task_id = taskID;

  //  Call the sensor to do initialisation.
  sensor->control.init_sensor_func();
}

void sensor_task(void) {
  //  Background task to receive and process sensor data.
  //  This task will be reused by all sensors: temperature, humidity, altitude.
  //  Don't declare any static variables inside here because they will conflict
  //  with other sensors.
  SensorContext *context = NULL;

  task_open();
  context = (SensorContext *) task_get_data();

  // Initialize the SensorMsg contained in this sensor context
  context->msg.super.signal = SENSOR_DATA_SIG;
  context->msg.sensorId = context->sensor->info.id;

  for (;;) {

    //  Wait for event or timeout
    if (context->sensor->info.poll_interval) {
      task_wait(context->sensor->info.poll_interval);
    }
    else {
      event_wait_timeout(context->sensor->info.event, TICKS_PER_S * 10);
    }

    context = (SensorContext *) task_get_data();
    sem_wait(i2cSemaphore);

    //  We have to fetch the context pointer again after the wait.
    context = (SensorContext *) task_get_data();

    //  Poll for the sensor data and copy into the message.
    context->msg.count = context->sensor->info.poll_sensor_func(context->msg.data, MAX_SENSOR_DATA_SIZE);

    //  We are done with the I2C Bus.  Release the semaphore so that another task can fetch the sensor data.
    sem_signal(i2cSemaphore);

    context = (SensorContext *) task_get_data();

    //  Do we have new data?
    if (context->msg.count > 0) {
      msg_post_async(context->receive_task_id, context->msg);
    }

  }

  task_close();  //  End of the task. Should never come here.
}



//  SensorInfo constructor for C++ only.
SensorInfo::SensorInfo(
  const char name0[],
  uint8_t (*poll_sensor_func0)(float *data, uint8_t size)
) {
  name = name0;
  poll_sensor_func = poll_sensor_func0;
}

//  SensorInfo constructor for C++ only.
SensorControl::SensorControl(
  void (*init_sensor_func0)(void),
  void (*next_channel_func0)(void),
  void (*prev_channel_func0)(void)
) {
  init_sensor_func = init_sensor_func0;
  next_channel_func = next_channel_func0;
  prev_channel_func = prev_channel_func0;
}

//  Sensor constructor for C++ only.
Sensor::Sensor(
  const char name[],
  void (*init_sensor_func)(void),
  uint8_t (*poll_sensor_func)(float *data, uint8_t size),
  void (*next_channel_func)(void),
  void (*prev_channel_func)(void)
): 
  info(name, poll_sensor_func),
  control(init_sensor_func, next_channel_func, prev_channel_func) {
}

