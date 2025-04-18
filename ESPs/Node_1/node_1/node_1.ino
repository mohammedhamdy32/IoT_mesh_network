#include <painlessMesh.h>

#define MESH_PREFIX "mesh-network"
#define MESH_PASSWORD "meshpassword"
#define MESH_PORT 5555
#define POT_PIN   34
#define LED_PIN   2 
#include <ArduinoJson.h>
StaticJsonDocument<200> doc;

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

    String msg = "1{\"temperature\": " + String(level, 1) + 
                ", \"humidity\": 65.2" +
                ", \"light\": 350" +
                ", \"pressure\": 1013.2}";
    mesh.sendBroadcast(msg);
    Serial.println("Broadcast: " + msg);
}

Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage);

void receivedCallback(uint32_t from, String &msg) {
    msg.trim();
    Serial.println("Received from " + String(from) + ": " + msg);
    String jsonPart = msg.substring(1);

    DeserializationError error = deserializeJson( doc, jsonPart );
    float value = doc["value"];
    Serial.println(value);

    /* For node 2 */
    if( msg.startsWith("1") ) 
    {

      /* LED ON */
      if( value > 90 )
      {
        ledState = true;
        manualOverride = true;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("LED ON");
      }else
      {
        ledState = false;
        manualOverride = true;
        digitalWrite(LED_PIN, LOW);
        Serial.println("LED OFF");
      }

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
