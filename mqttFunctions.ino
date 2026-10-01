void sendHomeAssistantDiscovery() {
    const char* devJson = "{\"ids\":[\"minibar_esp32\"],\"name\":\"Mini Bar\",\"mdl\":\"ESP32 Mini Bar Controller\",\"mf\":\"Tim\",\"sw\":\"2.0\"}";

    // 1. Main Lights (On/Off, Brightness, RGB Color, Effects)
    String lightPayload = String("{\"name\":\"Lights\",\"uniq_id\":\"minibar_main_lights\",\"cmd_t\":\"minibarlights/OnOff\",\"stat_t\":\"minibarlights/OnOffState\",\"bri_cmd_t\":\"minibarlights/brightness\",\"bri_stat_t\":\"minibarlights/brightnessState\",\"bri_scl\":255,\"rgb_cmd_t\":\"minibarlights/color\",\"rgb_stat_t\":\"minibarlights/colorState\",\"fx_cmd_t\":\"minibarlights/effects\",\"fx_stat_t\":\"minibarlights/effectState\",\"fx_list\":[\"Solid White\",\"Slow Change\",\"Scanning\",\"Breathing Multi Color\",\"Scanner\",\"Rainbow\"],\"avty_t\":\"minibarlights/status\",\"dev\":") + devJson + "}";
    client.publish("homeassistant/light/minibar/lights/config", lightPayload, true);

    // 2. Bar Sign (On/Off, Brightness)
    String signPayload = String("{\"name\":\"Bar Sign\",\"uniq_id\":\"minibar_sign_light\",\"cmd_t\":\"minibarlights/barSignOnOff\",\"stat_t\":\"minibarlights/barSignOnOffState\",\"bri_cmd_t\":\"minibarlights/barSignBrightness\",\"bri_stat_t\":\"minibarlights/barSignBrightnessState\",\"bri_scl\":255,\"avty_t\":\"minibarlights/status\",\"dev\":") + devJson + "}";
    client.publish("homeassistant/light/minibar/sign/config", signPayload, true);

    // 3. Upper Brightness Slider
    String upperPayload = String("{\"name\":\"Upper Brightness\",\"uniq_id\":\"minibar_upper_brightness\",\"cmd_t\":\"minibarlights/upper_brightness\",\"stat_t\":\"minibarlights/set_upper_brightness\",\"min\":0,\"max\":255,\"step\":1,\"avty_t\":\"minibarlights/status\",\"dev\":") + devJson + "}";
    client.publish("homeassistant/number/minibar/upper_brightness/config", upperPayload, true);

    // 4. Lower Brightness Slider
    String lowerPayload = String("{\"name\":\"Lower Brightness\",\"uniq_id\":\"minibar_lower_brightness\",\"cmd_t\":\"minibarlights/lower_brightness\",\"stat_t\":\"minibarlights/set_lower_brightness\",\"min\":0,\"max\":255,\"step\":1,\"avty_t\":\"minibarlights/status\",\"dev\":") + devJson + "}";
    client.publish("homeassistant/number/minibar/lower_brightness/config", lowerPayload, true);

    // 5. Animation Speed Slider
    String speedPayload = String("{\"name\":\"Animation Speed\",\"uniq_id\":\"minibar_speed\",\"cmd_t\":\"minibarlights/speed_slider\",\"stat_t\":\"minibarlights/speedState\",\"min\":0,\"max\":100,\"step\":1,\"avty_t\":\"minibarlights/status\",\"dev\":") + devJson + "}";
    client.publish("homeassistant/number/minibar/speed/config", speedPayload, true);

    // 6. Static Color Switch
    String switchPayload = String("{\"name\":\"Static Color\",\"uniq_id\":\"minibar_static_color\",\"cmd_t\":\"minibarlights/static_color\",\"stat_t\":\"minibarlights/staticColorState\",\"pl_on\":\"on\",\"pl_off\":\"off\",\"avty_t\":\"minibarlights/status\",\"dev\":") + devJson + "}";
    client.publish("homeassistant/switch/minibar/static_color/config", switchPayload, true);

    // 7. Power On Preset Button
    String buttonPayload = String("{\"name\":\"Power On Preset\",\"uniq_id\":\"minibar_power_on\",\"cmd_t\":\"minibarlights/powerOnAutomation\",\"pl_prs\":\"PRESS\",\"avty_t\":\"minibarlights/status\",\"dev\":") + devJson + "}";
    client.publish("homeassistant/button/minibar/power_on/config", buttonPayload, true);
}

void publishAllStates() {
    client.publish("minibarlights/status", "online", true);
    client.publish("minibarlights/brightnessState", String(brightness));
    client.publish("minibarlights/barSignBrightnessState", String(originalBarSignBrightness));
    client.publish("minibarlights/OnOffState", lightsOn ? "ON" : "OFF");
    client.publish("minibarlights/barSignOnOffState", barSignOn ? "ON" : "OFF");
    client.publish("minibarlights/colorState", String(red) + "," + String(green) + "," + String(blue));
    client.publish("minibarlights/speedState", String(masterSpeed));
    client.publish("minibarlights/staticColorState", staticColor ? "on" : "off");
    client.publish("minibarlights/set_lower_brightness", String(lowerBrightness));
    client.publish("minibarlights/set_upper_brightness", String(upperBrightness));
    client.publish("minibarlights/effectState", getCurrentEffectState());
}

