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
#define LED 25

CRGB wineLeds[NUM_LEDS];
CRGB compartmentLeds[NUM_LEDS];
CRGB rightGlassLeds[NUM_LEDS];
CRGB leftGlassLeds[NUM_LEDS];

// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 10;  //Resolution 8, 10, 12, 15

boolean lightsOn = true;
int prevUpperBrightness = 0;
int prevLowerBrightness = 0;
int brightness = 25;
int upperBrightness = 100;
int lowerBrightness = 100;
unsigned long mqttUpdateTime = 0;
int currentState;
int masterSpeed = 50;
boolean staticColor = true;

boolean barSignOn = true;
int barSignBrightness = 1024;
int originalBarSignBrightness;

uint8_t changingHue = 0;

boolean colorTranslated = false;

int red;
int green;
int blue;

int testCount;

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

    pinMode(LED, OUTPUT);
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(LED, ledChannel);

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

    FastLED.addLeds<LED_TYPE, DATA_PIN1, COLOR_ORDER>(wineLeds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, DATA_PIN2, COLOR_ORDER>(compartmentLeds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, DATA_PIN3, COLOR_ORDER>(rightGlassLeds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, DATA_PIN4, COLOR_ORDER>(leftGlassLeds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    FastLED.setBrightness(255);
}

void loop() {
    handleClientTest();

    EVERY_N_MILLISECONDS(8) {
        ledStateMachine();
    }
    FastLED.show();

    EVERY_N_MILLISECONDS(8) {
        setBarSign();
    }

    // EVERY_N_MILLISECONDS(5000) {
    //     if (barSignOn) {
    //         ledcWrite(ledChannel, 0);
    //     } else {
    //         ledcWrite(ledChannel, 1024);
    //     }
    //     barSignOn = !barSignOn;
    // }
}

void setBarSign() {
    if (barSignOn) {
        ledcWrite(ledChannel, barSignBrightness);
    } else {
        ledcWrite(ledChannel, 0);
    }
}

void ledStateMachine() {
    switch (currentState) {
        case 0:  //solid white
            solidColors(false);
            break;
        case 1:  //Slow Change
            slowChange();
            break;
        case 2:  //Test Pattern
            singleScanning();
            break;
        case 3:
            sinelon();
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

String getCurrentEffectState() {
    switch (currentState) {
        case 0:
            return "Solid White";
        case 1:
            return "Slow Change";
        case 2:
            return "Scanning";
        case 3:
            return "Breathing Multi Color";
        case 4:
            return "Scanner";
        case 5:
            return "Rainbow";
    }
}

CHSV rgb2hsv() {
    CRGB color1temp;
    color1temp.r = red;
    color1temp.g = green;
    color1temp.b = blue;
    CHSV temp1 = rgb2hsv_approximate(color1temp);
    return temp1;
}

void solidColors(boolean upperOnly) {
    for (int i = 0; i < 20; i++) {
        CHSV temp1 = hsv2rgb();
        temp1.value = upperBrightness;
        compartmentLeds[i] = temp1;
        rightGlassLeds[i] = temp1;
        leftGlassLeds[i] = temp1;
        if (!upperOnly) {
            temp1.value = lowerBrightness;
            wineLeds[i] = temp1;
        }
    }
}

void slowChange() {
    for (int i = 0; i < 20; i++) {
        wineLeds[i] = CHSV(changingHue, 255, lowerBrightness);
        compartmentLeds[i] = CHSV(changingHue, 255, upperBrightness);
        rightGlassLeds[i] = CHSV(changingHue, 255, upperBrightness);
        leftGlassLeds[i] = CHSV(changingHue, 255, upperBrightness);
    }
    EVERY_N_MILLISECONDS_I(timingObj2, 500) {
        changingHue++;
        timingObj2.setPeriod(map(masterSpeed, 0, 100, 1000, 3));
    }
}

void singleScanning() {
    EVERY_N_MILLISECONDS_I(timingObj, 200) {
        if (staticColor) {
            CHSV temp1 = hsv2rgb();
            temp1.value = lowerBrightness;
            wineLeds[testCount] = temp1;
        } else {
            wineLeds[testCount] = CHSV(changingHue, 255, lowerBrightness);
        }

        testCount++;
        timingObj.setPeriod(map(masterSpeed, 0, 100, 1000, 50));
    }

    EVERY_N_MILLISECONDS(16) {
        fadeToBlackBy(wineLeds, NUM_LEDS, map(masterSpeed, 0, 100, 2, 30));
    }

    if (testCount > 14) {
        testCount = 0;
        if (!staticColor) {
            changingHue = random8();
        }
    }

    solidColors(true);
}
