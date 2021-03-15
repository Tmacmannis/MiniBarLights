#include <FastLED.h>
#include <credentials.h>

#include "EspMQTTClient.h"
#include "OTA.h"

#define DATA_PIN1 12
#define DATA_PIN2 16
#define DATA_PIN3 17
#define DATA_PIN4 18

#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 20

CRGB leds1[NUM_LEDS];
CRGB leds2[NUM_LEDS];
CRGB leds3[NUM_LEDS];
CRGB leds4[NUM_LEDS];

boolean lightsOn = true;
int prevBrightness = 0;
int brightness = 25;
unsigned long mqttUpdateTime = 0;
int currentState;

uint8_t changingHue = 0;

boolean colorTranslated = false;

int red;
int green;
int blue;

EspMQTTClient client(
    mySSID,
    myPASSWORD,
    mqttIP,          // MQTT Broker server ip
    "tim",           // Can be omitted if not needed
    "14Q4YsC6YrXl",  // Can be omitted if not needed
    "MiniBar",       // Client name that uniquely identify your device
    haPORT           // The MQTT port, default to 1883. this line can be omitted
);

TaskHandle_t Task1;

void setup() {
    setupOTA("MiniBarLights", mySSID, myPASSWORD);
    TelnetStream.begin();

    client.enableDebuggingMessages();                                           // Enable debugging messages sent to serial output
    client.enableHTTPWebUpdater();                                              // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overrited with enableHTTPWebUpdater("user", "password").
    client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
    client.enableDebuggingMessages(true);

    xTaskCreatePinnedToCore(
        Task1code, /* Task function. */
        "Task1",   /* name of task. */
        10000,     /* Stack size of task */
        NULL,      /* parameter of the task */
        1,         /* priority of the task */
        &Task1,    /* Task handle to keep track of created task */
        0);        /* pin task to core 0 */
    delay(500);

    FastLED.addLeds<LED_TYPE, DATA_PIN1, COLOR_ORDER>(leds1, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, DATA_PIN2, COLOR_ORDER>(leds2, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, DATA_PIN3, COLOR_ORDER>(leds3, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, DATA_PIN4, COLOR_ORDER>(leds4, NUM_LEDS).setCorrection(TypicalLEDStrip);

    FastLED.setBrightness(brightness);
}

void loop() {
    handleClientTest();

    EVERY_N_MILLISECONDS(16) {
        ledStateMachine();
        FastLED.show();
    }
}

void ledStateMachine() {
    switch (currentState) {
        case 0:  //solid white
            for (int i = 0; i < 20; i++) {
                leds1[i].setRGB(red, green, blue);
                leds2[i].setRGB(red, green, blue);
                leds3[i].setRGB(red, green, blue);
                leds4[i].setRGB(red, green, blue);
            }
            break;
        case 1:  //Slow Change
            for (int i = 0; i < 20; i++) {
                leds1[i] = CHSV(changingHue, 255, brightness);
                leds2[i] = CHSV(changingHue, 255, brightness);
                leds3[i] = CHSV(changingHue, 255, brightness);
                leds4[i] = CHSV(changingHue, 255, brightness);
            }
            EVERY_N_MILLISECONDS(500){
                changingHue++;
            }
            break;
        case 2: //Test Pattern

            break;
            
    }
}


void setAnimation(String payload) {
    if (payload == "Solid White") {
        currentState = 0;
        red = 255;
        green = 255;
        blue = 255;
    }
    if (payload == "Slow Change") {
        currentState = 1;
    }
    if (payload == "Scanning") {
        currentState = 2;
    }
    if (payload == "Breathing Multi Color") {
        currentState = 3;
    }
    if (payload == "Scanner") {
        currentState = 4;
    }
    if (payload == "Rainbow") {
        currentState = 5;
    }
}

