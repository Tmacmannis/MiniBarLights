#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include <esp_arduino_version.h>
#include <credentials.h>

#ifdef ssid
#undef ssid
#endif
#ifdef password
#undef password
#endif

#include "EspMQTTClient.h"
#include "OTA.h"

// Forward declarations
void multiScanning();
void rainbow();
void sinelon();
CHSV hsv2rgb();
void setBarSign();
void Task1code(void* pvParameters);
void sendHomeAssistantDiscovery();
void publishAllStates();
String getCurrentEffectState();
void solidColors(boolean upperOnly);
void slowChange();
void singleScanning();

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

volatile boolean lightsOn = true;
int prevUpperBrightness = 100;
int prevLowerBrightness = 100;
int brightness = 25;
volatile int upperBrightness = 100;
volatile int lowerBrightness = 100;
unsigned long mqttUpdateTime = 0;
volatile int currentState = 0;
volatile int masterSpeed = 50;
volatile boolean staticColor = true;

volatile boolean barSignOn = true;
volatile int barSignBrightness = 1024;
int originalBarSignBrightness = 255;

uint8_t changingHue = 0;

boolean colorTranslated = false;

volatile int red = 255;
volatile int green = 255;
volatile int blue = 255;

int testCount = 0;

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
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcAttach(LED, freq, resolution);
#else
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(LED, ledChannel);
#endif
    setBarSign();

    TelnetStream.begin();

    client.setMaxPacketSize(1024);
    client.enableDebuggingMessages(true);
    client.enableLastWillMessage("minibarlights/status", "offline", true);

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
    if (lightsOn) {
        EVERY_N_MILLISECONDS(15) {
            ledStateMachine();
        }
        EVERY_N_MILLISECONDS(20) {
            FastLED.show();
        }
    } else {
        delay(10);
    }

    EVERY_N_MILLISECONDS(50) {
        setBarSign();
    }
}

void setBarSign() {
    static int lastSignOutput = -1;
    int target = barSignOn ? barSignBrightness : 0;
    if (target != lastSignOutput) {
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(LED, target);
#else
        ledcWrite(ledChannel, target);
#endif
        lastSignOutput = target;
    }
}

void ledStateMachine() {
    switch (currentState) {
        case 0:  // Solid White / Color
            solidColors(false);
            break;
        case 1:  // Slow Change
            slowChange();
            break;
        case 2:  // Test Pattern / Scanning
            singleScanning();
            break;
        case 3:  // Breathing Multi Color / Sinelon
            sinelon();
            break;
        case 4:  // Scanner
            multiScanning();
            break;
        case 5:  // Rainbow
            rainbow();
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
        default:
            return "Solid White";
    }
}

void solidColors(boolean upperOnly) {
    CHSV upperColor = hsv2rgb();
    upperColor.value = upperBrightness;
    CHSV lowerColor = upperColor;
    lowerColor.value = lowerBrightness;

    for (int i = 0; i < NUM_LEDS; i++) {
        compartmentLeds[i] = upperColor;
        rightGlassLeds[i] = upperColor;
        leftGlassLeds[i] = upperColor;
        if (!upperOnly) {
            wineLeds[i] = lowerColor;
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
