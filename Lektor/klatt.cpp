#include "lektor_private.h"
#include "Lektor.h"
#include <math.h>

#ifndef PI
#define PI M_PI
#endif
#define Float float


/*
 * Convertion table, db to linear, 87 dB --> 32767
 *                                 86 dB --> 29491 (1 dB down = 0.5**1/6)
 *                                 ...
 *                                 81 dB --> 16384 (6 dB down = 0.5)
 *                                 ...
 *                                  0 dB -->     0
 *
 * The just noticeable difference for a change in intensity of a vowel
 *   is approximately 1 dB.  Thus all amplitudes are quantized to 1 dB
 *   steps.
 */

static const float PROGMEM amptable[88] =
{
 0.0, 0.0, 0.0, 0.0, 0.0,
 0.0, 0.0, 0.0, 0.0, 0.0,
 0.0, 0.0, 0.0, 6.0, 7.0,
 8.0, 9.0, 10.0, 11.0, 13.0,
 14.0, 16.0, 18.0, 20.0, 22.0,
 25.0, 28.0, 32.0, 35.0, 40.0,
 45.0, 51.0, 57.0, 64.0, 71.0,
 80.0, 90.0, 101.0, 114.0, 128.0,
 142.0, 159.0, 179.0, 202.0, 227.0,
 256.0, 284.0, 318.0, 359.0, 405.0,
 455.0, 512.0, 568.0, 638.0, 719.0,
 811.0, 911.0, 1024.0, 1137.0, 1276.0,
 1438.0, 1622.0, 1823.0, 2048.0, 2273.0,
 2552.0, 2875.0, 3244.0, 3645.0, 4096.0,
 4547.0, 5104.0, 5751.0, 6488.0, 7291.0,
 8192.0, 9093.0, 10207.0, 11502.0, 12976.0,
 14582.0, 16384.0, 18350.0, 20644.0, 23429.0,
 26214.0, 29491.0, 32767.0
};

/* Generic resonator function */
static float
resonator(
resonator_ptr r,
Float input)
{
 register float x = r->a * input + r->b * r->p1 + r->c * r->p2;
 r->p2 = r->p1;
 r->p1 = x;
 return x;
}

/* Generic anti-resonator function
   Same as resonator except that a,b,c need to be set with setzeroabc()
   and we save inputs in p1/p2 rather than outputs.
   There is currently only one of these - "rnz"
 */
/*  Output = (rnz.a * input) + (rnz.b * oldin1) + (rnz.c * oldin2) */

static float
antiresonator(
resonator_ptr r,
Float input)
{
 register float x = r->a * input + r->b * r->p1 + r->c * r->p2;
 r->p2 = r->p1;
 r->p1 = input;
 return x;
}


void Lektor::flutter(struct klatt_frame *pars)
{
    long original_f0 = pars->F0hz10 / 10;
    double fla = (double) g_f0_flutter / 50;
    double flb = (double) original_f0 / 100;
    double flc = sin(2 * PI * 12.7 * time_count);
    double fld = sin(2 * PI * 7.1 * time_count);
    double fle = sin(2 * PI * 4.7 * time_count);
    double delta_f0 = fla * flb * (flc + fld + fle) * 10;
    F0hz10 += (long) delta_f0;
}

float Lektor::impulsive_source(long nper)
{
    static float doublet[] = {0., 13000000., -13000000.};
    if (nper < 3) {
        vwave = doublet[nper];
    }
    else {
        vwave = 0.0;
    }
 /* Low-pass filter the differenciated impulse with a critically-damped
    second-order filter, time constant proportional to Kopen */
    return resonator(&rgl, vwave);
}



/*----------------------------------------------------------------------------*/
/* Convert formant freqencies and bandwidth into
   resonator difference equation coefficents
 */
