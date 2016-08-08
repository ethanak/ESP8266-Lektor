#include "lektor_private.h"
#include "Lektor.h"

#include <stdlib.h>
#include <string.h>
#ifdef ESP8266
#include <c_types.h>
#else
#include <ctype.h>
#endif

#ifdef __linux__
#include "phonames.h"
#include <stdio.h>
#endif

#include "phraser.h"



#define pushbuf(znak) do {if (_pushbuf(znak)) return LEKTOR_ERROR_STRING_OVERLAP;} while(0)
#define pushstr(offs) do {if (_pushstr(pgm_read_word(&offs))) return LEKTOR_ERROR_STRING_OVERLAP;} while(0)

int Lektor::_pushbuf(unsigned char znak)
{
    if (znak == ' ') {
        blank = 1;
        return 0;
    }
    if (blank && znak) {
        if (thistr <= strbeg) return 1;
        *strbeg++ = ' ';
        blank = 0;
    }
    if (thistr <= strbeg) return 1;
    *strbeg++ = znak;
    return 0;
}

int Lektor::_pushstr(int offs)
{
    unsigned char s;
    for (;;) {
        s=pgm_read_byte(&stringi[offs]);
        if (!s) break;
        pushbuf(s);
        offs++;
    }
    return 0;
}

int Lektor::put_hour(int hour, int minute)
{
    if (hour <= 20) {
        pushstr(godziny[hour]);
    }
    else {
        pushstr(godziny[20]);
        pushbuf(' ');
        pushstr(godziny[hour % 10]);
    }
    pushbuf(' ');
    if (!minute) {
        pushstr(jednostki[0]);
        return 0;
    }
    if (minute >=10 && minute < 20) {
        pushstr(jednostki[minute]);
        return 0;
    }
    if (minute < 10) {
        pushstr(jednostki[0]);
    }
    else {
        pushstr(dziesiatki[(minute / 10) - 2]);
    }
    minute %= 10;
    if (minute) {
        pushbuf(' ');
        if (minute == 2) minute = 20;
        pushstr(jednostki[minute]);
    }
    return 0;
}

int Lektor::put_triplet(int triplet, int leading)
{
    if (leading == 2 && triplet < 100) {
        pushstr(jednostki[0]);
        pushbuf(' ');
    }
    if (leading && triplet < 10) {
        pushstr(jednostki[0]);
        pushbuf(' ');
    }
    if (triplet >= 100) {
        pushstr(setki[triplet / 100 - 1]);
        triplet %= 100;
        if (!triplet) return 0;
        pushbuf(' ');
    }

    if (triplet < 20) {
        pushstr(jednostki[triplet]);
        return 0;
    }
    pushstr(dziesiatki[triplet / 10 - 2]);
    triplet %= 10;
    if (triplet) {
        pushbuf(' ');
        pushstr(jednostki[triplet]);
    }
    return 0;
}

int Lektor::put_mlnform(int ile, int co)
{
    int idx=0;
    if (ile == 1) idx = 0;
    else {
        ile %= 100;
        if (ile >= 10 && ile < 20) idx=2;
        else {
            ile %= 10;
            if (ile >= 2 && ile <= 4) idx = 1;
            else idx = 2;
        }
    }
    if (co) idx += 3;
    pushbuf(' ');
    pushstr(tysiace[idx]);
    return 0;
}

int Lektor::put_int(uint32_t arg)
{
    int mln;
    if (arg >= 1000000000) {
        return 1;
    }
    if (arg >= 1000000) {
        mln = arg / 1000000;
        arg = arg % 1000000;
        if (mln != 1) {
            put_triplet(mln, 0);
        }
        if (put_mlnform(mln, 1)) return LEKTOR_ERROR_STRING_OVERLAP;
        if (!arg) return 0;
        pushbuf(' ');
    }
    if (arg >= 1000) {
        mln = arg / 1000;
        arg = arg % 1000;
        if (mln != 1) {
            put_triplet(mln, 0);
        }
        if (put_mlnform(mln, 0)) return LEKTOR_ERROR_STRING_OVERLAP;
        if (!arg) return 0;
        pushbuf(' ');
    }
    return put_triplet(arg,0);
}

