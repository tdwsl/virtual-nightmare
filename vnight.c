// virtual nightmare interpreter

#include <stdio.h>
#include <string.h>

#define MAXFILES 20

FILE *files[MAXFILES];

unsigned short mem[65536];
unsigned short regs[16], nz;
char debug = 0;

int openFile(unsigned short a, char *m) {
    char buf[81];
    for(int i = 0; i < 80 && mem[a+i] || (buf[i]=0); i++) buf[i] = mem[a+i];
    for(int i = 0; i < MAXFILES; i++)
        if(!files[i]) {
            if(files[i] = fopen(buf, m)) return i+1;
            return 0;
        }
    return 0;
}

int closeFile(unsigned short f) {
    if(f >= MAXFILES) return -1;
    if(!files[f]) return -1;
    if(fclose(files[f])) return -1;
    files[f] = 0;
    return 0;
}

int writeByte(char b, unsigned short f) {
    f--;
    if(f >= MAXFILES) return -1;
    if(!files[f]) return -1;
    return fputc(b, files[f]);
}

int readByte(unsigned short f) {
    f--;
    if(f >= MAXFILES) return -1;
    if(!files[f]) return -1;
    return fgetc(files[f]);
}

int run() {
    for(;;) {
        unsigned short ins = mem[regs[15]];
        if(debug) printf("%.4x %.4x  ", regs[15], ins);
        regs[15]++;
        if(ins < 0x8000) {
            regs[0] = ins;
        } else if(ins < 0xc000) {
            unsigned short b = ins&0xf;
            unsigned short *a = &regs[ins>>4&0xf];
            if(!(ins&0x2000)) b = regs[b];
            else if(b == 15) b = ~regs[0];
            switch(ins>>8&0x1f) {
            case 0: *a = b; break; // mov
            case 1: *a += b; break; // add
            case 2: *a = mem[b]; break; // lw
            case 3: mem[b] = *a; break; // sw
            case 4: *a -= b; break; // sub
            case 5: *a &= b; break; // and
            case 6: *a |= b; break; // or
            case 7: *a ^= b; break; // xor
            case 8: *a = (short)*a>>(short)b; break; // asr
            case 9: *a >>= b; break; // lsr
            case 10: *a <<= b; break; // lsl
            case 11: *a *= b; break; // mul
            case 12: *a = (short)*a/(short)b; break; // div
            case 13: *a /= b; break; // divu
            case 14: *a = (short)*a%(short)b; break; // rem
            case 15: *a %= b; break; // remu
            case 16: *a = (short)*a<(short)b; break; // lt
            case 17: *a = *a<b; break; // ltu
            case 18: *a = (short)*a>(short)b; break; // gt
            case 19: *a = *a>b; break; // gtu
            case 20: *a = (short)*a>=(short)b; break; // ge
            case 21: *a = *a>=b; break; // geu
            case 22: *a = (short)*a<=(short)b; break; // le
            case 23: *a = *a<=b; break; // leu
            case 24: *a = *a==b; break; // eq
            case 25: *a = *a!=b; break; // ne
            case 26: *a = ~b; break; // inv
            case 27: *a = -b; break; // neg
            case 28: *a = regs[15]; regs[15] = b; break; // jl
            case 29: *a = regs[14]+b; break; // va
            case 30: *a = mem[(unsigned short)(regs[14]+b)]; break; // lv
            case 31: mem[(unsigned short)(regs[14]+b)] = *a; break; // sv
            }
            nz = *a;
        } else if(ins < 0xe000) {
            switch(ins&0x1fff) {
            case 0: return 0;
            case 1: fputc(regs[1], stdout); break;
            case 2: regs[1] = fgetc(stdin); break;
            case 3: debug = regs[1] != 0; break;
            case 4: regs[1] = openFile(regs[1], "rb"); break;
            case 5: regs[1] = openFile(regs[1], "wb"); break;
            case 6: regs[1] = closeFile(regs[1]); break;
            case 7: regs[1] = writeByte(regs[1], regs[2]); break;
            case 8: regs[1] = readByte(regs[1]); break;
            default: return 3;
            }
            nz = regs[1];
        } else {
            short a = (short)(ins<<4)>>(short)4;
            if(ins&0x1000) { if(nz) regs[15] += a; }
            else if(!nz) regs[15] += a;
        }
    }
}

int main(int argc, char **args) {
    if(argc < 2) {
        printf("usage: %s file <arg1,arg2,...>\n", args[0]);
        return 1;
    }
    FILE *fp = fopen(args[1], "rb");
    if(!fp) { printf("failed to open %s\n", args[1]); return 1; }
    while(!feof(fp)) {
        unsigned short a, n;
        fread(&a, 2, 1, fp);
        fread(&n, 2, 1, fp);
        if(n && (unsigned short)(n+a) <= a) n = 65536-n;
        fread(&mem[a], 2, n, fp);
    }
    fclose(fp);
    regs[15] = regs[14] = 0;
    unsigned short *a = &mem[regs[14] -= argc-1];
    regs[2] = regs[14]; regs[1] = argc-1;
    for(int i = 1; i < argc; i++) {
        char *s = args[i];
        int i0 = a[i-1] = regs[14] -= strlen(s)+1;
        int j;
        for(j = 0; s[j]; j++) mem[i0+j] = s[j];
        mem[i0+j] = 0;
    }
    unsigned short n = run();
    for(int i = 0; i < MAXFILES; i++) if(files[i]) fclose(files[i]);
    switch(n) {
    case 0: return regs[1];
    case 1: printf("invalid instruction"); break;
    case 2: printf("division by zero"); break;
    case 3: printf("unknown interrupt"); break;
    default: printf("error %d\n", n); break;
    }
    printf(" at 0x%.4x\n", regs[15]-1);
    return 1;
}
