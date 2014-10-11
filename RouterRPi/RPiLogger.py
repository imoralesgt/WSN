#Wireless Sensor Network - Architecture School
#Universidad de San Carlos de Guatemala
#Eng. Ivan Rene Morales - October 2014

"""
This module is intended to be run on a Raspberry PI B+
along with a MSP430G2553 Microcontroller-based router station.

Data will be sent from uC to this server via UART (AMA0), and then,
logged on a CSV file (which may be used to automate some stuff later on).
By now, RPi's job is to log and (may be) upload real-time data to the Cloud

This script will be launched as a daemon (init.d) and will notify
the uC when system is ready to start logging data.
"""