int Lektor::put_date(int d, int m, int y)
{
    if (d <= 20) {
        pushstr(dni[d]);
    }
    else {
        pushstr(dni[20 + d/30]);
        pushbuf(' ');
        pushstr(dni[d % 10]);
    }
    pushbuf(' ');
    pushstr(miesiaca[m -1]);
    pushbuf(' ');
    return put_int(y);
}

int Lektor::eos(void)
{
    unsigned char mode = 0;
    if (*(thistr) == '-' && (thistr)[1] && isoAlnum((thistr)[1])) {
        (thistr)++;
        return 0;
    }
    while (*(thistr) == ' ') (thistr)++;
    if (!*(thistr)) return TTS_PHRASE_DOT;
    for ( ;*(thistr) ; (thistr)++) {
        if (isoAlnum(*(thistr)) || ((*(thistr) == '-' || *(thistr) == '+') &&
            isdigit((thistr)[1]))) break;
        if (mode < TTS_PHRASE_DOT && *thistr == '.') mode = TTS_PHRASE_DOT;
        else if (mode < TTS_PHRASE_COMMA && strchr("-,:;()", *(thistr))) {
            mode = TTS_PHRASE_COMMA;
        }
        else if (mode < TTS_PHRASE_EXCLAM && *(thistr) == '!') {
            mode = TTS_PHRASE_EXCLAM;
        }
        else if (mode < TTS_PHRASE_QUESTION && *(thistr) == '?') {
            mode = TTS_PHRASE_QUESTION;
        }
        else break;
    }
    // tu comma2
    return mode;
}


int Lektor::is_hour(void)
{
    unsigned char *c=thistr;
    int h, m;
    if (!isdigit(*c)) return 0;
    h=(*c++) - '0';
    if (!isdigit(*c)) return 0;
    h=(h * 10) + ((*c++) - '0');
    if (*c++ != ':') return 0;
    if (h > 24) return 0;
    if (!isdigit(*c)) return 0;
    m=(*c++) - '0';
    if (!isdigit(*c)) return 0;
    m = (10*m) + ((*c++) - '0');
    if (isoAlnum(*c)) return 0;
    par1=h;
    par2=m;
    thistr=c;
    return 1;
}

int Lektor::is_date(void)
{
    unsigned char *c = thistr;
    int d, m, y, i;
    if (!isdigit(*c)) return 0;
    d = (*c++) - '0';
    if (isdigit(*c)) {
        d = (10 * d) + ((*c++) - '0');
    }
    if (*c++ != '.') return 0;
    if (d < 1 || d > 31) return 0;
    if (!isdigit(*c)) return 0;
    m = (*c++) - '0';
    if (isdigit(*c)) {
        m = (10 * m) + ((*c++) - '0');
    }
    if (*c++ != '.') return 0;
    if (m < 1 || m > 12) return 0;
    y=0;
    for (i=0; i<4; i++) {
        if (!isdigit(*c)) return 0;
        y = (y * 10) + ((*c++) - '0');
    }
    if (isoAlnum(*c)) return 0;
    if (y < 1000 || y > 2999) return 0;
    thistr=c;
    par1 = d;
    par2 = m;
    par3 = y;
    return 1;
}


int Lektor::spell_number(void)
{
    unsigned char *c=thistr;
    int len=0;
    int v, i, t;
    for (len=0; isdigit(c[len]); len++);
    while (len > 0) {
        if (len <= 3) {
            v=0;
            for (i=0; i<len; i++) v = (v * 10) + ((*c++) - '0');
            thistr = c;
            return put_triplet(v, len - 1);
        }
        if (len == 4) t = 2;
        else t = 3;
        v=0;
        for (i=0; i<t; i++) v = (v * 10) + ((*c++) - '0');
        thistr = c;
        if (put_triplet(v, t - 1)) return LEKTOR_ERROR_STRING_OVERLAP;
        len -= t;
        if (len) pushbuf(' ');
    }
    return 0;
}


int Lektor::match_unit(void)
{
    unsigned int i,j;
    unsigned char ch;
    unsigned char *c = thistr;
    while (*c == ' ') c++;
    if (!*c) return -1;
    for (i=0; i < sizeof(unitki) / sizeof(unitki[0]); i++) {
        int n = pgm_read_word(&unitki[i]);
        for (j=0;; j++) {

            ch=pgm_read_byte(&stringi[n+j]);
            if (!ch) {
                if (!isoAlnum(c[j])) {
                    (thistr) = c + j;
                    return i;
                }
                break;
            }
            if (c[j] != ch) {
                break;
            }
        }
    }
    return -1;
}

