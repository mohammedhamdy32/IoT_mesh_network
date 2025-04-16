import paho.mqtt.client as mqtt
import random

c = 0
broker = "localhost"

data_topic    = "home/data"
esp_control_topic = "home/esp_control"
rass_control_topic = "home/rass_control"

def on_message(client, userdata, message):

    print(f"Received message: {message.payload.decode()} on topic {message.topic}")

    if message.payload.decode().startswith('Node'):
       # Extract data form message
       node_part, led_part = message.payload.decode().split(',')
       node_number = node_part.split(':')[0].replace('Node', '')  # 2
       value = node_part.split(':')[1]                            # 0.00
       led_status = led_part.split(':')[1]                        # OFF
       print(f"Node Number: {node_number}, Value: {value}, LED Status: {led_status}")

    # Put data in database
    ###

    # Send data
    c = random.randint(0, 1)
    #if c == 0:
    #    client.publish(rass_control_topic,"Node2:LED:OFF\n")
    #else:    
    #    client.publish(rass_control_topic,"Node2:LED:OFF\n")
    
    print("------------------------------------------")

c = 0
client = mqtt.Client()
client.connect(broker, 1883)
client.subscribe(data_topic)
client.subscribe(esp_control_topic)
client.on_message = on_message

print("Waiting for messages...")
client.loop_forever()


