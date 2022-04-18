#sauce https://iotbytes.wordpress.com/store-mqtt-data-from-sensors-into-sql-database/

import paho.mqtt.client as mqtt
from dbHandler import dataHandler

# MQTT Parameters
broker = "34.101.49.52"
port = 1883
keepAliveInterval = 45
masterTopic = "sys/#"

#sys/charger1/s1
#sys/charger2/s1
#sys/chargerX/s1

# Subscribre to topic
def on_connect(mosq, obj, rc, properties=None):
	mqttc.subscribe(masterTopic, 0)

# Save to DB table
def on_message(mosq, obj, msg):
	# This is the Master Call for saving MQTT Data into DB 
	print ("MQTT Data Received...")
	print ("MQTT Topic: " + msg.topic  )
	print ("Data: ",end='')
	print (msg.payload)
	dataHandler(msg.topic, msg.payload)

def on_subscribe(mosq, obj, mid, granted_qos):
    pass

mqttc = mqtt.Client()

# Assign event callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_subscribe = on_subscribe

# Connect
mqttc.connect(broker, int(port), int(keepAliveInterval))

# Continue the network loop
mqttc.loop_forever()
