#ifndef Lektor_h
#define Lektor_h

#include <stdint.h>

#ifndef __linux__
#include <Arduino.h>
#include <stdarg.h>
#endif

#define LEKTOR_ERROR_STRING_EMPTY -1
#define LEKTOR_ERROR_STRING_OVERLAP -2
#define LEKTOR_ERROR_UNIMPLEMENTED -3
#define LEKTOR_ERROR_ILLEGAL -4


#define LEKTOR_AUDIO_I2S 0
#define LEKTOR_AUDIO_I2S_SB 1
#define LEKTOR_AUDIO_PWM5 2
#define LEKTOR_AUDIO_PWM7 3
#define LEKTOR_AUDIO_PWM_MASK 2

#define LEKTOR_BUFFER_SIZE 1024
#define LEKTOR_PRINTF_SIZE 128

typedef struct {
  float a;
  float b;
  float c;
  float p1;
  float p2;
}
resonator_t, *resonator_ptr;

class Lektor
#ifndef __linux__
: public Print
#endif
{
    public:
        Lektor(void);
        int say(char *str);
        int setSpeed(float s);
        int setAudioMode(int n);
        int setRobotic(int n);
        int setFrequency(int n);
        int setSampleFreq(int n);
        void setIdleCallback(int (*callback)(void));
        void setIdleMicros(int us);
        void setMouthCallback(void (*callback)(int));
#ifndef __linux__
        virtual size_t write(uint8_t);
        size_t wwrite(uint16_t);
        void clean(void);
        int flush(void);
#ifdef F
        int printf(const __FlashStringHelper *format, ...);
#endif
        int printf(const char *format, ...);

#endif


    private:
        char buffer[LEKTOR_BUFFER_SIZE];
        int say_buffer(void);
        uint16_t getUnichar(char **str);
        int uniIsspace(int znak);
        int _pushout(char c, char **strout, char *strin, unsigned char *blank);
        int _cpy(int n, char **strout, char *strin, unsigned char *blank);
        int getPhrase(char *buffer, size_t buflen, char **str);
        int toISO2(char *buffer, int buflen);
        int isoAlnum(unsigned char znak);
        int isoAlpha(unsigned char znak);
        int isoVowel(unsigned char znak);
        int isoUpper(unsigned char znak);
        int isoToLower(unsigned char znak);
        int prestresser(void);
        int translator(void);
        int poststresser(unsigned char *pos);
        int phtoelm(int mode);
        int _pushbuf(unsigned char znak);
        int _pushstr(int offs);
        int put_hour(int hour, int minute);
        int put_triplet(int triplet, int leading);
        int put_mlnform(int ile, int co);
        int put_int(uint32_t arg);
        int put_date(int d, int m, int y);
        int put_ipadr(void);
        int is_ipadr(void);
        int eos(void);
        int is_hour(void);
        int is_date(void);
        int spell_number(void);
        int match_unit(void);
        int match_secdry(void);
        int put_number(void);
        void make_room(unsigned char *buffer);
        int classify_word(int which);
        int getDict(void);
        unsigned char *next_char(unsigned char *pos);
        unsigned char *prev_char(unsigned char *pos);
        int inclass(unsigned char znak, int clasno);
        int ditlen(unsigned char *c);
        int inclassa(unsigned char znak, int clasno);
        int encode(unsigned char *pos);
        int pasuj(unsigned char *inp, int prod, int dir);

        int holmes(uint16_t *elm, unsigned int mode);

        int parwave(struct klatt_frame *pars);
        void parwave_init(void);
        void flutter(struct klatt_frame *pars);
        float impulsive_source(long nper);
        float natural_source(long nper);
        void pitch_synch_par_reset(struct klatt_frame *frame, long ns);
        void frame_init(struct klatt_frame *frame);
        void setabc(long int f, long int bw, resonator_ptr rp);
        void setabcg(long int f, long int bw, resonator_ptr rp, float gain);
        void setzeroabc(long int f, long int bw, resonator_ptr rp);


        int (*idle_callback)(void);
        void (*mouth_callback)(int mouth);
        int init_audio(void);
        int put_sample(short int sample);
        void finalize_audio(void);
        int audio_callback(void);

#ifndef __linux__
        int charcount;
        int last_error;
#endif

        // phraser
        unsigned char *strbeg;
        unsigned char *thistr;
        unsigned char blank;
        int par1, par2, par3;

        // audio

        int audioMode;
        int idle_micros;

        // holmes
        double mSec_per_frame;
        int robotic;
        int frequency;

        // klatt
        // global
        int  g_f0_flutter;
        int  g_outsl;
        long g_samrate;
        long g_nspfr;
        int g_nfcascade;
        // internals
        int time_count;
        int warnsw;                    /* JPI added for f0 flutter */

/* COUNTERS */

        long nper;                 /* Current loc in voicing period   40000 samp/s */

/* COUNTER LIMITS */

        long T0;                   /* Fundamental period in output samples times 4 */
        long nopen;                /* Number of samples in open phase of period  */
        long nmod;                 /* Position in period to begin noise amp. modul */

/* Variables that have to stick around for awhile, and thus locals
   are not appropriate
 */

/* Incoming parameter Variables which need to be updated synchronously  */

        long F0hz10;               /* Voicing fund freq in Hz  */
        long AVdb;                 /* Amp of voicing in dB,    0 to   70  */
        long Kskew;                /* Skewness of alternate periods,0 to   40  */

/* Various amplitude variables used in main loop */

        float amp_voice;           /* AVdb converted to linear gain  */
        float amp_bypas;           /* AB converted to linear gain  */
        float par_amp_voice;       /* AVpdb converted to linear gain  */
        float amp_aspir;           /* AP converted to linear gain  */
        float amp_frica;           /* AF converted to linear gain  */
        float amp_breth;           /* ATURB converted to linear gain  */

/* State variables of sound sources */

        long skew;                 /* Alternating jitter, in half-period units  */

        //float natglot_a;           /* Makes waveshape of glottal pulse when open  */
        //float natglot_b;           /* Makes waveshape of glottal pulse when open  */
        float vwave;               /* Ditto, but before multiplication by AVdb  */
        float vlast;               /* Previous output of voice  */
        float nlast;               /* Previous output of random number generator  */
        float glotlast;            /* Previous value of glotout  */
        float decay;               /* TLTdb converted to exponential time const  */
        float onemd;               /* in voicing one-pole low-pass filter  */
        float minus_pi_t;          /* func. of sample rate */
        float two_pi_t;            /* func. of sample rate */


/* INTERNAL MEMORY FOR DIGITAL RESONATORS AND ANTIRESONATOR  */


        resonator_t rnpp;
        resonator_t r1p;
        resonator_t r2p;
        resonator_t r3p;
        resonator_t r4p;
        resonator_t r5p;
        resonator_t r6p;
        resonator_t r1c;
        resonator_t r2c;
        resonator_t r3c;
        resonator_t r4c;
        resonator_t r5c;
        resonator_t r6c;
        resonator_t r7c;
        resonator_t r8c;
        resonator_t rnpc;
        resonator_t rnz;
        resonator_t rgl;
        resonator_t rlp;
        resonator_t rout;


};


#endif
