#include <painlessMesh.h>

#define MESH_PREFIX "mesh-network"
#define MESH_PASSWORD "meshpassword"
#define MESH_PORT 5555
#define POT_PIN 34
#define LED_SERVO_PIN  2  
#define RX_PIN 16
#define TX_PIN 17

#define SEND_TO_BRIDGE(msg)  Serial1.println(msg)

painlessMesh mesh;
bool ledState = false;
bool manualOverride = false;
String node1Data = "";

void sendMessageTask(void *parameter) 
{
  for (;;) 
  {
    int potValue = analogRead(POT_PIN);
    float level = (potValue / 1023.0) * 100;

    if (!manualOverride) 
    {
      if (level > 70) 
      {
        digitalWrite(LED_SERVO_PIN, HIGH);
        ledState = true;
      } else 
      {
        digitalWrite(LED_SERVO_PIN, LOW);
        ledState = false;
      }
    }

    // Broadcast sensor reading and LED status
    String node2Data = "Node2:" + String(level) + ",LED:" + (ledState ? "ON" : "OFF");
    mesh.sendBroadcast(node2Data);
    Serial.println("Broadcast: " + node2Data);

    // Send to bridge
    SEND_TO_BRIDGE(node2Data);
    Serial.println("Sent to Bridge: " + node2Data);

    vTaskDelay(pdMS_TO_TICKS(5000));  // Delay for 5 seconds
  }
}

/* If a node in mesh sends a data, pass it to bridge */
void receivedCallback(uint32_t from, String &msg) 
{
  node1Data = msg;
  Serial.println("Received from " + String(from) + ": " + msg);
  // Send to bridge
  SEND_TO_BRIDGE(node1Data);

}

void setup() 
{
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);  // UART1 on custom pins

  pinMode(POT_PIN, INPUT);
  pinMode(LED_SERVO_PIN, OUTPUT);
  digitalWrite(LED_SERVO_PIN, LOW);

  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);

  // Create FreeRTOS task on Core 1 with high priority (3)
  xTaskCreatePinnedToCore(
    sendMessageTask,   // Task function
    "SendMessageTask", // Task name
    4096,              // Stack size
    NULL,              // Parameter
    3,                 // Priority (higher = more priority)
    NULL,              // Task handle
    1                  // Core 1
  );
}

void loop() 
{
  mesh.update();

  if (Serial1.available()) 
  {
    String command = Serial1.readStringUntil('\n');
    command.trim( );
    //command.toLowerCase();
    
    Serial.println("Received from Bridge:" + command);

    if( command.endsWith("LED:OFF ") || command.endsWith("LED:OFF \n") )  
    {
      ledState = false;
      manualOverride = true;
      digitalWrite(LED_SERVO_PIN, LOW);
      Serial.println("LED OFF");
    }else if( command.endsWith("LED:ON") || command.endsWith("LED:ON\n") ) 
    {
      ledState = true;
      manualOverride = true;
      digitalWrite(LED_SERVO_PIN, HIGH);
      Serial.println("LED ON");
    } else 
    {
      Serial.println("Unknown");
    }
  }
}
