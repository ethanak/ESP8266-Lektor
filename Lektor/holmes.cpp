#include "lektor_private.h"
#include "Lektor.h"
#include <math.h>

#define elm_stdy(elem, n) pgm_read_float(&E_stdy[nEparm * (elem) + (n)])
#define elm_fixd(elem, n) pgm_read_float(&E_fixd[nEparm * (elem) + (n)])
#define elm_prop(elem, n) pgm_read_byte(&E_prop[nEparm * (elem) + (n)])
#define elm_ed(elem, n) pgm_read_byte(&E_ed[nEparm * (elem) + (n)])
#define elm_id(elem, n) pgm_read_byte(&E_id[nEparm * (elem) + (n)])
#define elm_rk(elem) pgm_read_byte(&E_rk[elem])
#define elm_du(elem) (pgm_read_byte(&E_du[elem]) & 127)
#define elm_wvl(elem) (pgm_read_byte(&E_du[elem]) & 128)

typedef struct
{
    float stdy;
    float fixd;
    char  prop;
    char  ed;
    char  id;
    char kludge;
 } interp_t, *interp_ptr;

enum Eparm_e
{
    fn, f1, f2, f3, b1, b2, b3, an, a1, a2, a3, a4, a5, a6, ab, av, avc, asp, af,
  nEparm
};


typedef struct Elm_s
{
    char rk;
    char du;
    uint16_t kludge;
    uint32_t feat;
    interp_t p[nEparm];
} Elm_t;

#include "elmtab.h"

static const struct klatt_frame PROGMEM def_pars = {
     1330,  /* F0       long F0hz10;   */
   60,  /* AV       long AVdb;     */
  500,  /* F1       long F1hz;     */
   60,  /* BW1      long B1hz;     */
 1500,  /* F2       long F2hz;     */
   90,  /* BW2      long B2hz;     */
 2800,  /* F3       long F3hz;     */
  150,  /* BW3      long B3hz;     */
 3250,  /* F4       long F4hz;     */
  200,  /* BW4      long B4hz;     */
 3700,  /* F5       long F5hz;     */
  200,  /* BW5      long B5hz;     */
 4990,  /* F6       long F6hz;     */
  500,  /* BW6      long B6hz;     */
  270,  /* Fnz      long FNZhz;    */
  100,  /* BWnz     long BNZhz;    */
  270,  /* Fnp      long FNPhz;    */
  100,  /* BWnp     long BNPhz;    */
    0,  /* Aasp     long ASP;      */
   30,  /* Nopn     long Kopen;    */
    0,  /* Atur     long Aturb;    */
   10,  /* tilt     long TLTdb;    */
    0,  /* Afrc     long AF;       */
    0,  /* skew     long Kskew;    */
    0,  /* A1       long A1;       */
   80,  /* BWp1     long B1phz;    */
    0,  /* A2       long A2;       */
  200,  /* BWp2     long B2phz;    */
    0,  /* A3       long A3;       */
  350,  /* BWp3     long B3phz;    */
    0,  /* A4       long A4;       */
  500,  /* BWp4     long B4phz;    */
    0,  /* A5       long A5;       */
  600,  /* BWp5     long B5phz;    */
    0,  /* A6       long A6;       */
  800,  /* BWp6     long B6phz;    */
    0,  /* AN       long ANP;      */
    0,  /* AB       long AB;       */
    0,  /* AVpa     long AVpdb;    */
   62   /* G0       long Gain0;    */

};

#define AMP_ADJ 14
#define Float float


typedef struct
{
    float v;                        /* boundary value */
    int t;                          /* transition time */
}
slope_t;

typedef struct
{
    slope_t p[nEparm];
}
trans_t;

typedef struct
{
    float a;
    float b;
    float v;
}
filter_t, *filter_ptr;

static float filter(filter_ptr p, Float v)
{
    return p->v = (p->a * v + p->b * p->v);
}

/* 'a' is dominant element, 'b' is dominated
   ext is flag to say to use external times from 'a' rather
   than internal i.e. ext != 0 if 'a' is NOT current element.

 */


