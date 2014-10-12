#Wireless Sensor Network - Architecture School
#Universidad de San Carlos de Guatemala
#Eng. Ivan Rene Morales - October 2014

"""
This module is intended to be run on a Raspberry PI B+
along with a MSP430G2553 Microcontroller-based router station.

Data will be sent from uC to this server via UART (ttyAMA0), and then,
logged on a CSV file (which may be used to automate some stuff later on).
By now, RPi's job is to log and (may be) upload real-time data to the Cloud

This script will be launched as a daemon (init.d) and will notify
the uC when system is ready to start logging data.
"""

import serial
import time


class logger(object):
	def __init__(self, fileName, rPI = True, url = ""):
		self.fileName = fileName
		self.url = url

		if rPI:
			self.PORT = '/dev/ttyUSB0'
			self.initGPIO()
			from Adafruit_CharLCD import Adafruit_CharLCD
		else:
			self.PORT = 'COM29'

		self.BAUDRATE = 9600
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


	def readSerial(self):
		pass

	def appendData(self, data):
		pass

	def uploadData(self, data):
		pass


log = logger('a.txt', False)
try:
	while True:
		print log.readSerial()
		time.sleep(2)
except KeyboardInterrupt:
	log.closeSerial()
	print "Adios!"