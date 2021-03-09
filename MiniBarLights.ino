#include <credentials.h>
#include "EspMQTTClient.h"
#include "OTA.h"

EspMQTTClient client(
    mySSID,
    myPASSWORD,
    mqttIP,      // MQTT Broker server ip
    "tim",       // Can be omitted if not needed
    "14Q4YsC6YrXl",   // Can be omitted if not needed
    "MiniBar",  // Client name that uniquely identify your device
    haPORT       // The MQTT port, default to 1883. this line can be omitted
);

TaskHandle_t Task1;

void setup() {
    setupOTA("MiniBarLights", mySSID, myPASSWORD);
    TelnetStream.begin();

    client.enableDebuggingMessages();                                           // Enable debugging messages sent to serial output
    client.enableHTTPWebUpdater();                                              // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overrited with enableHTTPWebUpdater("user", "password").
    client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
    client.enableDebuggingMessages(false);

    xTaskCreatePinnedToCore(
        Task1code, /* Task function. */
        "Task1",   /* name of task. */
        10000,     /* Stack size of task */
        NULL,      /* parameter of the task */
        1,         /* priority of the task */
        &Task1,    /* Task handle to keep track of created task */
        0);        /* pin task to core 0 */
    delay(500);
}

void loop() {
    handleClientTest();
}


void Task1code(void* pvParameters) {
    TelnetStream.print("Task1 running on core ");
    TelnetStream.println(xPortGetCoreID());

    for (;;) {
        delay(28);
        //ArduinoOTA.handle();
        // server.handleClient();
        // handleClientTest();
        client.loop();  // takes 60 micro seconds to complete, fast...

    }
}


void onConnectionEstablished() {
    // Subscribe to "mytopic/test" and display received message to Serial
    client.subscribe("minibarlights/test", [](const String& payload) {
        TelnetStream.print("payload is: ");
        TelnetStream.println(payload);
        TelnetStream.print("sub task running on core ");
        TelnetStream.println(xPortGetCoreID());
    });
}