/* static void set_trans PROTO((slope_t * t, Elm_ptr a, Elm_ptr b, int ext, int e)); */

static void
set_trans(slope_t * t, int a, int b, int ext)
{
    int i;
    for (i = 0; i < nEparm; i++) {
        t[i].t = ext ? elm_ed(a,i) : elm_id(a,i);
        if (t[i].t) {
            t[i].v = elm_fixd(a,i) + (elm_prop(a,i) * elm_stdy(b,i)) * 0.01;         }
        else {
            t[i].v = elm_stdy(b,i);
        }
    }
}

/* static float linear PROTO((Float a, Float b, int t, int d)); */

/*
   ______________ b
   /
   /
   /
   a____________/
   0   d
   ---------------t---------------
 */

static float
linear(Float a, Float b, int t, int d)
{
    if (t <= 0) return a;
    else if (t >= d) return b;
    else {
        float f = (float) t / (float) d;
        return a + (b - a) * f;
    }
}

/* static float interpolate PROTO((char *w, char *p, slope_t * s, slope_t * e, Float mid, int t, int d)); */

static float
interpolate(slope_t * s, slope_t * e, Float mid, int t, int d)
{
    float steady = d - (s->t + e->t);
    if (steady >= 0) {
   /* Value reaches stready state somewhere ... */
        if (t < s->t)
            return linear(s->v, mid, t, s->t);	/* initial transition */
        else {
            t -= s->t;
            if (t <= steady)
                return mid;                 /* steady state */
            else
                return linear(mid, e->v, (int) (t - steady), e->t);
     /* final transition */
            }
        }
    else {
        float f = (float) 1.0 - ((float) t / (float) d);
        float sp = linear(s->v, mid, t, s->t);
        float ep = linear(e->v, mid, d - t, e->t);
        return f * sp + ((float) 1.0 - f) * ep;
    }
}

static inline int Stress(int a)
{
    a &= 0x300;
    if (!a) return 0;
    if (a == 0x100) return -1;
    else return a>>8;
}

static inline int Duration(int a)
{
    int du = elm_du(a & 255);
    if (a & 0x400) du = (du * 2)/3;
    else if (a & 0x300) du += 4;
    return du;
}


