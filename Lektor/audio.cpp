#include "lektor_private.h"
#include "Lektor.h"


int Lektor::setAudioMode(int mode)
{
    if (mode < LEKTOR_AUDIO_I2S || mode > LEKTOR_AUDIO_PWM7) {
        return LEKTOR_ERROR_ILLEGAL;
    }
    audioMode = mode;
    return 0;
}

#ifdef __linux__
#include <stdio.h>
static FILE * fout;

int Lektor::init_audio(void)
{
    fout=popen("play -t raw -e signed-integer -b 16 -c 1 -r 8000 -", "w");
    return 0;
}



int Lektor::put_sample(short int sample)
{
    fwrite(&sample, 2, 1, fout);
    return 0;
}

void Lektor::finalize_audio(void)
{
    pclose(fout);
}

#else
#include <Arduino.h>
#include <i2s_reg.h>
#include <i2s.h>


static uint32_t last_micro_call;
static bool ftime;
int Lektor::init_audio(void)
{
    int i;
    if (audioMode < LEKTOR_AUDIO_I2S || audioMode > LEKTOR_AUDIO_PWM7) {
        return LEKTOR_ERROR_ILLEGAL;
    }
    i2s_begin();
    i2s_set_rate((audioMode == LEKTOR_AUDIO_PWM7)?(4 * g_samrate):g_samrate);
    last_micro_call = micros();
    ftime = true;
    return 0;
}

int ICACHE_FLASH_ATTR Lektor::audio_callback(void)
{
    if (!idle_callback) {
        yield();
        last_micro_call = micros();
        return 0;
    }
    last_micro_call = micros();
    return idle_callback();
}

const uint32_t ICACHE_RODATA_ATTR fakePwm[]={0,
   0x00000001, 0x00010001, 0x01010001, 0x01010101,
   0x01110101, 0x01110111, 0x01111111, 0x11111111,
   0x11111113, 0x11131113, 0x13131113, 0x13131313,
   0x13331313, 0x13331333, 0x13333333, 0x33333333,
   0x33333337, 0x33373337, 0x37373337, 0x37373737,
   0x37773737, 0x37773777, 0x37777777, 0x77777777,
   0x7777777f, 0x777f777f, 0x7f7f777f, 0x7f7f7f7f,
   0x7fff7f7f, 0x7fff7fff, 0x7fffffff, 0xffffffff};


static float gain = 2.9;


unsigned short inline swab(unsigned short w)
{
    return ((w >> 8) & 255) | ((w & 255) << 8);
}

int ICACHE_FLASH_ATTR Lektor::put_sample(short int sample)
{
    int32_t samp;
    static int err = 0;
    static int last_bit = 0;
    int i, rc;

    if (ftime) {
        ftime = false;
        if (audioMode == LEKTOR_AUDIO_PWM5) {
            for (i=0; i<1000; i++) i2s_write_sample_nb(fakePwm[i>>6]);
        }
        else if (audioMode == LEKTOR_AUDIO_PWM7) {
            for (i=0; i<4000; i++) i2s_write_sample_nb(0);
            for (i=0; i<4000; i++) i2s_write_sample_nb(fakePwm[i>>8]);
        }
        else {
            for (i = 0; i < 1000; i++) i2s_write_sample_nb(0);
        }
    }
    if (micros() - last_micro_call > idle_micros) {
        if ((rc = audio_callback())) return rc;
    }
    while (i2s_is_full()) {
        if (micros() - last_micro_call > idle_micros) {
            if ((rc = audio_callback())) return rc;
        }
    }
    if (!(audioMode & LEKTOR_AUDIO_PWM_MASK)) { // I2S lub I2S_SB

        samp =  (int)(sample * gain) & 0xffff;
        if (audioMode == LEKTOR_AUDIO_I2S_SB) {
            samp = swab(samp) & 0xffff;
        }
        samp |= (samp << 16) ;
        i2s_write_sample_nb(samp);
        return 0;
    }
    samp = sample * gain + 32768 - err;
    samp = constrain(samp, 0, 65535L);

    if (audioMode == LEKTOR_AUDIO_PWM5) {
        err = samp & 0x7ff;
        samp = samp >> 11;
        i2s_write_sample_nb(fakePwm[samp]);
    }
    else {
        err = samp & 0x1ff;
        samp = samp >> 9;
        for (i = 3; i >= 0; i--) {
            i2s_write_sample_nb(fakePwm[(samp + i) >> 2]);
        }
    }
    return 0;
}

void Lektor::finalize_audio(void)
{
    int i;
    if (audioMode == LEKTOR_AUDIO_PWM5) {
        for (i=1000; i>=0; i--) i2s_write_sample_nb(fakePwm[i>>6]);
        for (i=0; i<1000; i++) i2s_write_sample_nb(0);

    }
    else if (audioMode == LEKTOR_AUDIO_PWM7) {
        for (i=4000; i>=0; i--) i2s_write_sample_nb(fakePwm[i>>8]);
        for (i=0; i<4000; i++) i2s_write_sample_nb(0);
    }
    //while (!i2s_is_empty()) delay(1);
    delay(10);
    i2s_end();
}

#endif
