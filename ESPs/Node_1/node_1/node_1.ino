#include <painlessMesh.h>

#define MESH_PREFIX "mesh-network"
#define MESH_PASSWORD "meshpassword"
#define MESH_PORT 5555
#define POT_PIN   23
#define LED_PIN   2 

Scheduler userScheduler;
painlessMesh mesh;
bool ledState = false;
bool manualOverride = false;

void sendMessage() {
    int potValue = analogRead(POT_PIN);
    float level = (potValue / 1023.0) * 100;

    if (!manualOverride) {
        if (level > 70) {
            digitalWrite(LED_PIN, HIGH);
            ledState = true;
        } else {
            digitalWrite(LED_PIN, LOW);
            ledState = false;
        }
    }

    String msg = "Node1,Pot:" + String(level) + ",LED:" + (ledState ? "ON" : "OFF");
    mesh.sendBroadcast(msg);
    Serial.println("Broadcast: " + msg);
}

Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage);

void receivedCallback(uint32_t from, String &msg) {
    msg.trim();
    msg.toLowerCase();
    Serial.println("Received from " + String(from) + ": " + msg);
    if (msg == "onl" || msg == "node1:led:on") {
        digitalWrite(LED_PIN, HIGH);
        ledState = true;
        manualOverride = true;
        mesh.sendSingle(from, "Node1:ACK:LED_ON");
        Serial.println("LED turned ON");
    } else if (msg == "offl" || msg == "node1:led:off") {
        digitalWrite(LED_PIN, LOW);
        ledState = false;
        manualOverride = true;
        mesh.sendSingle(from, "Node1:ACK:LED_OFF");
        Serial.println("LED turned OFF");
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    pinMode(POT_PIN, INPUT);

    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    userScheduler.addTask(taskSendMessage);
    taskSendMessage.enable();
}

void loop() {
    mesh.update();
}
