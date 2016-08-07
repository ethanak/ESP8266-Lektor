#ifdef __linux__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PROGMEM
#define PSTR(a) a
#define pgm_read_byte(a) (*(unsigned char *)(a))
#define pgm_read_word(a) (*(unsigned short *)(a))
#define pgm_read_float(a) (*(float *)(a))
#define pgm_read_dword(a) (*(unsigned int *)(a))
#define DEBUG 1
#define memcpy_P memcpy

#else

#include <Arduino.h>

#ifdef ESP8266
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif

#endif

#define TTS_PHRASE_DOT 1
#define TTS_PHRASE_COMMA 2
#define TTS_PHRASE_COMMA2 3
#define TTS_PHRASE_EXCLAM 4
#define TTS_PHRASE_QUESTION 5

struct klatt_frame
 {
  long F0hz10;            /* Voicing fund freq in Hz                          */
  long AVdb;              /* Amp of voicing in dB,            0 to   70       */
  long F1hz;              /* First formant freq in Hz,        200 to 1300     */
  long B1hz;              /* First formant bw in Hz,          40 to 1000      */
  long F2hz;              /* Second formant freq in Hz,       550 to 3000     */
  long B2hz;              /* Second formant bw in Hz,         40 to 1000      */
  long F3hz;              /* Third formant freq in Hz,        1200 to 4999    */
  long B3hz;              /* Third formant bw in Hz,          40 to 1000      */
  long F4hz;              /* Fourth formant freq in Hz,       1200 to 4999    */
  long B4hz;              /* Fourth formant bw in Hz,         40 to 1000      */
  long F5hz;              /* Fifth formant freq in Hz,        1200 to 4999    */
  long B5hz;              /* Fifth formant bw in Hz,          40 to 1000      */
  long F6hz;              /* Sixth formant freq in Hz,        1200 to 4999    */
  long B6hz;              /* Sixth formant bw in Hz,          40 to 2000      */
  long FNZhz;             /* Nasal zero freq in Hz,           248 to  528     */
  long BNZhz;             /* Nasal zero bw in Hz,             40 to 1000      */
  long FNPhz;             /* Nasal pole freq in Hz,           248 to  528     */
  long BNPhz;             /* Nasal pole bw in Hz,             40 to 1000      */
  long ASP;               /* Amp of aspiration in dB,         0 to   70       */
  long Kopen;             /* # of samples in open period,     10 to   65      */
  long Aturb;             /* Breathiness in voicing,          0 to   80       */
  long TLTdb;             /* Voicing spectral tilt in dB,     0 to   24       */
  long AF;                /* Amp of frication in dB,          0 to   80       */
  long Kskew;             /* Skewness of alternate periods,   0 to   40 in sample#/2  */
  long A1;                /* Amp of par 1st formant in dB,    0 to   80       */
  long B1phz;             /* Par. 1st formant bw in Hz,       40 to 1000      */
  long A2;                /* Amp of F2 frication in dB,       0 to   80       */
  long B2phz;             /* Par. 2nd formant bw in Hz,       40 to 1000      */
  long A3;                /* Amp of F3 frication in dB,       0 to   80       */
  long B3phz;             /* Par. 3rd formant bw in Hz,       40 to 1000      */
  long A4;                /* Amp of F4 frication in dB,       0 to   80       */
  long B4phz;             /* Par. 4th formant bw in Hz,       40 to 1000      */
  long A5;                /* Amp of F5 frication in dB,       0 to   80       */
  long B5phz;             /* Par. 5th formant bw in Hz,       40 to 1000      */
  long A6;                /* Amp of F6 (same as r6pa),        0 to   80       */
  long B6phz;             /* Par. 6th formant bw in Hz,       40 to 2000      */
  long ANP;               /* Amp of par nasal pole in dB,     0 to   80       */
  long AB;                /* Amp of bypass fric. in dB,       0 to   80       */
  long AVpdb;             /* Amp of voicing,  par in dB,      0 to   70       */
  long Gain0;             /* Overall gain, 60 dB is unity,    0 to   60       */
 };

#define NPAR                 40