void Lektor::setabc(
long int f,                       /* Frequency of resonator in Hz  */
long int bw,                      /* Bandwidth of resonator in Hz  */
resonator_ptr rp)                 /* Are output coefficients  */
{
 double arg = minus_pi_t * bw;
 float r = exp(arg);              /* Let r  =  exp(-pi bw t) */
 rp->c = -(r * r);                /* Let c  =  -r**2 */
 arg = two_pi_t * f;
 rp->b = r * cos(arg) * 2.0;      /* Let b = r * 2*cos(2 pi f t) */
 rp->a = 1.0 - rp->b - rp->c;     /* Let a = 1.0 - b - c */
}

/* Convienience function for setting parallel resonators with gain */
void Lektor::setabcg(
long int f,                       /* Frequency of resonator in Hz  */
long int bw,                      /* Bandwidth of resonator in Hz  */
resonator_ptr rp,                 /* Are output coefficients  */
float gain)
{
    setabc(f, bw, rp);
    rp->a *= gain;
}

/* Convert formant freqencies and bandwidth into
 *      anti-resonator difference equation constants
 */
void Lektor::
setzeroabc(
long int f,                       /* Frequency of resonator in Hz  */
long int bw,                      /* Bandwidth of resonator in Hz  */
resonator_ptr rp)                 /* Are output coefficients  */
{
    setabc(f, bw, rp);               /* First compute ordinary resonator coefficients */
 /* Now convert to antiresonator coefficients */
    rp->a = 1.0 / rp->a;             /* a'=  1/a */
    rp->b *= -rp->a;                 /* b'= -b/a */
    rp->c *= -rp->a;                 /* c'= -c/a */
}

/*----------------------------------------------------------------------------*/


/* Convert from decibels to a linear scale factor */
static float
DBtoLIN(long dB)
{
 /* Check limits or argument (can be removed in final product) */
 if (dB < 0)
  dB = 0;
 else if (dB >= 88)
  {
   dB = 87;
  }
 return pgm_read_float(&amptable[dB]) * 0.001;
}

static float
dBconvert(long arg)
{
 return 20.0 * log10((double) arg / 32767.0);
}


/* Reset selected parameters pitch-synchronously */

void Lektor::pitch_synch_par_reset(struct klatt_frame *frame, long ns)
{
 long temp;
 float temp1;
 if (F0hz10 > 0)
  {
   T0 = (40 * g_samrate) / F0hz10;
   /* Period in samp*4 */
   amp_voice = DBtoLIN(AVdb);

   /* Duration of period before amplitude modulation */
   nmod = T0;
   if (AVdb > 0)
    {
     nmod >>= 1;
    }

   /* Breathiness of voicing waveform */

   amp_breth = DBtoLIN(frame->Aturb) * 0.1;

   /* Set open phase of glottal period */
   /* where  40 <= open phase <= 263 */

   nopen = 4 * frame->Kopen;
   if (nopen > 263)
    {
     nopen = 263;
    }

   if (nopen >= (T0 - 1))
    {
     nopen = T0 - 2;
    }

   if (nopen < 40)
    {
     nopen = 40;                  /* F0 max = 1000 Hz */
    }


   /* Reset width of "impulsive" glottal pulse */

   temp = g_samrate / nopen;
   setabc(0L, temp, &rgl);

   /* Make gain at F1 about constant */

   temp1 = nopen * .00833;
   rgl.a *= (temp1 * temp1);

   /* Truncate skewness so as not to exceed duration of closed phase
      of glottal period */

   temp = T0 - nopen;
   if (Kskew > temp)
    {
     Kskew = temp;
    }
   if (skew >= 0)
    {
     skew = Kskew;                /* Reset skew to requested Kskew */
    }
   else
    {
     skew = -Kskew;
    }

   /* Add skewness to closed portion of voicing period */

   T0 = T0 + skew;
   skew = -skew;
  }
 else
  {
   T0 = 4;                        /* Default for f0 undefined */
   amp_voice = 0.0;
   nmod = T0;
   amp_breth = 0.0;
  }

 /* Reset these pars pitch synchronously or at update rate if f0=0 */

 if ((T0 != 4) || (ns == 0))
  {
   /* Set one-pole low-pass filter that tilts glottal source */
   decay = (0.033 * frame->TLTdb);	/* Function of samp_rate ? */
   if (decay > 0.0)
    {
     onemd = 1.0 - decay;
    }
   else
    {
     onemd = 1.0;
    }
  }
}