int Lektor::holmes(uint16_t *elm, unsigned int mode)
{
    filter_t flt[nEparm];
    struct klatt_frame pars;
    int le = 0;
    unsigned i = 0;
    unsigned tstress = 0;
    unsigned ntstress = 0;
    slope_t stress_s;
    slope_t stress_e;
    float top;
    int j;
    float delta_top;
    int delta_beg,delta_end;
    int nelm;

    int last_mouth=0;


    //pars = def_pars;
    memcpy_P(&pars, &def_pars, sizeof(pars));
    pars.F0hz10 = frequency * 10;
    top = 1.1 * pars.F0hz10;
    pars.FNPhz = elm_stdy(le,fn);
    pars.B1phz = pars.B1hz = 60;
    pars.B2phz = pars.B2hz = 90;
    pars.B3phz = pars.B3hz = 150;
    pars.B4phz = def_pars.B4phz;

 /* flag new utterance */
    parwave_init();

 /* Set stress attack/decay slope */
    stress_s.t = 40;
    stress_e.t = 40;
    stress_e.v = 0.0;

    for (j = 0; j < nEparm; j++) {
        flt[j].v = elm_stdy(le,j);
        flt[j].a = 1.0;
        flt[j].b = 0.0;
    }
    for (nelm = 0; elm[nelm]; nelm++);
    delta_beg = nelm - (mode >> 3) + 1;
    delta_end = nelm;
    delta_top=130.0;
    mode &= 7;
    if (mode==TTS_PHRASE_COMMA) delta_top=-50.0;else
    if (mode==TTS_PHRASE_QUESTION) delta_top=-160;else
    if (mode==TTS_PHRASE_EXCLAM) delta_top=250;else
    if (mode==TTS_PHRASE_COMMA2) delta_top=50;
    if (delta_end-delta_beg>0) delta_top=delta_top/(float)(delta_end-delta_beg);
    for (i = 0; i< nelm; i++) {
        int ce = elm[i] & 255;
        int dur = Duration(elm[i]);
        if (dur > 0) {
            int ne = elm[i+1] & 255;
            slope_t start[nEparm];
            slope_t end[nEparm];
            unsigned t;

            if (mouth_callback) {
                int m;
                if (elm_wvl(ce)) {
                    if (Stress(elm[i])) m = 3;
                    else m = 2;
                }
                else {
                    int k;
                    m = 1;
                    for (k = 0; k < len_E_closed; k++) {
                        if (pgm_read_byte(&E_closed[k]) == ce) {
                            m = 0;
                            break;
                        }
                    }
                }
                m |= LEKTOR_MTH_SPEAKING_BIT | pgm_read_byte(&E_shape[ce]);
                if (last_mouth != m) {
                    mouth_callback((last_mouth = m));
                }
            }

            if (elm_rk(ce) > elm_rk(le)) {
                set_trans(start, ce, le, 0);
                /* we dominate last */
            }
            else {
                set_trans(start, le, ce, 1);
                /* last dominates us */
            }

            if (elm_rk(ne) > elm_rk(ce)) {
                set_trans(end, ne, ce, 1);
                /* next dominates us */
            }
            else {
                set_trans(end, ce, ne, 0);
                /* we dominate next */
            }


            for (t = 0; t < dur; t++, tstress++) {
                float base = top * 0.8;
                float tp[nEparm];
                int j;

                if (tstress == ntstress) {
                    j = i+1;
                    stress_s = stress_e;
                    tstress = 0;
                    ntstress = dur;
                    while (j <= nelm) {
                        int e   = elm[j] & 255;
                        unsigned du = Duration(elm[j]);
                        int s  = (j < nelm) ? Stress(elm[j]) : 3;
                        int s1;
                        j++;
                        if (s || elm_wvl(e)) {
                            unsigned d = 0;
                            if (s)
                                stress_e.v = (float) s / 3;
                            else
                                stress_e.v = (float) 0.1;
                            do {
                                d += du;
                                e = elm[j] & 255;
                                du = Duration(elm[j]);
                                s1 = Stress(elm[j++]);
                            } while (elm_wvl(e) && s1 == s);
                            ntstress += d / 2;
                            break;
                        }
                        ntstress += du;
                    }
                }

                for (j = 0; j < nEparm; j++)
                    tp[j] = filter(flt + j, interpolate(&start[j], &end[j], elm_stdy(ce,j), t, dur));

       /* Now call the synth for each frame */

                pars.F0hz10 = base + (top - base) *
                interpolate(&stress_s, &stress_e, (float) 0, tstress, ntstress);

                pars.AVdb = pars.AVpdb = tp[av];
                pars.AF = tp[af];
                pars.FNZhz = tp[fn];
                pars.ASP = tp[asp];
                pars.Aturb = tp[avc];
                pars.B1phz = pars.B1hz = tp[b1];
                pars.B2phz = pars.B2hz = tp[b2];
                pars.B3phz = pars.B3hz = tp[b3];
                pars.F1hz = tp[f1];
                pars.F2hz = tp[f2];
                pars.F3hz = tp[f3];
       /* AMP_ADJ + is a bodge to get amplitudes up to klatt-compatible levels
          Needs to be fixed properly in tables
        */
/*
   pars.ANP  = AMP_ADJ + tp[an];
 */
                pars.AB = AMP_ADJ + tp[ab];
                pars.A5 = AMP_ADJ + tp[a5];
                pars.A6 = AMP_ADJ + tp[a6];
                pars.A1 = AMP_ADJ + tp[a1];
                pars.A2 = AMP_ADJ + tp[a2];
                pars.A3 = AMP_ADJ + tp[a3];
                pars.A4 = AMP_ADJ + tp[a4];
                if (parwave(&pars)) {
                    if (mouth_callback) mouth_callback(0);
                    return 1;
                }

                /* Declination of f0 envelope 0.25Hz / cS */
                if (!robotic && i>=delta_beg && i<delta_end) top -= delta_top;
            }
        }
        le = ce;
    }
    if (mouth_callback) mouth_callback(0);
    return 0;
}
