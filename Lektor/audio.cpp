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
#include <i2s.h>


uint32_t last_micro_call;
int Lektor::init_audio(void)
{
    if (audioMode < LEKTOR_AUDIO_I2S || audioMode > LEKTOR_AUDIO_PWM7) {
        return LEKTOR_ERROR_ILLEGAL;
    }
    i2s_begin();
    i2s_set_rate((audioMode == LEKTOR_AUDIO_PWM7)?(4 * g_samrate):g_samrate);
    return 0;
}

int Lektor::audio_callback(void)
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


static unsigned short swab(unsigned short w)
{
    return ((w >> 8) & 255) | ((w & 255) << 8);
}

int Lektor::put_sample(short int sample)
{
    uint32_t samp;
    static int err = 0;
    static int last_bit = 0;
    int i, rc;

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
        samp = samp >> 7;
        for (i = 3; i >= 0; i--) {
            i2s_write_sample_nb(fakePwm[(samp + i) >> 2]);
        }
    }
    return 0;
}

void Lektor::finalize_audio(void)
{
    while (!i2s_is_empty()) delay(1);
    i2s_end();
}

#endif
