//  Aggregate sensor data and decide whether to send to network now.
#include <string.h>
#include "sensor.h"
#include "wisol.h"
#include "aggregate.h"

static SensorMsg *recallSensor(uint8_t id);
static void copySensorData(SensorMsg *dest, const SensorMsg *src);
static void addToPayload(int data);

//  Remember the last sensor value of each sensor.
static SensorMsg sensorData[MAX_SENSOR_COUNT];

//  Buffer for constructing the message payload to be sent, in hex digits, plus terminating null.
#define PAYLOAD_SIZE (1 + MAX_MESSAGE_SIZE * 2)  //  Each byte takes 2 hex digits. Add 1 for terminating null.
static char payload[PAYLOAD_SIZE];  //  e.g. "0102030405060708090a0b0c"

#define NO_SENSOR 0xff


void aggregate_saveSensorData(const SensorMsg *msg) {
  //  Aggregate the sensor data.  Here we just save the last value for each sensor.
  SensorMsg *savedSensor = recallSensor(msg->sensorId);

  if (savedSensor != NULL) {
    copySensorData(savedSensor, msg);
  }
}

void aggregate_getSensorData(char *buf) {
  //  Create a new Sigfox message. Add a running sequence number to the message.
  payload[0] = 0;
  static int sequenceNumber = 0;

  addToPayload(sequenceNumber++);


  for (int i = 0; i < MAX_SENSOR_COUNT; i++) {

    //  Get each sensor data and add to the message payload.
    float data = 0;

    if (sensorData[i].sensorId != NO_SENSOR) {
      data = sensorData[i].data[0];
      int scaledData = data * 10; //  Scale up by 10 to send 1 decimal place. So 27.1 becomes 271
      addToPayload(scaledData);
    }
  }

  //  If the payload has odd number of digits, pad with '0'.
  int length = strlen(payload);
  if (length % 2 != 0 && length < PAYLOAD_SIZE - 1) {
      payload[length] = '0';
      payload[length + 1] = 0;
  }

  // copy to output buffer
  strcpy(buf, payload);
}

static void addToPayload(int data) {
    //  Add the integer data to the message payload as numDigits digits in hexadecimal.
    //  So data=1234 and numDigits=4, it will be added as "1234".  Not efficient, but easy to read.
    int length = strlen(payload);

    if (length + 4 >= PAYLOAD_SIZE) {  //  No space for numDigits hex digits.
        return;
    }

    for (int i = 4 - 1; i >= 0; i--) {  //  Add the digits in reverse order (right to left).
        int d = data % 10;  //  Take the last digit.
        data = data / 10;  //  Shift to the next digit.
        payload[length + i] = '0' + d;  //  Write the digit to payload: 1 becomes '1'.
    }
    payload[length + 4] = 0;  //  Terminate the payload after adding numDigits hex chars.
}

static void copySensorData(SensorMsg *dest, const SensorMsg *src) {
    //  Copy sensor data from src to dest.
    for (int i = 0; i < src->count; i++) {
        dest->data[i] = src->data[i];
    }
    dest->count = src->count;
}

static SensorMsg *recallSensor(uint8_t id) {
    //  Return the sensor data for the sensor name.  If not found, allocate
    //  a new SensorMsg and return it.  If no more space, return NULL.
    int emptyIndex = -1;
    for (int i = 0; i < MAX_SENSOR_COUNT; i++) {
        //  Search for the sensor id in our data.
        if (sensorData[i].sensorId == id) {
            return &sensorData[i];  //  Found it.
        }

        //  Find the first empty element.
        if (emptyIndex == -1 && sensorData[i].sensorId == NO_SENSOR) {
            emptyIndex = i;
        }
    }

    //  Allocate a new element.
    if (emptyIndex == -1) {
      //  No more space.
      return NULL;
    }

    sensorData[emptyIndex].sensorId = id;
    sensorData[emptyIndex].count = 0;  //  Clear the values.
    sensorData[emptyIndex].data[0] = 0;  //  Reset to 0 in case we need to send.
    return &sensorData[emptyIndex];
}

void setup_aggregate(void) {
    //  Clear the aggregated sensor data.
    for (int i = 0; i < MAX_SENSOR_COUNT; i++) {
        sensorData[i].sensorId = NO_SENSOR;
        sensorData[i].count = 0;  //  Clear the values.
    }
}
