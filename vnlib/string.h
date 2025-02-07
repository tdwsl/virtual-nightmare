#ifndef STRING_H
#define STRING_H

int strcmp(char *a, char *b) {
    for(int i = 0; a[i] || b[i]; i++) {
        int n;
        if(n = a[i]-b[i]) return n;
    }
    return 0;
}

int strlen(char *s) {
    int i;
    for(i = 0; s[i]; i++);
    return i;
}

char *strcpy(char *a, char *b) {
    int i = 0;
    do a[i] = b[i]; while(b[i++]);
    return a;
}

char *strchr(char *s, char c) {
    while(*s) { if(*s == c) return s; ++s; }
    return 0;
}

#endif
