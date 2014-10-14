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

class GUI(object):
	def __init__(self, rPI = True):
		self.rPI = rPI
		self.PUSH_BUTTONS = (17, 27, 22, 23)
		self.RDY = 24
		if self.rPI:
			import RPi.GPIO as GPIO
			from Adafruit_CharLCD import Adafruit_CharLCD
			self.GPIO = GPIO
			self.lcd = Adafruit_CharLCD()
			GPIO.setwarnings(False)
			GPIO.setmode(GPIO.BCM)
			GPIO.setup(self.RDY, GPIO.OUT)
			self.setRDYstate(1)
			for i in self.PUSH_BUTTONS:
				GPIO.setup(i, GPIO.IN)


	def readPushButtons(self):
		state = []
		for i in self.PUSH_BUTTONS:
			state.append(self.GPIO.input(i))
		return state

	def setRDYstate(self, state):
		self.GPIO.output(self.RDY, state)

	def lcdClear(self):
		if self.rPI:
			self.lcd.clear()

	def lcdMessage(self, msg):
		if self.rPI:
			self.lcd.message(msg)



class logger(object):
	def __init__(self, rPI = True, url = ""):
		self.url = url
		self.rPI = rPI
		self.fileName = self.getFileName()

		if rPI:
			self.PORT = '/dev/ttyACM0'
			
		else:
			self.PORT = 'COM54'

		self.BAUDRATE = 115200
		self.TIMEOUT = 0
		self.openSerial()

		self.gui = GUI(self.rPI)

	def openSerial(self):
		try:
			self.uart = serial.Serial(self.PORT, self.BAUDRATE, timeout = self.TIMEOUT)
			self.uart.flushInput()
			self.uart.flushOutput()
			return 1
		except:
			print "Error al abrir puerto serial " + self.PORT
			return None

	def readSerial(self):
		try:
			#self.uart.write('a')
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
		archivo = open(self.fileName, 'a')
		archivo.write(data)
		archivo.close()

	def uploadData(self, data):
		pass

	def getRPi(self):
		return self.rPI

	def translateDate(self, day, month, year):
		MONTHS = ('Jan, Feb, Mar, Apr, Jun, Jul, Aug, Sep, Oct, Nov, Dec')
		return str(year)+str(MONTHS.find(month)+1)+str(day)

	def translateTime(self, time):
		timeSplit = time.split(':')
		time = timeSplit[0]+timeSplit[1]+timeSplit[2]
		return time

	def getCurrentDateTime(self):
		dateTime = time.ctime()
		s = dateTime.split(' ')
		return s

	def getCurrentTime(self):
		return self.getCurrentDateTime()[3] #Return current time

	def getCurrentDate(self):
		dateTime = self.getCurrentDateTime()
		dateTime = self.translateDate(dateTime[2], dateTime[1], dateTime[4])
		return dateTime

	def getFileName(self):
		return self.getCurrentDate() + '_' + self.getCurrentTime() + '.csv'		


log = logger(True)
try:
	while True:
		data = log.readSerial()
		log.appendData(data)
		print data,
		log.gui.lcdClear()
		log.gui.lcdMessage('T = ' + str(data).strip('\t').strip('\r\n'))
		#time.sleep(1)
except KeyboardInterrupt:
	log.closeSerial()
	print "Adios!"


'''
To set date:
date -s "2 OCT 2006 18:00:00"
'''