int Lektor::match_secdry(void)
{
    unsigned int i,j;
    unsigned char ch;
    unsigned char *c = thistr;
    for (i=0; i < sizeof(sunitki) / sizeof(sunitki[0]); i+=2) {
        int n = pgm_read_word(&sunitki[i]);
        for (j=0;; j++) {
            ch=pgm_read_byte(&stringi[n+j]);
            if (!ch) {
                if (!isoAlnum(c[j])) {
                    (thistr) = c + j;
                    return 2*i + 1;
                }
                break;
            }
            if (c[j] != ch) {
                break;
            }
        }
    }
    return 0;
}

int Lektor::put_number(void)
{
    int v = strtol((const char *)thistr, (char **)&thistr, 10);
    int u = 0;
    if (put_int(v)) return LEKTOR_ERROR_STRING_OVERLAP;
    if ((*thistr == '.' || *thistr == ',') && isdigit((thistr)[1])) {
        pushbuf(' ');
        pushstr(wyrazy[2]);
        pushbuf(' ');
        (thistr)++;
        if (spell_number()) return LEKTOR_ERROR_STRING_OVERLAP;
        u=3;
    }
    if (!u && v > 1) {
        v %= 100;
        if (v > 10 && v < 20) u = 2;
        else {
            v = v % 10;
            if (v >= 2 && v <= 4) u = 1;
            else u = 2;
        }
    }
    v = match_unit();
    if (v >= 0) {
        v = (v << 2) + u;
        pushbuf(' ');
        pushstr(unity[v]);
        if (*thistr == '/' && isoAlpha((thistr)[1])) {
            pushstr(wyrazy[3]);
            (thistr)++;
            v = match_unit();
            if (v >= 0) {
                pushstr(unity[4 * v]);
            }
            else {
                v=match_secdry();
                if (v) {
                    pushstr(sunitki[v]);
                }
            }
        }
    }
    return 0;
}

int Lektor::is_ipadr(void)
{
    unsigned char *c=thistr;
    int i,n;
    for (i=0;i<4;i++) {
        if (i && *c++ != '.') return 0;
        if (!isdigit(*c) || (*c == '0' && isdigit(c[1]))) return 0;
        n=strtol((const char *)c,(char **)&c,10);
        if (n < 0 || n > 255) return 0;
    }
    if (*c == '.') c++;
    if (isoAlnum(*c)) return 0;
    return 1;
}

int Lektor::put_ipadr(void)
{
    int last_n = 0;
    int i,n;
    for (i=0;i<4;i++) {
        if (i && *thistr++ != '.') return LEKTOR_ERROR_ILLEGAL;
        n=strtol((const char *)thistr,(char **)&thistr,10);
        if (n < 0 || n > 255) return LEKTOR_ERROR_ILLEGAL;
        if (i) {
            if (last_n == 200) {
                if (n < 100) {
                    pushstr(wyrazy[4]);
                }
            }
        }
        else if (n < 10) {
            last_n %= 100;
            if (last_n > 10 && !(last_n % 10)) {
                pushstr(wyrazy[4]);
            }
        }
        put_int(n);
        last_n = n;
    }
    return 0;
}

void Lektor::make_room(unsigned char *buffer)
{
    int len=thistr - buffer;
    thistr = buffer;
    int ilen = strlen((const char *)buffer) + 1;
    if (ilen < len) {
        thistr += len - ilen;
        memmove(thistr, buffer, ilen);
    }
    blank = 0;
    strbeg=buffer;
}

int Lektor::ditlen(unsigned char *c)
{
    int len;
    for (len=0; isdigit(c[len]); len++);
    return len;
}


