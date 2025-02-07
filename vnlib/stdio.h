#ifndef STDIO_H
#define STDIO_H

static int (*putc)(char);

int putchar(char c) {
    #dw 0xc001
    return (unsigned char)c;
}

static int putd(int n) {
    char s[20];
    int i = 0;
    if(n < 0) { n = -n; putc('-'); }
    do {
        s[i++] = n%10;
        n /= 10;
    } while(n);
    n = i;
    do putc(s[--i]+'0'); while(i);
    return n;
}

static int putx(unsigned n, char a) {
    char s[4];
    int i = 0;
    do {
        char c = n&0xf;
        c += (c < 10) ? '0' : a;
        s[i++] = c;
        n >>= 4;
    } while(n);
    n = i;
    do putc(s[--i]); while(i);
    return n;
}

int puts(char *s) {
    int i;
    for(i = 0; s[i]; i++) putc(s[i]);
    return i;
}

static int _printf(char *s, void **a) {
    char c;
    int x = 0;
    while(c = *s++) {
        if(c == '%') {
            c = *s++;
            //#dw 0xc003
            switch(c) {
            case 'd': x += putd(*a++); break;
            case 'c': putc(*a++); x++; break;
            case 's': x += puts(*a++); break;
            case 'x': x += putx(*a++, 'a'-10); break;
            case 'X': x += putx(*a++, 'A'-10); break;
            case 0: break;
            default: putc(c); x++; break;
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
    if(mode[1] == 0 || mode[2] == 'b' && mode[3] == 0) {
        if(*mode == 'r') {
            filename; #dw 0xc004
        } else if(*mode == 'w') {
            filename; #dw 0xc005
        } else 0;
    } else 0;
}

int fputc(char c, FILE *fp) { #dw 0xc007 }
int fgetc(FILE *fp) { #dw 0xc008 }
int fclose(FILE *fp) { #dw 0xc006 }

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
    void **data = (void**)data;
    unsigned t = (sz*n+1)/2;
    for(unsigned i = 0; i < t; i++) {
        if(fputc(data[i], fp) == EOF) return i*2/sz;
    }
    return n*sz;
}

unsigned fread(void *data, unsigned sz, unsigned n, FILE *fp) {
    void **data = (void**)data;
    unsigned t = (sz*n+1)/2;
    for(unsigned i = 0; i < t; i++) {
        char c = fgetc(fp);
        if(c == EOF) return i*2/sz;
        data[i] = c;
    }
    return n*sz;
}

#endif
