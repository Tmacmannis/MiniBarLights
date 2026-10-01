int multiScanCount = 0;

void multiScanning() {
    EVERY_N_MILLISECONDS_I(timingObj3, 200) {
        multiScanningStateMachine();
        timingObj3.setPeriod(map(masterSpeed, 0, 100, 1000, 50));
    }

    EVERY_N_MILLISECONDS(16) {
        fadeToBlackBy(wineLeds, NUM_LEDS, map(masterSpeed, 0, 100, 2, 30));
    }

    solidColors(true);
}

void multiScanningStateMachine() {
    if (staticColor) {
        CHSV temp1 = hsv2rgb();
        temp1.value = lowerBrightness;
        wineLeds[0 + multiScanCount] = temp1;
        wineLeds[9 - multiScanCount] = temp1;
        wineLeds[10 + multiScanCount] = temp1;
    } else {
        wineLeds[0 + multiScanCount] = CHSV(changingHue, 255, lowerBrightness);
        wineLeds[9 - multiScanCount] = CHSV(changingHue, 255, lowerBrightness);
        wineLeds[10 + multiScanCount] = CHSV(changingHue, 255, lowerBrightness);
    }

    multiScanCount++;
    if(multiScanCount > 4){
        multiScanCount = 0;
        if (!staticColor) {
            changingHue = random8();
        }
    }
}


void sinelon() {
    fadeToBlackBy(wineLeds, 15, 20);
    int pos = beatsin16(map(masterSpeed, 0, 100, 8, 25), 0, 4);
    CHSV temp1 = hsv2rgb();
    temp1.value = lowerBrightness;
    wineLeds[pos] = temp1;
    wineLeds[9 - pos] = temp1;
    wineLeds[pos + 10] = temp1;
    solidColors(true);
}

void rainbow() {
    static uint8_t starthue = 0;
    fill_rainbow(wineLeds, NUM_LEDS, starthue, 7);
    fill_rainbow(compartmentLeds, NUM_LEDS, starthue, 7);
    fill_rainbow(rightGlassLeds, NUM_LEDS, starthue, 7);
    fill_rainbow(leftGlassLeds, NUM_LEDS, starthue, 7);

    for (int i = 0; i < NUM_LEDS; i++) {
        wineLeds[i].nscale8_video(lowerBrightness);
        compartmentLeds[i].nscale8_video(upperBrightness);
        rightGlassLeds[i].nscale8_video(upperBrightness);
        leftGlassLeds[i].nscale8_video(upperBrightness);
    }

    EVERY_N_MILLISECONDS_I(timingRainbow, 20) {
        starthue++;
        timingRainbow.setPeriod(map(masterSpeed, 0, 100, 80, 5));
    }
}