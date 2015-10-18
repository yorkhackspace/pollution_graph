#!/usr/bin/env python3
"""
Python half of the pollution monitor.

This bit is the brains, such that they are. The arduino
will be pretty dumb just driving the lights and display
and also reading the rotary encoders.

Arduino sends

Lnn - Where nn is a number from 00 to 99 for locations.
Tnn - Where nn is a number for the hours.

Python sends

Vnn - Where nn is a number from 00 to 25 for the LED value

"""

from test_data import *
import serial
import logging

class Arduino():
    def __init__(port, data_values):
        self.port == port
        self.data_values = data_values
        self.arduino = open_arduino(port)
        self.currentTime = 8
        self.currentLocation = 1

    def write(self, message):
        """Writes a simple string to the arduino"""
        err = none
        debug("Sending {}".format(message))
        self.arduino.writeline(b(message))
        return err

    def arduino_read(self):
        msg = arduino.readline()
        if msg.startswith("T"):
            self.process_time(self, msg)
        elif msg.startswith("L"):
            process_location(msg)
        else:
            error("Unknown message: {}".format(msg))

    def process_time(self, msg):
        global current_location
        value, err = split_message(msg)
        if err != nil and value < 24 and value >= 0:
            debug("Recived temperature: {}".format(value))
            current_location["time"] = value
            new_gas_level = get_gas_value()
            self.arduino.write("L{}".format(new_gas_level))
        else:
            error("Recived duff temperature:'{}'".format(msg))

def open_arduino(port):
    """Just connect to the serial port for now
    there should probably be some more error checking!"""
    arduino = serial.Serial(port, 57600)
    # Wait for it to come alive. Assume you send a hello
    # string or some such at boot and it will reboot on reconnect
    firstline = audiono.readline()
    logging.debug("Arduino Starting... {} ".format(firstline))
    return arduino


def split_message(msg):
    """Assumes Letter followed by a number"""
    err = False
    value = int(msg[1:2])
    return value, err


def get_gas_value():
    """Return the gas value at the current location
    For now we are just doing NO2"""
    global source_data
    global current_location
    global locations
    gas = NO2
    location_data = [ x for x in source_data if x['location'] == locations[current_location["location"]] ]
    location_data_for_gas = [(x['date'], x['value']) for x in location_data if x['gas'] == 'NO2']
    # TODO need to actually work here. Just fudge it for now
    return location_data_for_gas[0][1]




def run_server():
    """The main thread that runs the server"""
    arduino = Arduino(port = '/dev/ttyUSBO')
    arduino.set_temperature()
    while True:
        arduino.read(arduino)


if __name__ == "__main__":
    # This is going to be global, so things
    # can read from it when required.
    source_data = extract_csv_to_list('input.csv')
    current_location = {
        'hour': 8,
        'location': 1,
    }
    # Get this from the data above.
    locations = ['Bootham Row', 'Gillygate']
    run_server()
