#ifndef STDIO_H
#define STDIO_H

#define stdin -1
#define stdout -2

static int (*putc)(char);

int putchar(char c) {
    #dw 0xc001
    return (unsigned char)c;
}

static int putd(int n, int pad, char padc) {
    char s[20];
    int i = 0;
    if(n < 0) { n = -n; putc('-'); }
    do {
        s[i++] = n%10;
        n /= 10;
    } while(n);
    for(n = i; n < pad; n++) putc(padc);
    do putc(s[--i]+'0'); while(i);
    return n;
}

static int putx(unsigned n, char a, int pad, char padc) {
    char s[4];
    int i = 0;
    do {
        char c = n&0xf;
        c += (c < 10) ? '0' : a;
        s[i++] = c;
        n >>= 4;
    } while(n);
    for(n = i; n < pad; n++) putc(padc);
    do putc(s[--i]); while(i);
    return n;
}

int puts(char *s, int pad, char padc) {
    int i;
    for(i = 0; s[i]; i++);
    for(; i < pad; i++) putc(padc);
    for(i = 0; s[i]; i++) putc(s[i]);
    return i;
}

static int _printf(char *s, void **a) {
    char c;
    int x = 0;
    while(c = *s++) {
        if(c == '%') {
            char padc = ' '; int pad = 0;
again:
            c = *s++;
            switch(c) {
            case 'd': x += putd(*a++, pad, padc); break;
            case 'c': putc(*a++); x++; break;
            case 's': x += puts(*a++, pad, padc); break;
            case 'x': x += putx(*a++, 'a'-10, pad, padc); break;
            case 'X': x += putx(*a++, 'A'-10, pad, padc); break;
            case '.': padc = '0'; goto again;
            case 0: break;
            default:
                if(c >= '0' && c <= '9') { pad = c - '0'; goto again; }
                putc(c); x++; break;
            }
            //0; #dw 0xc003
        } else { putc(c); x++; }
    }
    return x;
}

int printf(char *s, ...) {
    void **a = (void**)&s+1;
    putc = putchar;
    return _printf(s, a);
}

static char *sds;

static int sputc(char c) {
    *sds++ = c;
    return (unsigned char)c;
}

int sprintf(char *ds, char *s, ...) {
    void **a = (void**)&s+1;
    sds = ds;
    putc = sputc;
    int n = _printf(s, a);
    *s++ = 0;
    return n;
}

typedef void FILE;
#define EOF -1

FILE *fopen(char *filename, char *mode) {
    if(mode[1] == 0 || mode[1] == 'b' && mode[2] == 0) {
        if(*mode == 'r') {
            filename; #dw 0xc004
        } else if(*mode == 'w') {
            filename; #dw 0xc005
        } else 0;
    } else 0;
}

int fputc(char c, FILE *fp) {
    if(fp == stdout) { c; #dw 0xc001 } else { c; #dw 0xc007 }
}
int fgetc(FILE *fp) { if(fp == stdin) { #dw 0xc002 } else { fp; #dw 0xc008 } }
int fclose(FILE *fp) { #dw 0xc006 }
int getchar() { #dw 0xc002 }

static FILE *putfp;

static int fput(char c) {
    return fputc(c, putfp);
}

int fprintf(FILE *fp, char *s, ...) {
    void **a = (void*)&s+1;
    putfp = fp;
    putc = fput;
    return _printf(s, a);
}

unsigned fwrite(void *data, unsigned sz, unsigned n, FILE *fp) {
    unsigned *dat = (unsigned*)data;
    sz /= 2;
    n *= sz;
    for(unsigned i = 0; i < n; i++) {
        unsigned d = dat[i];
        if(fputc(d, fp) == EOF || fputc(d>>8, fp) == EOF)
            return i/sz;
    }
    return n/sz;
}

unsigned fread(void *data, unsigned sz, unsigned n, FILE *fp) {
    void **data = (void**)data;
    sz /= 2;
    n *= sz;
    for(unsigned i = 0; i < n; i++) {
        unsigned char c = fgetc(fp), d = fgetc(fp);
        if(c == EOF || d == EOF) return i/sz;
        data[i] = c|d<<8;;
    }
    return n/sz;
}

char *fgets(char *buf, unsigned n, FILE *fp) {
    unsigned i;
    for(i = 0; i < n-1; i++) {
        char c = buf[i] = fgetc(fp);
        if(c == EOF) break;
        if(c == '\n') { i++; break; }
    }
    buf[i] = 0;
    if(!i) return 0;
    return buf;
}

int fputs(char *s, FILE *fp) {
    int i;
    for(i = 0; s[i]; i++)
        if(fputc(s[i], fp) == EOF) return EOF;
    return i;
}

#endif
