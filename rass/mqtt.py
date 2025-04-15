import paho.mqtt.client as mqtt

broker = "localhost"

data_topic    = "home/data"
esp_control_topic = "home/esp_control"
rass_control_topic = "home/rass_control"

def on_message(client, userdata, message):

    print(f"Received message: {message.payload.decode()} on topic {message.topic}")
    # Put data in database
    ###

    # Send data
    client.publish(rass_control_topic,"ON")

client = mqtt.Client()
client.connect(broker, 1883)
client.subscribe(data_topic)
client.subscribe(esp_control_topic)
client.on_message = on_message

print("Waiting for messages...")
client.loop_forever()


