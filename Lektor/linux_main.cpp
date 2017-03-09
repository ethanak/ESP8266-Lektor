#ifdef __linux__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Lektor.h"

Lektor lektor;


void mcb(int a)
{
    if (!a) printf("========\n");
    else printf("E = %d, C = %c, P = %c\n",
        LEKTOR_MTH_OPEN(a), 
        LEKTOR_MTH_VOICED(a) ? '*' : ' ',
        ".aieoulwmfx-"[LEKTOR_MTH_SHAPE(a)]);
}
main(int argc, char *argv[])
{
    int opt, i, len;
    char *buf;

    while ((opt = getopt(argc, argv, "rs:f:")) != -1) {
        switch(opt) {
            case 'r':
            lektor.setRobotic(1);
            break;

            case 's':
            lektor.setSpeed(strtod(optarg,NULL));
            break;

            case 'f':
            lektor.setFrequency(strtol(optarg, NULL, 10));
            break;

            default:
            fprintf(stderr, "Błędna opcja\n");
            exit(1);
        }

    }
    lektor.setMouthCallback(mcb);
    len=0;
    for (i=optind; i< argc; i++) {
        len += strlen(argv[i])+1;
    }
    if (!len) {
        fprintf(stderr, "Co mam powiedzieć?\n");
        exit(1);
    }
    buf=(char *)malloc(len);
    buf[0]=0;
    for (i=optind; i< argc; i++) {
        if (i > optind) strcat(buf, " ");
        strcat(buf, argv[i]);
    }
    lektor.say(buf);
}
#endif