int Lektor::getPhrase(char *buffer, size_t buflen, char **str)
{
    int n,i, phrasemode;
    unsigned char syls, lc;

    blank = 0;

    strbeg = (unsigned char *)buffer;
    n = (buflen - (*str - buffer)) - (strlen((const char *)*str)+1);
    if (n > 0) {
        memmove(*str + n, *str, strlen((const char *)*str)+1);
        (*str) += n;
    }
    while (**str) {
        if (isoAlnum(**str) || ((**str == '-' || **str == '+') && isdigit((*str)[1]))) break;
        if (!strchr(",()-.\'\"", **str)) break;
        (*str)++;
    }
    if (!**str) return LEKTOR_ERROR_STRING_EMPTY;
    thistr = (unsigned char *)*str;
    for(;;) {
        n=eos();
        if (n) {
            pushbuf(0);
            phrasemode = n ;
            break;
        }
        if (isoAlpha(*thistr)) {
            n = getDict();
            if (n >= 0) {
                for (;;) {
                    int z = pgm_read_byte(&stringi[n++]);
                    if (!z) break;
                    pushbuf(z);
                }
                pushbuf(' ');
                continue;
            }
            syls = 0;
            lc = 0;
            for (i=0; (thistr)[i]; i++) {
                if (!isoAlpha((thistr)[i])) break;
                if (isoVowel((thistr)[i])) syls = 1;
                if (!isoUpper((thistr)[i])) lc = 1;
            }
            n=i;
            if (syls && lc) {
                for (i=0;i<n;i++) {
                    int z = *(thistr)++;
                    pushbuf(isoToLower(z));
                }
                pushbuf(' ');
                continue;
            }
            if (n == 1 && strchr("wzWZaAiIoOuU",*(thistr))) {
                 pushbuf(isoToLower(*(thistr)++));
                 pushbuf(' ');
                 continue;
            }
            for (i=0; i<n; i++) {
                unsigned char *c = strbeg;
                int znak = (*(thistr)++) & 255;
                if (znak < 0x80) {
                    pushstr(lospel[znak - 0x21]);
                }
                else {
                    pushstr(hispel[znak - 0xa1]);
                }
                if (i == n-1) {
                    if (*c == ' ') c++;
                    if (*c == '0') *c = '1';
                }
                pushbuf(' ');
            }
            continue;

        }
        if (is_ipadr()) {
            if(put_ipadr()) return LEKTOR_ERROR_STRING_OVERLAP;
            pushbuf(' ');
            continue;
        }
        if (is_hour()) {
            if(put_hour(par1, par2)) return LEKTOR_ERROR_STRING_OVERLAP;
            pushbuf(' ');
            continue;
        }
        if (is_date()) {
            if (put_date(par1, par2, par3)) return LEKTOR_ERROR_STRING_OVERLAP;
            pushbuf(' ');
            continue;
        }
        if (*(thistr) == '-' && isdigit(thistr[1])) {
            pushstr(wyrazy[0]);
            pushbuf(' ');
            (thistr)++;
            continue;
        }
        else if (*(thistr) == '+' && isdigit(thistr[1])) {
            pushstr(wyrazy[1]);
            pushbuf(' ');
            (thistr)++;
            continue;
        }
        else if (!isdigit(*thistr)) {
            if (((*thistr) & 127) <= 0x21) {
                pushbuf(' ');
                (thistr)++;
                continue;
            }
            else {
                if ((*thistr) < 0x80) {
                    pushstr(lospel[(*thistr) - 0x21]);
                }
                else {
                    pushstr(hispel[(*thistr) - 0xa1]);
                }
                pushbuf(' ');
                (thistr)++;
                continue;
            }
#ifdef DEBUG
                printf("Unimplemended %c %0xX\n", *thistr, *thistr);
#endif
                return LEKTOR_ERROR_UNIMPLEMENTED;
        }
        if ((*thistr == '0' && isdigit ((thistr)[1])) || ditlen(thistr) > 6) {
            if (spell_number()) return LEKTOR_ERROR_STRING_OVERLAP;
            pushbuf(' ');
            continue;
        }
        if (put_number()) return LEKTOR_ERROR_STRING_OVERLAP;
        pushbuf(' ');
    }
    *str = (char *)thistr;
    make_room((unsigned char *)buffer);
    n = prestresser();
    if (n) return n;
#ifdef DEBUG
    printf("Prestressed [%s]\n",buffer);
#endif
    make_room((unsigned char *)buffer);
    n = translator();
    if (n) return n;
#ifdef DEBUG
    printf("Translated [%s]\n",buffer);
#endif
    poststresser((unsigned char *)buffer);
#ifdef DEBUG
    printf("Poststress [%s]\n",buffer);
#endif
    make_room((unsigned char *)buffer);
    return phtoelm(phrasemode);
}

