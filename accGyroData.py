from imu import MPU6050
from machine import I2C, Pin
import time

i2c = I2C(1, sda=Pin(6), scl=Pin(7), freq=400000)
mpu = MPU6050(i2c)

while True:
    print(mpu.accel.x, mpu.accel.y, mpu.accel.z)
    time.sleep(0.05)
    #print(mpu.gyro.x, mpu.gyro.y, mpu.gyro.z)
    #time.sleep(0.05)