/* Get variable parameters from host computer,
   initially also get definition of fixed pars
 */

void Lektor::frame_init(struct klatt_frame *frame)
{
 long Gain0;                      /* Overall gain, 60 dB is unity  0 to   60  */
 float amp_parF1;                 /* A1 converted to linear gain  */
 float amp_parFN;                 /* ANP converted to linear gain  */
 float amp_parF2;                 /* A2 converted to linear gain  */
 float amp_parF3;                 /* A3 converted to linear gain  */
 float amp_parF4;                 /* A4 converted to linear gain  */
 float amp_parF5;                 /* A5 converted to linear gain  */
 float amp_parF6;                 /* A6 converted to linear gain  */

 /*
    Read  speech frame definition into temp store
    and move some parameters into active use immediately
    (voice-excited ones are updated pitch synchronously
    to avoid waveform glitches).
  */

 F0hz10 = frame->F0hz10;
 AVdb = frame->AVdb - 7;
 if (AVdb < 0)
  AVdb = 0;

 amp_aspir = DBtoLIN(frame->ASP) * .05;
 amp_frica = DBtoLIN(frame->AF) * 0.25;

 Kskew = frame->Kskew;
 par_amp_voice = DBtoLIN(frame->AVpdb);

 /* Fudge factors (which comprehend affects of formants on each other?)
    with these in place ALL_PARALLEL should sound as close as
    possible to CASCADE_PARALLEL.
    Possible problem feeding in Holmes's amplitudes given this.
  */
 amp_parF1 = DBtoLIN(frame->A1) * 0.4;	/* -7.96 dB */
 amp_parF2 = DBtoLIN(frame->A2) * 0.15;	/* -16.5 dB */
 amp_parF3 = DBtoLIN(frame->A3) * 0.06;	/* -24.4 dB */
 amp_parF4 = DBtoLIN(frame->A4) * 0.04;	/* -28.0 dB */
 amp_parF5 = DBtoLIN(frame->A5) * 0.022;	/* -33.2 dB */
 amp_parF6 = DBtoLIN(frame->A6) * 0.03;	/* -30.5 dB */
 amp_parFN = DBtoLIN(frame->ANP) * 0.6;	/* -4.44 dB */
 amp_bypas = DBtoLIN(frame->AB) * 0.05;	/* -26.0 db */


 /* Set coefficients of variable cascade resonators */

 if (g_nfcascade >= 6)
  setabc(frame->F6hz, frame->B6hz, &r6c);

 if (g_nfcascade >= 5)
  setabc(frame->F5hz, frame->B5hz, &r5c);

 setabc(frame->F4hz, frame->B4hz, &r4c);
 setabc(frame->F3hz, frame->B3hz, &r3c);
 setabc(frame->F2hz, frame->B2hz, &r2c);
 setabc(frame->F1hz, frame->B1hz, &r1c);

 /* Set coeficients of nasal resonator and zero antiresonator */
 setabc(frame->FNPhz, frame->BNPhz, &rnpc);
 setzeroabc(frame->FNZhz, frame->BNZhz, &rnz);

 /* Set coefficients of parallel resonators, and amplitude of outputs */
 setabcg(frame->F1hz, frame->B1phz, &r1p, amp_parF1);
 setabcg(frame->FNPhz, frame->BNPhz, &rnpp, amp_parFN);
 setabcg(frame->F2hz, frame->B2phz, &r2p, amp_parF2);
 setabcg(frame->F3hz, frame->B3phz, &r3p, amp_parF3);
 setabcg(frame->F4hz, frame->B4phz, &r4p, amp_parF4);
 setabcg(frame->F5hz, frame->B5phz, &r5p, amp_parF5);
 setabcg(frame->F6hz, frame->B6phz, &r6p, amp_parF6);


 /* fold overall gain into output resonator */
 Gain0 = frame->Gain0 - 3;
 if (Gain0 <= 0)
  Gain0 = 57;
 /* output low-pass filter - resonator with freq 0 and BW = g_samrate
    Thus 3db point is g_samrate/2 i.e. Nyquist limit.
    Only 3db down seems rather mild...
  */
 setabcg(0L, (long) g_samrate, &rout, DBtoLIN(Gain0));
}

