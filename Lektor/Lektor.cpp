#include "lektor_private.h"
#include "Lektor.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


Lektor::Lektor(void)
{
    mSec_per_frame = 8.0;
    robotic = 0;
    g_samrate = 8000;
    g_f0_flutter = 0;
    g_nspfr = (g_samrate * mSec_per_frame) / 1000;
    frequency = 100;
    g_nfcascade = 6;
    idle_callback = NULL;
    mouth_callback = NULL;
#ifndef __linux__
    charcount = 0;
#endif
}

int Lektor::setSpeed(float s)
{
    if (s < -1.0 || s > 1.0) return LEKTOR_ERROR_ILLEGAL;
    mSec_per_frame = 8.0 - 2.0 * s;
    return 0;
}

int Lektor::setRobotic(int n)
{
    robotic = n ? 1 : 0;
    return 0;
}

int Lektor::setFrequency(int t)
{
    if (t < 60 || t > 400) return LEKTOR_ERROR_ILLEGAL;
    frequency = t;
    return 0;
}

int Lektor::setSampleFreq(int t)
{
    if (t < 8000 || t > 16000) return LEKTOR_ERROR_ILLEGAL;
    g_samrate = t;
    return 0;
}

void Lektor::setIdleCallback(int(*callback)(void))
{
    idle_callback = callback;
}

void Lektor::setIdleMicros(int us)
{
    if (us >= 1000) idle_micros = us;
}

void Lektor::setMouthCallback(void (*callback)(int))
{
    mouth_callback = callback;
}

int Lektor::say(char *str)
{
    strcpy(buffer, str);
    return say_buffer();
}

int Lektor::say_buffer(void)
{
    int rc=0;
    char *strp;
    rc = toISO2(buffer, sizeof(buffer));
    if (rc) return rc;
    strp = buffer;
    g_nspfr = (g_samrate * mSec_per_frame) / 1000;
    init_audio();
    for (;;) {
        rc= getPhrase(buffer, LEKTOR_BUFFER_SIZE, &strp);
        if (rc < 0) break;
#if DEBUG
        printf("Mode = %d, boundary = %d\n",
            rc & 7, rc >> 3);
#endif
        rc = holmes((uint16_t *)buffer, rc);
        if (rc) break;
    }
    finalize_audio();
#ifndef __linux__
    charcount = 0;
#endif
    return rc;
}

#ifndef __linux__

size_t Lektor::wwrite(uint16_t znak)
{
    if (znak < 0x80) return write(znak);
    if (znak < 0x800) {
        if (!write((znak >> 6) | 0xc0)) return 0;
        return write((znak & 0x3f) | 0x80)?2:0;
    }
    if (!write((znak >> 12) | 0xe0)) return 0;
    if (!write(((znak >> 6) & 0x3f) | 0x80)) return 0;
    return write((znak & 0x3f) | 0x80)?3:0;
}


size_t Lektor::write(uint8_t znak)
{
    if (znak != '\n') {
        if (charcount >= LEKTOR_BUFFER_SIZE - 2) {
            return 0;
        }
        buffer[charcount++]=znak;
        return (size_t)1;
    }
    buffer[charcount] = 0;
    if (charcount) {
        clean();
        if (last_error = say_buffer()) {
            return 0;
        }
    }
    return (size_t)1;
}

void Lektor::clean(void)
{
    charcount = 0;
    last_error = 0;
}

void Lektor::flush(void)
{
    buffer[charcount] = 0;
    say_buffer();
}

int Lektor::printf(const char *format, ...)
{
    char print_buf[LEKTOR_PRINTF_SIZE];
    int cnt, i;
    va_list ap;
    va_start(ap, format);
    cnt = vsnprintf(print_buf, LEKTOR_PRINTF_SIZE, format, ap);
    va_end(ap);
    if (cnt > LEKTOR_PRINTF_SIZE) {
        cnt = LEKTOR_PRINTF_SIZE;
    }
    for (i = 0; i < cnt; i++) {
        if (!write((uint8_t)print_buf[i])) {
            return last_error;
        }
    }
    return 0;
}

#ifdef F
int Lektor::printf(const __FlashStringHelper *format, ...)
{
    char print_buf[LEKTOR_PRINTF_SIZE];
    int cnt, i;
    va_list ap;
    va_start(ap, format);
#ifdef __AVR__
    cnt = vsnprintf_P(print_buf, LEKTOR_PRINTF_SIZE, (const char *)format, ap);
#else
    cnt = vsnprintf(print_buf, LEKTOR_PRINTF_SIZE, (const char *)format, ap);
#endif
    va_end(ap);
    if (cnt > LEKTOR_PRINTF_SIZE) {
        cnt = LEKTOR_PRINTF_SIZE;
    }
    for (i = 0; i < cnt; i++) {
        if (!write((uint8_t)print_buf[i])) {
            return last_error;
        }
    }
    return 0;
}

#endif
#endif


