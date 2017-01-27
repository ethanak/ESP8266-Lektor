#include <Lektor.h>

Lektor spiker;

void setup()
{
    spiker.setAudioMode(LEKTOR_AUDIO_I2S_SB);
    Serial.begin(115200);
    delay(100);
}


void loop()
{
    if (Serial.available()) {
        String s = Serial.readStringUntil('\n');
        Serial.println(s);
        delay(100);
        Serial.end();
        delay(10);
        spiker.println(s);
        Serial.begin(115200);
    }
    delay(20);
}
