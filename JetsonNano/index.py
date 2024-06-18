import smbus            
from time import sleep
import math
import numpy as np
import pandas as pd
import time
import pickle
from io import StringIO
import requests
from datetime import datetime

import SVM_Embedded as _embedded

#some MPU6050 Registers and their Address
PWR_MGMT_1   = 0x6B
SMPLRT_DIV   = 0x19
CONFIG       = 0x1A
GYRO_CONFIG  = 0x1B
INT_ENABLE   = 0x38
ACCEL_XOUT_H = 0x3B
ACCEL_YOUT_H = 0x3D
ACCEL_ZOUT_H = 0x3F
GYRO_XOUT_H  = 0x43
GYRO_YOUT_H  = 0x45
GYRO_ZOUT_H  = 0x47

def call_cloud_function():
    url = 'https://us-central1-falling-detection-3e200.cloudfunctions.net/handleFallDetection'
   
    body = {
        'data': {
            'deviceID': 'DEVICE01',
            'time': datetime.now().strftime("%d/%m/%Y %H:%M:%S")
        }
    }
    response = requests.post(url, json=body)
	
def MPU_Init():
	#write to sample rate register
	bus.write_byte_data(Device_Address, SMPLRT_DIV, 7)

	#Write to power management register
	bus.write_byte_data(Device_Address, PWR_MGMT_1, 1)

	#Write to Configuration register
	bus.write_byte_data(Device_Address, CONFIG, 0)

	#Write to Gyro configuration register
	bus.write_byte_data(Device_Address, GYRO_CONFIG, 24)

	#Write to interrupt enable register
	bus.write_byte_data(Device_Address, INT_ENABLE, 1)
 
def read_raw_data(addr):
	#Accelero and Gyro value are 16-bit
        high = bus.read_byte_data(Device_Address, addr)
        low = bus.read_byte_data(Device_Address, addr+1)
     
        #concatenate higher and lower value
        value = ((high << 8) | low)
         
        #to get signed value from mpu6050
        if(value > 32768):
                value = value - 65536
        return value
 
 
bus = smbus.SMBus(1)    # or bus = smbus.SMBus(0) for older version boards
Device_Address = 0x68   # MPU6050 device address

df = pd.DataFrame(columns = ['time', 'acc_x', 'acc_y', 'acc_z', 'gyro_x', 'gyro_y', 'gyro_z', 'acc_re', 'gyro_re'])
 
MPU_Init()
 
while True:
	start_time = time.time()*1000
	duration = time.time()*1000 - start_time
	while duration < 1 * 1000:
		#Read Accelerometer raw value
		acc_x = read_raw_data(ACCEL_XOUT_H)
		acc_y = read_raw_data(ACCEL_YOUT_H)
		acc_z = read_raw_data(ACCEL_ZOUT_H)

		#Read Gyroscope raw value
		gyro_x = read_raw_data(GYRO_XOUT_H)
		gyro_y = read_raw_data(GYRO_YOUT_H)
		gyro_z = read_raw_data(GYRO_ZOUT_H)

		#Full scale range +/- 250 degree/C as per sensitivity scale factor
		Ax = acc_x / 16384.0 * 9.81
		Ay = acc_y / 16384.0 * 9.81
		Az = acc_z / 16384.0 * 9.81
		Are = np.sqrt(Ax ** 2 + Ay ** 2 ++ Az ** 2)

		Gx = gyro_x / 131.0 * math.pi / 180
		Gy = gyro_y / 131.0 * math.pi / 180
		Gz = gyro_z / 131.0 * math.pi / 180
		Gre = np.sqrt(Gx ** 2 + Gy ** 2 ++ Gz ** 2)

		duration = time.time()*1000 - start_time
		
		new_row = {
			'time': duration,
			'acc_x': Ax, 'acc_y': Ay, 'acc_z': Az,
			'gyro_x': Gx, 'gyro_y': Gy, 'gyro_z': Gz,
			'acc_re': Are, 'gyro_re': Gre
		}

		df = df.append(new_row, ignore_index=True)

	### Load model từ file pickle
	with open('/home/ntthuthao1121/PBL5/best_model.pkl', 'rb') as file:
		model = pickle.load(file)
	### Test hàm dự báo
	result = _embedded.predict(model, df)

	if result==1:
		call_cloud_function()
	df = df.drop(df.index)