void Task1code(void* pvParameters) {
    TelnetStream.print("Task1 running on core ");
    TelnetStream.println(xPortGetCoreID());

    for (;;) {
        client.loop();
        server.handleClient();

        unsigned long currentMillis = millis();
        // Heartbeat publish every 30 seconds instead of 1-second spam
        if (currentMillis - mqttUpdateTime >= 30000) {
            mqttUpdateTime = currentMillis;
            if (client.isConnected()) {
                publishAllStates();
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void onConnectionEstablished() {
    client.publish("minibarlights/status", "online", true);
    sendHomeAssistantDiscovery();
    publishAllStates();

    client.subscribe("minibarlights/brightness", [](const String& payload) {
        TelnetStream.print("brightness payload is: ");
        TelnetStream.println(payload);
        int val = payload.toInt();
        brightness = map(val, 3, 255, 0, 255);
        if (brightness < 0) brightness = 0;
        lowerBrightness = brightness;
        upperBrightness = brightness;
        client.publish("minibarlights/brightnessState", String(brightness));
        client.publish("minibarlights/set_lower_brightness", String(lowerBrightness));
        client.publish("minibarlights/set_upper_brightness", String(upperBrightness));
    });

    client.subscribe("minibarlights/OnOff", [](const String& payload) {
        TelnetStream.print("OnOff payload is: ");
        TelnetStream.println(payload);
        if (payload.equalsIgnoreCase("OFF")) {
            if (lightsOn) {
                if (lowerBrightness > 0) prevLowerBrightness = lowerBrightness;
                if (upperBrightness > 0) prevUpperBrightness = upperBrightness;
                lightsOn = false;
                FastLED.clear(true);
            }
        } else {
            lightsOn = true;
            lowerBrightness = (prevLowerBrightness > 0) ? prevLowerBrightness : 100;
            upperBrightness = (prevUpperBrightness > 0) ? prevUpperBrightness : 100;
            brightness = upperBrightness;
        }
        client.publish("minibarlights/OnOffState", lightsOn ? "ON" : "OFF");
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
        client.publish("minibarlights/colorState", String(red) + "," + String(green) + "," + String(blue));
    });

    client.subscribe("minibarlights/effects", [](const String& payload) {
        TelnetStream.print("effects payload is: ");
        TelnetStream.println(payload);
        setAnimation(payload);
        client.publish("minibarlights/effectState", getCurrentEffectState());
    });

    client.subscribe("minibarlights/speed_slider", [](const String& payload) {
        TelnetStream.print("speed payload is: ");
        TelnetStream.println(payload);
        masterSpeed = payload.toInt();
        client.publish("minibarlights/speedState", String(masterSpeed));
    });

    client.subscribe("minibarlights/static_color", [](const String& payload) {
        TelnetStream.print("static color payload is: ");
        TelnetStream.println(payload);
        staticColor = payload.equalsIgnoreCase("on");
        client.publish("minibarlights/staticColorState", staticColor ? "on" : "off");
    });

    client.subscribe("minibarlights/upper_brightness", [](const String& payload) {
        TelnetStream.print("upper brightness payload is: ");
        TelnetStream.println(payload);
        upperBrightness = payload.toInt();
        client.publish("minibarlights/set_upper_brightness", String(upperBrightness));
    });

    client.subscribe("minibarlights/lower_brightness", [](const String& payload) {
        TelnetStream.print("lower brightness payload is: ");
        TelnetStream.println(payload);
        lowerBrightness = payload.toInt();
        client.publish("minibarlights/set_lower_brightness", String(lowerBrightness));
    });

    client.subscribe("minibarlights/powerOnAutomation", [](const String& payload) {
        TelnetStream.print("power on called");
        TelnetStream.println(payload);
        lightsOn = true;
        lowerBrightness = 50;
        upperBrightness = 50;
        brightness = 50;
        setAnimation("Solid White");
        publishAllStates();
    });

    client.subscribe("minibarlights/barSignOnOff", [](const String& payload) {
        TelnetStream.print("bar sign on off");
        TelnetStream.println(payload);
        barSignOn = payload.equalsIgnoreCase("ON");
        setBarSign();
        client.publish("minibarlights/barSignOnOffState", barSignOn ? "ON" : "OFF");
    });

    client.subscribe("minibarlights/barSignBrightness", [](const String& payload) {
        TelnetStream.print("bar sign brightness");
        TelnetStream.println(payload);
        originalBarSignBrightness = payload.toInt();
        barSignBrightness = map(originalBarSignBrightness, 3, 255, 0, 1024);
        setBarSign();
        client.publish("minibarlights/barSignBrightnessState", String(originalBarSignBrightness));
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