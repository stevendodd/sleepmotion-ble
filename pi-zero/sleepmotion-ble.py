import sys
import time
import logging
from flask import Flask, logging as flog
from flask import request
from bluepy import btle


app = Flask(__name__)
logging.basicConfig(level=logging.INFO)

def connect(mac: str = None, uuid: str = None):    
    if mac is None:
        mac = "57:4c:54:2c:c6:31"
        
    if uuid is None:
        uuid = "fee9"
        
    global bed
    global c
    global app
    
    connected = False
    while not connected:
        try:
            app.logger.info("Connecting...")
            bed = btle.Peripheral(mac)
            connected = True
        except btle.BTLEDisconnectError as err:
            time.sleep(3)
            app.logger.error(err)

    app.logger.info("Discovering Services...")
    app.logger.info(bed.services)
    service = bed.getServiceByUUID(uuid)

    app.logger.info("Discovering Characteristics...")
    c = service.getCharacteristics()[0]

def getConnection():
    global app
    try:
        bed.getState()
    except btle.BTLEInternalError as err:
        app.logger.info("Reconnecting")
        connect()

bed = None
c = None
connect()

"""
6e01003cab - light
6e010045b4 - zg
6e010031a0 - flat
6e01002493 head up
6e01002594 head down
6e01002695 feet up
6e01002796 feet down
"""

@app.route("/zerog",methods = ['POST', 'GET'])
def zerog():
    if request.method == 'POST':
      getConnection()
      c.write(bytearray.fromhex("6e010045b4"))
    return "done"

@app.route("/flat",methods = ['POST', 'GET'])
def flat():
    if request.method == 'POST':
      getConnection()
      c.write(bytearray.fromhex("6e010031a0"))
    return "done"

@app.route("/light",methods = ['POST', 'GET'])
def light():
    if request.method == 'POST':
      getConnection()
      c.write(bytearray.fromhex("6e01003cab"))
    return "done"

@app.route("/situp",methods = ['POST', 'GET'])
def situp():
    if request.method == 'POST':
        getConnection()
        hold = 32
        while hold >= 0:
            c.write(bytearray.fromhex("6e01002695"))
            time.sleep(0.25)
            hold -= 1
            
        hold = 90
        while hold >= 0:
            c.write(bytearray.fromhex("6e01002493"))
            time.sleep(0.25)
            hold -= 1
        
    return "done"

@app.route("/feetdown",methods = ['POST', 'GET']) 
def feetDown():
    if request.method == 'POST':
        getConnection()
        hold = 32
        while hold >= 0:
            c.write(bytearray.fromhex("6e01002796"))
            time.sleep(0.25)
            hold -= 1
    return "done"