int Lektor::getDict(void)
{
    unsigned char *c = thistr;
    unsigned int i, j;
    for (i=0; i < sizeof(vobdic) / sizeof(vobdic[0]); i++) {
        int n = pgm_read_word(&vobdic[i]);
        int found = 0;
        for (j=0;; j++) {
            unsigned char b = pgm_read_byte(&stringi[n+j]);
            if (!b) {
                if (!isoAlnum(c[j])) found = 1;
                break;
            }
            if (isoUpper(b)) {
                if (c[j] != b) break;
            }
            else {
                if (isoToLower(c[j]) != b) break;
            }

        }
        if (found) {
            thistr += j;
            return pgm_read_word(&vobval[i]);
        }
    }
    return -1;
}

int Lektor::classify_word(int which)
{
    unsigned char *c = thistr;
    unsigned int i, j;
    if (which) {
        while (*c && *c != ' ') c++;
        while (*c == ' ') c++;
        if (!*c) return 0;
    }
    for (i=0; i < sizeof(wdic) / sizeof(wdic[0]); i++) {
        int n = pgm_read_word(&wdic[i]);
        int found = 0;
        for (j=0;; j++) {
            unsigned char b = pgm_read_byte(&stringi[n+j]);
            if (!b) {
                if (!c[j] || c[j] == ' ') found = 1;
                break;
            }
            if (c[j] != b) break;
        }
        if (found) {
            return pgm_read_byte(&wdicval[i]);
        }
    }

    // hermeneutyka
    if (which) return 0;
    int wlen=0, found=0, nstr;
    unsigned char znak;
    for (wlen=0; c[wlen] && isoAlnum(c[wlen]); wlen++) {
        if (!isoAlpha(c[wlen])) {
            return 0;
        }
    }
    for (i=0; i < sizeof(ikactl) / sizeof(ikactl[0]); i++) {
        nstr = pgm_read_word(&ikactl[i]);
        for (j=0;;j++) {
            znak = pgm_read_byte(&stringi[nstr + j]);
            if (!znak) break;
            if (j >= wlen) break;
            if (isoToLower(c[wlen - j - 1]) != znak) break;
        }
        if (!znak) {
            wlen -= j;
            if (wlen > 3) {
                j=isoToLower(c[wlen-1]);
                if (j == 'i' || j == 'y') found = 1;
            }
            break;
        }
    }
    if (!found) {
        return 0;
    }
    for (i=0; i < sizeof(ikasfx) / sizeof(ikasfx[0]); i++) {
        nstr = pgm_read_word(&ikasfx[i]);
        int stres = (pgm_read_byte(&stringi[nstr]) == '3')? 32 : 0;
        nstr ++;
        for (j=0;;j++) {
            znak = pgm_read_byte(&stringi[nstr + j]);
            if (!znak) return stres;
            if (znak == '#') {
                if (j == wlen) return stres;
                break;
            }
            if (j >= wlen) break;
            if (znak == 'A') {
                if (isoVowel(c[wlen - j - 1])) return stres;
                break;
            }
            if (isoToLower(c[wlen - j - 1]) != znak) break;
        }
    }


    return 0;
}

int Lektor::prestresser(void)
{
    int flag, flag2, next_unstressed=0;
    for(;;) {
        if (*thistr == ' ') {
            pushbuf(' ');
            thistr++;
            continue;
        }
        if (!*thistr) {
            break;
        }
        if (isdigit(*thistr) || *thistr == '-') {
            while (*thistr && *thistr != ' ') {
                pushbuf(*thistr++);
            }
            next_unstressed = 0;
            continue;
        }
        if (next_unstressed) {
            flag = 4;
        }
        else {
            flag = classify_word(0);
        }
        next_unstressed = 0;
        if (flag & 32) { // akcent wymuszony na trzeciej
            pushbuf('3');
        }
        else if (flag & 64) { // nie
            flag2=classify_word(1);
            if (flag2 & 1) { // czasownik
                pushbuf('-');
            }
            else {
                pushbuf('0');
            }
        }
        else if (flag & 8) { // lewy
            flag2 = classify_word(1);
            if (flag2 & 16) {
                pushbuf('1');
                next_unstressed = 1;
            }
            else {
                pushbuf('0');
            }
        }
        else if (flag & (4 | 16)) { // nieakcentowany
            pushbuf('0');
        }
        else {
            pushbuf('2');
        }
        while (*thistr && *thistr != ' ') {
            pushbuf(*thistr++);
        }
    }
    pushbuf(0);
    return 0;
}

