// Importing Libraries
#include <Wire.h> //I^2C communication
#include <Adafruit_MPU6050.h> //MPU6050 registers
#include <Adafruit_Sensor.h> //formatting for sensor data

//create MPU6050 object
Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200); //start com ESP32 <-> computer 
  // 115200 = baud rate (communication speed)
  // 115200 = 115,200 bits per second / 8 = 14,400 bytes per second
  // suggested by chatGPT -> common for esp32(?)
  // set serial monitor and plotters to same baud if changing 

  // SDA = GPIO4, SCL = GPIO5
  Wire.begin(4, 5);

  if (!mpu.begin()) { // initialize mpu6050 and verify connection
    Serial.println("MPU6050 not found!");
    while (1) { //stays here if sensor isnt found
      delay(10); //to prevent esp32 from relooping a ton of times per sec
    }
  }

  Serial.println("MPU6050 connected!");
}

void loop() {

  // variables to store latest readings from mpu6050
  sensors_event_t accel, gyro, temp;

  // read current sensor measurements and store them in the variables above
  mpu.getEvent(&accel, &gyro, &temp);

  // print the x acceleration value
  Serial.print(accel.acceleration.x);
  Serial.print(",");

  // print the y acceleration value
  Serial.print(accel.acceleration.y);
  Serial.print(",");

  // print the z acceleration value
  Serial.print(accel.acceleration.z);
  Serial.print(",");

  // print the x gyroscope value
  Serial.print(gyro.gyro.x);
  Serial.print(",");

  // print the y gyroscope value
  Serial.print(gyro.gyro.y);
  Serial.print(",");

  // print z gyroscope value and move to next line
  Serial.println(gyro.gyro.z);

  // wait 100 ms inbetween measurements
  // code samples at around 10 Hz (10 readings/sec), change delay for diff hz
  delay(100);
}