static short int
clip(Float input)
{
 return (short int)input;
 long temp = input;
 /* clip on boundaries of 16-bit word */
 if (temp < -32767)
  {
   temp = -32767;
  }
 else if (temp > 32767)
  {
   temp = 32767;
  }
 return (temp);
}


int Lektor::parwave(struct klatt_frame *frame)
{
 long ns;
 float out = 0.0;
 /* Output of cascade branch, also final output  */

 /* Initialize synthesizer and get specification for current speech
    frame from host microcomputer */

 frame_init(frame);
 if (g_f0_flutter != 0)
  {
   time_count++;                  /* used for f0 flutter */
   flutter(frame);       /* add f0 flutter */
  }

 /* MAIN LOOP, for each output sample of current frame: */

 for (ns = 0; ns < g_nspfr; ns++)
  {
   static unsigned long seed = 5; /* Fixed staring value */
   float noise;
   int n4;
   float sourc;                   /* Sound source if all-parallel config used  */
   float glotout;                 /* Output of glottal sound source  */
   float par_glotout;             /* Output of parallelglottal sound sourc  */
   float voice;                   /* Current sample of voicing waveform  */
   float frics;                   /* Frication sound source  */
   float aspiration;              /* Aspiration sound source  */
   long nrand;                    /* Varible used by random number generator  */

   /* Our own code like rand(), but portable
      whole upper 31 bits of seed random
      assumes 32-bit unsigned arithmetic
      with untested code to handle larger.
    */
   seed = seed * 1664525 + 1;
   if (8 * sizeof(unsigned long) > 32)
         seed &= 0xFFFFFFFF;

   /* Shift top bits of seed up to top of long then back down to LS 14 bits */
   /* Assumes 8 bits per sizeof unit i.e. a "byte" */
   nrand = (((long) seed) << (8 * sizeof(long) - 32)) >> (8 * sizeof(long) - 14);

   /* Tilt down noise spectrum by soft low-pass filter having
    *    a pole near the origin in the z-plane, i.e.
    *    output = input + (0.75 * lastoutput) */

   noise = nrand + (0.75 * nlast);	/* Function of samp_rate ? */
   nlast = noise;

   /* Amplitude modulate noise (reduce noise amplitude during
      second half of glottal period) if voicing simultaneously present
    */

   if (nper > nmod)
    {
     noise *= 0.5;
    }

   /* Compute frication noise */
   sourc = frics = amp_frica * noise;

   /* Compute voicing waveform : (run glottal source simulation at
      4 times normal sample rate to minimize quantization noise in
      period of female voice)
    */
   for (n4 = 0; n4 < 4; n4++)
    {
       voice = impulsive_source(nper);

     /* Reset period when counter 'nper' reaches T0 */
     if (nper >= T0)
      {
       nper = 0;
       pitch_synch_par_reset(frame, ns);
      }

     /* Low-pass filter voicing waveform before downsampling from 4*g_samrate */
     /* to g_samrate samples/sec.  Resonator f=.09*g_samrate, bw=.06*g_samrate  */

     voice = resonator(&rlp, voice);	/* in=voice, out=voice */

     /* Increment counter that keeps track of 4*g_samrate samples/sec */
     nper++;
    }

   /* Tilt spectrum of voicing source down by soft low-pass filtering, amount
      of tilt determined by TLTdb
    */
   voice = (voice * onemd) + (vlast * decay);
   vlast = voice;

   /* Add breathiness during glottal open phase */
   if (nper < nopen)
    {
     /* Amount of breathiness determined by parameter Aturb */
     /* Use nrand rather than noise because noise is low-passed */
     voice += amp_breth * nrand;
    }

   /* Set amplitude of voicing */
   glotout = amp_voice * voice;

   /* Compute aspiration amplitude and add to voicing source */
   aspiration = amp_aspir * noise;
   glotout += aspiration;

   par_glotout = glotout;

     /* Is ALL_PARALLEL */
     /* NIS - rsynth "hack"
        As Holmes' scheme is weak at nasals and (physically) nasal cavity
        is "back near glottis" feed glottal source through nasal resonators
        Don't think this is quite right, but improves things a bit
      */
     par_glotout = antiresonator(&rnz, par_glotout);
     par_glotout = resonator(&rnpc, par_glotout);
     /* And just use r1p NOT rnpp */
     out = resonator(&r1p, par_glotout);
     /* Sound sourc for other parallel resonators is frication
        plus first difference of voicing waveform.
      */
     sourc += (par_glotout - glotlast);
     glotlast = par_glotout;

   /* Standard parallel vocal tract
      Formants F6,F5,F4,F3,F2, outputs added with alternating sign
    */
   out = resonator(&r6p, sourc) - out;
   out = resonator(&r5p, sourc) - out;
   out = resonator(&r4p, sourc) - out;
   out = resonator(&r3p, sourc) - out;
   out = resonator(&r2p, sourc) - out;

   out = amp_bypas * sourc - out;

   out = resonator(&rout, out);
   //*jwave++ = clip(out); /* Convert back to integer */
   if (put_sample(clip(out))) return 1;

  }
  return 0;
}