unsigned char * Lektor::next_char(unsigned char *pos)
{
    if (!pos) return NULL;
    if (!*pos) return NULL;
    pos++;
    if (isdigit(*pos) || *pos == '-') pos++;
    if (!*pos) return NULL;
    return pos;
}

unsigned char * Lektor::prev_char(unsigned char *pos)
{
    if (!pos) return NULL;
    pos--;
    if (pos <= thistr) return NULL;
    if (isdigit(*pos) || *pos == '-') pos--;
    if (pos <= thistr) return NULL;
    return pos;
}


#define OP_CONC 1
#define OP_ALTER 2
#define OP_CALL 3
#define OP_CHAR 4
#define OP_CLASS 5
#define OP_EXCL 6

#include "translator.h"

int Lektor::inclassa(unsigned char znak, int clasno)
{
    int n;
    unsigned char z;
    if (!clasno) return 1;
    if (!znak) {
        if (clasno >= 8) return 1;
        return 0;
    }
    n=pgm_read_word(&klasy[clasno]);
    for (;; n++) {
        z=pgm_read_byte(&transtrings[n]);
        if (!z) break;
        if (z == znak) return 1;
    }
    return 0;
}


int Lektor::pasuj(unsigned char *inp, int prod, int dir)
{
    int oper, par1, par2, n, n1;
    unsigned char znak;
    for (;;) {
        if (prod & 0x8000) {
            oper = (prod >> 8) & 0x7f;
            par1 = prod & 0xff;
        }
        else {
            oper = pgm_read_byte(&opers[prod]);
            par1 = pgm_read_word(&params[2 * prod]);
            par2 = pgm_read_word(&params[2 * prod + 1]);
        }
        if (inp) znak = *inp;
        else znak = 0;
        switch(oper) {
            case OP_CHAR:
            if (znak !=(par1 & 255)) return 0;
            return 1;

            case OP_CLASS:
            if (!inclassa(znak ,par1)) return 0;
            return 1;

            case OP_CALL:
            return pasuj(inp,pgm_read_word(&funs[par1]),dir);

            case OP_EXCL:
            if (!inclassa(znak, par1)) return 0;
            n=pasuj(inp, par2, dir);
            if (!n) return 1;
            return 0;

            case OP_ALTER:
            n=pasuj(inp, par1, dir);
            if (n) return n;
            prod=par2;
            continue;

            case OP_CONC:
            n=pasuj(inp, par1, dir);
            if (!n) return 0;
            n1=pasuj((dir == 1) ? next_char(inp) : prev_char(inp), par2, dir);
            if (!n1) return 0;
            return n+n1;
        }
        return 0;
    }
}


int Lektor::encode(unsigned char *pos)
{
    int i, n, p;
    unsigned char z;
    for (i=0; ; i++) {
        z=pgm_read_byte(&langtable[i]);
        if (!z) {
            pushbuf(*pos);
            return 1;
        }
        if (z == *pos) break;
    }
    i = pgm_read_word(&langprods[i]);
    for (;;) {
        if (pasuj(next_char(pos), pgm_read_word(&prod_rite[i]), 1) &&
                pasuj(prev_char(pos), pgm_read_word(&prod_left[i]), -1)) {
            n=1;
            p=pgm_read_word(&prod_text[i]);
            for (;;) {
                z=pgm_read_byte(&transtrings[p++]);
                if (!z) break;
                if (z != '$') {
                    pushbuf(z);
                }
                else {
                    n++;
                }
            }
            return n;
        }
        i = (short int)pgm_read_word(&prod_next[i]);
        if ( i < 0) break;
    }
    pushbuf(*pos);
    return 1;
}

int Lektor::translator()
{
    unsigned char *pos = thistr;
    int n;

    for (; *pos;) {
        if (*pos == '-' || isdigit(*pos)) {
            pushbuf(*pos++);
            continue;
        }
        n=encode(pos);
        pos += n;
    }
    pushbuf('q');
    pushbuf('q');
    pushbuf(' ');
    pushbuf(0);
    return 0;
}

