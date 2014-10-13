#Wireless Sensor Network - Architecture School
#Universidad de San Carlos de Guatemala
#Eng. Ivan Rene Morales - October 2014

"""
This module is intended to be run on a Raspberry PI B+
along with an MSP430G2553 Microcontroller-based router station.

Data will be sent from uC to this server via UART (ttyAMA0), and then,
logged on a CSV file (which may be used to automate some stuff later on).
By now, RPi's job is to log and upload real-time data to the Cloud

This script will be launched as a daemon (init.d) and will notify
the uC when system is ready to start logging data.

User interface is done via a CharLCD and a couple of push-buttons.

Using Adafruit_CharLCD and RPi.GPIO libraries to implement GPIO functionality

User will be prompted to enter current date/time only once, everytime
the OS boots.

Some functionality will be also implemented in order to send sensors' data
to exosite (http://exosite.com) so WSN may be plotted and read in real-time
"""

import serial
import time


class logger(object):
	def __init__(self, fileName, rPI = True, url = ""):
		self.fileName = fileName
		self.url = url
		self.rPI = rPI

		if rPI:
			self.PORT = '/dev/ttyACM0'
			self.initGPIO()
			from Adafruit_CharLCD import Adafruit_CharLCD
			self.lcd = Adafruit_CharLCD()
		else:
			self.PORT = 'COM54'

		self.BAUDRATE = 115200
		self.TIMEOUT = 5
		self.openSerial()

	def initGPIO(self):
		#import RPi.GPIO
		pass

	def openSerial(self):
		try:
			self.uart = serial.Serial(self.PORT, self.BAUDRATE, timeout = self.TIMEOUT)
			return 1
		except:
			print "Error al abrir puerto serial " + self.PORT
			return None

	def readSerial(self):
		try:
			self.uart.write('a')
			data = self.uart.readline()
			return data
		except:
			print "Error al leer puerto serial " + self.PORT
			return None

	def closeSerial(self):
		try:
			self.uart.close()
			return 1
		except:
			print "Error al cerrar puerto serial " + self.PORT
			return None

	def appendData(self, data):
		pass

	def uploadData(self, data):
		pass

	def getRPi(self):
		return self.rPI

	def lcdClear(self):
		if self.rPI:
			self.lcd.clear()

	def lcdMessage(self, msg):
		if self.rPI:
			self.lcd.message(msg)



log = logger('a.txt', True)
try:
	while True:
		data = log.readSerial()
		print data,
		log.lcdClear()
		log.lcdMessage('T = ' + str(data).strip('\r\n'))
		time.sleep(1)
except KeyboardInterrupt:
	log.closeSerial()
	print "Adios!"