void Lektor::parwave_init(void)
{
 long FLPhz = (950 * g_samrate) / 10000;
 long BLPhz = (630 * g_samrate) / 10000;

 minus_pi_t = -PI / g_samrate;
 two_pi_t = -2.0 * minus_pi_t;

 setabc(FLPhz, BLPhz, &rlp);
 nper = 0;                        /* LG */
 T0 = 0;                          /* LG */

 rnpp.p1 = 0;                     /* parallel nasal pole  */
 rnpp.p2 = 0;

 r1p.p1 = 0;                      /* parallel 1st formant */
 r1p.p2 = 0;

 r2p.p1 = 0;                      /* parallel 2nd formant */
 r2p.p2 = 0;

 r3p.p1 = 0;                      /* parallel 3rd formant */
 r3p.p2 = 0;

 r4p.p1 = 0;                      /* parallel 4th formant */
 r4p.p2 = 0;

 r5p.p1 = 0;                      /* parallel 5th formant */
 r5p.p2 = 0;

 r6p.p1 = 0;                      /* parallel 6th formant */
 r6p.p2 = 0;

 r1c.p1 = 0;                      /* cascade 1st formant  */
 r1c.p2 = 0;

 r2c.p1 = 0;                      /* cascade 2nd formant  */
 r2c.p2 = 0;

 r3c.p1 = 0;                      /* cascade 3rd formant  */
 r3c.p2 = 0;

 r4c.p1 = 0;                      /* cascade 4th formant  */
 r4c.p2 = 0;

 r5c.p1 = 0;                      /* cascade 5th formant  */
 r5c.p2 = 0;

 r6c.p1 = 0;                      /* cascade 6th formant  */
 r6c.p2 = 0;

 r7c.p1 = 0;
 r7c.p2 = 0;

 r8c.p1 = 0;
 r8c.p2 = 0;

 rnpc.p1 = 0;                     /* cascade nasal pole  */
 rnpc.p2 = 0;

 rnz.p1 = 0;                      /* cascade nasal zero  */
 rnz.p2 = 0;

 rgl.p1 = 0;                      /* crit-damped glot low-pass filter */
 rgl.p2 = 0;

 rlp.p1 = 0;                      /* downsamp low-pass filter  */
 rlp.p2 = 0;

 vlast = 0;                       /* Previous output of voice  */
 nlast = 0;                       /* Previous output of random number generator  */
 glotlast = 0;                    /* Previous value of glotout  */
 warnsw = 0;
}