int Lektor::poststresser(unsigned char *pos)
{
    unsigned char *outpos=pos, *pstress;
    int i, len;
    unsigned char stressed=0;
    int stress;

    while (*pos) {
        if (isdigit(*pos)) stress = (*pos++) - '0';
        else if (*pos == '-') {
            stress = -1;
            pos++;
        }
        else stress = 2;
        stressed = 0;
        for (i=0; pos[i] && pos[i] != ' '; i++) {
            if (strchr("AEIOUY", pos[i])) {
                stressed = 1;
                break;
            }
        }
        pstress = NULL;
        if (!stressed) {
            len = i;
            if (stress < 0) {
                unsigned char *c=pos + len;
                for (;*c && *c != ' ';c++);
                while (*c == ' ') c++;
                if (!*c) stress = 2;
                else if (isdigit(*c)) {
                    stress = 2;
                    *c='0';
                }
            }
            if (stress > 0) {
                for (i=len-1; i>= 0 && stress > 0; i--) {
                    if (strchr("aeiouy", pos[i])) {
                        pstress = pos + i;
                        stress--;
                    }
                }
                if (pstress) *pstress = toupper(*pstress);
            }
        }

        while (*pos && *pos != ' ') {
            *outpos++ = *pos++;
        }
        while (*pos == ' ') {
            *outpos++ = *pos++;
        }
    }
    *outpos = 0;
    return 0;
}

#include "phtoelm.h"

int Lektor::phtoelm(int mode)
{
    unsigned char stress, znak, elm;
    int pos, melody, nsyl;
    unsigned char *zpr, *zos, *c, *d, *plact;
    unsigned char *edbg;
    zpr=NULL;
    zos=NULL;
    unsigned char lact=0; // ostatnia jest akcentowana
    edbg=strbeg;
    nsyl = 0;
    while (*thistr) {
        znak = *thistr++;
        if (znak == ' ') {
            continue;
        }
        if (strchr("AEIOUY", znak)) {
            stress = 3;
            lact = 1;
            znak = tolower(znak);
            nsyl++;
        }
        else {
            if (strchr("aeiouy", znak)) {
                lact = 0;
                nsyl++;
            }
            stress = 0;
        }

        pos = pgm_read_byte(&phelm[znak]);
        //printf("Znak %c, pos %d\n",znak, pos);
        if (pos == 255) {
            continue;
        }
        for (;;) {
            elm = pgm_read_byte(&elmlist[pos++]);
            if (!elm) break;
#ifdef __linux__
            printf("ELM %s %d\n",phonames[elm], elm);
#endif
            if (!robotic) elm |= (stress << 6);
            if (stress) {
                zpr = zos;
                zos = strbeg;
                if (lact) {
                    plact=strbeg;
                }
            }
            stress = 0;
            if (thistr <= strbeg) return LEKTOR_ERROR_STRING_OVERLAP;
            *strbeg++ = elm;
        }
    }
    if (lact && (nsyl == 1 || mode != TTS_PHRASE_DOT)) {
        if (thistr <= strbeg) return LEKTOR_ERROR_STRING_OVERLAP;
        for (c = strbeg; c >= plact; c--) {
            c[1] = c[0];
        }
        strbeg++;
        lact = 0;
    }
    melody = (zos)?(strbeg - zos):(strbeg - edbg);
    if (melody > 63) melody = 63;
    if (thistr <= strbeg) return LEKTOR_ERROR_STRING_OVERLAP;
    *strbeg = 0;
    /* korekta melodii */

    if (mode == TTS_PHRASE_QUESTION) {
	    if (zos) {
		    if (!lact) *zos= (*zos & 63) | 0x40; /* opadająca jeśli nie ostatnia */
		    else if (zpr) *zpr= (*zpr & 63) | 0x40; /* opadająca przedostatnia akcentowana */
	    }

    }
    else if (mode == TTS_PHRASE_COMMA) {
        if (zos) *zos= (*zos & 63) | 0x40;
    }
    else {
        if (zos && lact) *zos= (*zos & 63) | 0x40;
    }
    /* korekta akcentów */

    for (c = edbg; *c; c++) {
        if ((*c & 0xc0) == 0xc0) {
            c++;
            break;
        }
    }
    unsigned char actyp = 0;
    d = NULL;
    for (; *c; c++) {
        if ((*c & 0xc0) == 0xc0) {
            *c = (*c & 63) | (actyp ? 0x80 : 0x40);
            actyp ^= 1;
            d=c;
        }
    }
    if (d) *d |= 0xc0;
    return (melody << 3) | mode;
}
