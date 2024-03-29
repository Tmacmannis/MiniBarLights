void Task1code(void* pvParameters) {
    TelnetStream.print("Task1 running on core ");
    TelnetStream.println(xPortGetCoreID());

    for (;;) {
        delay(28);
        client.loop();  // takes 60 micro seconds to complete, fast...
        unsigned long currentMillis = millis();
        if (currentMillis - mqttUpdateTime >= 1000) {
            mqttUpdateTime = currentMillis;
            //brightness
            client.publish("minibarlights/brightnessState", String(brightness));

            client.publish("minibarlights/barSignBrightnessState", String(originalBarSignBrightness));

            //On OFf State
            if (lightsOn) {
                client.publish("minibarlights/OnOffState", "ON");
                client.publish("minibarlights/set_lower_brightness", String(lowerBrightness));
                client.publish("minibarlights/set_upper_brightness", String(upperBrightness));
                client.publish("minibarlights/effectState", getCurrentEffectState());
            } else {
                client.publish("minibarlights/OnOffState", "OFF");
            }

            if (barSignOn) {
                client.publish("minibarlights/barSignOnOffState", "ON");
            } else {
                client.publish("minibarlights/barSignOnOffState", "OFF");
            }            
        }
    }
}

void onConnectionEstablished() {
    client.subscribe("minibarlights/brightness", [](const String& payload) {
        TelnetStream.print("brightness payload is: ");
        TelnetStream.println(payload);
        brightness = map(payload.toInt(), 3, 255, 0, 255);
        lowerBrightness = brightness;
        upperBrightness = brightness;
    });

    client.subscribe("minibarlights/OnOff", [](const String& payload) {
        TelnetStream.print("OnOff payload is: ");
        TelnetStream.println(payload);
        if (payload == "OFF") {
            if (lightsOn) {
                prevLowerBrightness = lowerBrightness;
                prevUpperBrightness = upperBrightness;
                brightness = 0;
                lightsOn = false;
                lowerBrightness = 0;
                upperBrightness = 0;
            }
        } else {
            if (!lightsOn) {
                lightsOn = true;
                lowerBrightness = prevLowerBrightness;
                upperBrightness = prevUpperBrightness;
            }
        }
    });

    client.subscribe("minibarlights/color", [](const String& payload) {
        TelnetStream.print("Color payload is: ");
        TelnetStream.println(payload);

        String rval = getValue(payload, ',', 0);
        String gval = getValue(payload, ',', 1);
        String bval = getValue(payload, ',', 2);

        red = rval.toInt();
        green = gval.toInt();
        blue = bval.toInt();
    });

    client.subscribe("minibarlights/effects", [](const String& payload) {
        TelnetStream.print("effects payload is: ");
        TelnetStream.println(payload);
        setAnimation(payload);
    });

    client.subscribe("minibarlights/speed_slider", [](const String& payload) {
        TelnetStream.print("speed payload is: ");
        TelnetStream.println(payload);
        masterSpeed = payload.toInt();
    });

    client.subscribe("minibarlights/static_color", [](const String& payload) {
        TelnetStream.print("static color payload is: ");
        TelnetStream.println(payload);
        if (payload == "on") {
            staticColor = true;
        } else {
            staticColor = false;
        }
    });

    client.subscribe("minibarlights/upper_brightness", [](const String& payload) {
        TelnetStream.print("upper brightness payload is: ");
        TelnetStream.println(payload);
        upperBrightness = payload.toInt();
    });

    client.subscribe("minibarlights/lower_brightness", [](const String& payload) {
        TelnetStream.print("lower brightness payload is: ");
        TelnetStream.println(payload);
        lowerBrightness = payload.toInt();
    });

    client.subscribe("minibarlights/powerOnAutomation", [](const String& payload) {
        TelnetStream.print("power on called");
        TelnetStream.println(payload);
        lightsOn = true;
        lowerBrightness = 50;
        upperBrightness = 50;
        setAnimation("Solid White");
    });

    client.subscribe("minibarlights/barSignOnOff", [](const String& payload) {
        TelnetStream.print("bar sign on off");
        TelnetStream.println(payload);
        if (payload == "ON") {
            barSignOn = true;
        } else {
            barSignOn = false;
        }
    });

    client.subscribe("minibarlights/barSignBrightness", [](const String& payload) {
        TelnetStream.print("bar sign brightness");
        TelnetStream.println(payload);
        originalBarSignBrightness = payload.toInt();
        barSignBrightness = map(payload.toInt(), 3, 255, 0, 1024);
    });
}

String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

CHSV hsv2rgb() {
    CRGB color1temp;
    color1temp.r = red;
    color1temp.g = green;
    color1temp.b = blue;
    CHSV temp1 = rgb2hsv_approximate(color1temp);
    return temp1;
}