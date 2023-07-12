import random as rd
import time

from numpy import random

from paho.mqtt import client as mqtt_client
import json 

broker = 'localhost'
port = 1883
client_id = f'python-data-generator-{random.randint(0, 1000)}'
username = 'mqttUser'
password = 'mqttUser'

topics = "iot/air", "iot/water", "iot/status"

def connect_mqtt():
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(client_id)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def publish(client):
    while True:

        air_msg = generate_air_data()
        water_msg = generate_water_data()
        status_msg = generate_status_data()

        for msg in status_msg:
            result = client.publish(topics[2], json.dumps(msg))
            if result[0] == 0:
                print(f"[`iot/status`] -> `{json.dumps(msg)}` \n")
            else:
                print(f"Failed to send message to topic iot/status")


        for air, water in zip(air_msg, water_msg):
            msg = [air, water]
            result = client.publish(topics[0], json.dumps(msg[0])), client.publish(topics[1], json.dumps(msg[1]))

            for result, msg, topic in zip(result, msg, topics):
                if result[0] == 0:
                    print(f"[`{topic}`] -> `{json.dumps(msg)}` \n")
                else:
                    print(f"Failed to send message to topic {topic}")

            print("="*156)

        time.sleep(5)

def generate_air_data():
    
    name = ["North Hall", "South Hall", "East Hall", "West Hall"]
    
    air_data = [None] * len(name)

    for i, name in enumerate(name):
        # Generate random data
        temp = random.normal(24, 2)
        humidity = random.normal(40, 5)
        CO = max(random.normal(75, 50), 0)
        movement = rd.choice([0, 1, 2, 3, 4])

        # Create json object
        air_data[i] = {
            "node_type": "air",
            "node_name": name,
            "data": {
                "temp": temp,
                "humidity": humidity,
                "CO": CO,
                "movement": movement
            }
        }

    return air_data

def generate_water_data():
    
    name = ["Pacific Tank", "Atlantic Tank", "Indian Tank", "Southern Tank"]
    water_data  = [None] * len(name)


    for i, name in enumerate(name):
        # Generate random data
        # temp = random.normal(30, 2)
        temp = random.normal(20, 2)
        pH = random.normal(7, 1)

        # Create json object
        water_data[i] = {
            "node_type": "water",
            "node_name": name,
            "data": {
                "temp": temp,
                "ph": pH
            }
        }

    return water_data


def generate_status_data():
    
    nodeName = [["North Hall", "South Hall", "East Hall", "West Hall"],
                ["Pacific Tank", "Atlantic Tank", "Indian Tank", "Southern Tank"]]
    
    nodeType = ["air", "water"]

    status_data = [None] *(len(nodeName) * len(nodeName[0]))

    for i, type in enumerate(nodeType):
        for j, name in enumerate(nodeName[i]):
            print(name)
            temp = random.normal(24, 2)
            battery = random.normal(3200, 150)
            status_data[(i * 4) + j] = {
                "node_type": type,
                "node_name": name,
                "data": {
                    "temp": temp,
                    "batt": battery
                }
            }
        
    return status_data


def run():
    client = connect_mqtt()
    client.loop_start()
    publish(client)
    client.loop_stop()

if __name__ == '__main__':
    run()
