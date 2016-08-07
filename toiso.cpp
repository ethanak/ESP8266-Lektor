#include "lektor_private.h"
#include "Lektor.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "toiso.h"

static const unsigned char PROGMEM letterflags[] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0,
0, 7, 3, 3, 3, 7, 3, 3, 3, 7, 3, 3, 3, 3, 3, 7,
3, 3, 3, 3, 3, 7, 3, 3, 3, 7, 3, 0, 0, 0, 0, 0,
0, 5, 1, 1, 1, 5, 1, 1, 1, 5, 1, 1, 1, 1, 1, 5,
1, 1, 1, 1, 1, 5, 1, 1, 1, 5, 1, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 7, 0, 7, 0, 7, 7, 0, 0, 7, 7, 7, 7, 0, 7, 7,
0, 5, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1,
3, 7, 7, 7, 7, 3, 3, 3, 3, 7, 7, 7, 7, 7, 7, 3,
3, 3, 3, 7, 7, 7, 7, 0, 3, 7, 7, 7, 7, 3, 3, 1,
1, 5, 5, 5, 5, 1, 1, 1, 1, 5, 5, 5, 5, 5, 5, 1,
1, 1, 1, 5, 5, 5, 5, 0, 1, 5, 5, 5, 5, 5, 1, 0
};

static const unsigned char PROGMEM lowerer[] = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
64, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 91, 92, 93, 94, 95,
96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
160, 177, 162, 179, 164, 181, 182, 167, 168, 185, 186, 187, 188, 173, 190, 191,
176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
240, 241, 242, 243, 244, 245, 246, 215, 248, 249, 250, 251, 252, 253, 254, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

uint16_t Lektor::getUnichar(char **str)
{
    uint16_t znak;
    char n;
    unsigned char m;
    if (!*str) return 0;
    znak=(*(*str)++) & 255;
    if (!(znak & 0x80)) return znak;
    if ((znak & 0xe0)==0xc0) n=1;
    else if ((znak & 0xf0)==0xe0) n=2;
    else {
        while (((**str) & 0xc0) == 0x80) (*str)++;
        return 32;
    }
    znak &= 0x1f;
    while (n--) {
        m = **str;
        if ((m & 0xc0) != 0x80) {
            return 32;
        }
        (*str)++;
        znak = (znak << 6) | (m & 0x3f);
    }
    return znak;
}

int Lektor::uniIsspace(int znak)
{
	int i;
	static int spacje[]={0x1680,0x180e,0x2028,0x2029,0x205f,0x3000,0};
	if (znak<=0x20) return 1;
	if (znak>=0x7f && znak <=0xa0) return 1;
	if (znak>=0x2000 && znak <=0x200b) return 1;
	for (i=0;spacje[i];i++) if (znak==spacje[i]) return 1;
	return 0;
}

#define pushout(znak) _pushout(znak, &strout, strin, &blank)
#define cpy(znak) _cpy(znak, &strout, strin, &blank)

int Lektor::_pushout(char c, char **strout, char *strin, unsigned char *blank)
{
    if (c == ' ') {
        *blank = 1;
        return 0;
    }
    if (*blank) {
        if ((*strout) >= strin) return 1;
        *(*strout)++ = ' ';
        *blank = 0;
    }
    if ((*strout) >= strin) return 1;
    *(*strout)++ = c;
    return 0;
}

int Lektor::_cpy(int n, char **strout, char *strin, unsigned char *blank)
{
    char znak;
    for (;;) {
        znak = pgm_read_byte(&charstrings[n++]);
        if (!znak) return 0;
        if (_pushout(znak, strout, strin, blank)) return LEKTOR_ERROR_STRING_OVERLAP;
    }
    return 0;
}

int Lektor::toISO2(char *buffer, int buflen)
{
    size_t l=strlen(buffer);
    char *strin = buffer;
    int i;
    char *strout=buffer;
    unsigned char blank = 0;


    if (l < buflen - 1) {
        strin += buflen - (l + 1);
        memmove(strin, buffer, l + 1);
    }
    if (!*strin) return LEKTOR_ERROR_STRING_EMPTY;
    while(*strin) {
        uint16_t znak;
        znak = getUnichar(&strin);
        if (uniIsspace(znak)) {
            pushout(' ');
            continue;
        }
        if (znak < 0x80) {
            if (pushout(znak)) return LEKTOR_ERROR_STRING_OVERLAP;
            continue;
        }
		if (znak == 0x2022) {
			if (pushout('*')) return LEKTOR_ERROR_STRING_OVERLAP;
			continue;
		}
		if (znak == 0x2026) {
			if (pushout('.')) return LEKTOR_ERROR_STRING_OVERLAP;
			if (pushout('.')) return LEKTOR_ERROR_STRING_OVERLAP;
			if (pushout('.')) return LEKTOR_ERROR_STRING_OVERLAP;
			continue;
		}
		if (znak==0x218) znak=0x15e;
		else if (znak==0x219) znak=0x15f;
		else if (znak==0x21a) znak=0x162;
		else if (znak==0x21b) znak=0x163;
        else if (znak== 0x3a9) {
            if (pushout(0xa2)) return LEKTOR_ERROR_STRING_OVERLAP;
            continue;
        }
        else if (znak== 0x20ac) {
            if (pushout(0xa4)) return LEKTOR_ERROR_STRING_OVERLAP;
            continue;
        }
		if (znak<=0x17f) {
            uint16_t zn = pgm_read_word(&chartable[znak - 0xa0]);
            if (!(zn & 0x8000)) {
                if (pushout(zn)) return LEKTOR_ERROR_STRING_OVERLAP;
            }
            else {
                if (cpy(zn & 0x7fff)) return LEKTOR_ERROR_STRING_OVERLAP;
            }
            continue;
        }
		if (znak >= 0x2018 && znak <=0x201b) {
			if (pushout('\'')) return LEKTOR_ERROR_STRING_OVERLAP;
			continue;
		}
		if ((znak >= 0x201c && znak <= 0x201f) || znak == 0x2039 || znak == 0x203a) {
			if (pushout('"')) return LEKTOR_ERROR_STRING_OVERLAP;
			continue;
		}
		if (znak == 0x2013 || znak == 0x2014 || znak == 0x2212 || znak == 0x2015) {
			if (pushout('-')) return LEKTOR_ERROR_STRING_OVERLAP;
			continue;
		}
        for (i=0; i < PROCHARS; i++) if (prochar[i].znak == znak) break;
		if (i < PROCHARS) {
            if (cpy(prochar[i].offs)) return LEKTOR_ERROR_STRING_OVERLAP;
            continue;
        }

        if (pushout(' ')) return LEKTOR_ERROR_STRING_OVERLAP;
    }
    *strout=0;
    return 0;
}


int Lektor::isoVowel(unsigned char znak)
{
    if (pgm_read_byte(&letterflags[znak]) & 4) return 1;
    return 0;
}

int Lektor::isoAlnum(unsigned char znak)
{
    if (pgm_read_byte(&letterflags[znak]) & 9) return 1;
    return 0;
}
int Lektor::isoAlpha(unsigned char znak)
{
    if (pgm_read_byte(&letterflags[znak]) & 1) return 1;
    return 0;
}
int Lektor::isoUpper(unsigned char znak)
{
    if (pgm_read_byte(&letterflags[znak]) & 2) return 1;
    return 0;
}
int Lektor::isoToLower(unsigned char znak)
{
    return pgm_read_byte(&lowerer[znak]);
}

