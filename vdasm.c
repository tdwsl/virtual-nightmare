#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define IW 22
#define NINS 41

char *ins[] = {
    "mov","add","lw","sw",
    "sub","and","or","xor",
    "asr","lsr","lsl","mul",
    "div","divu","rem","remu",
    "lt","ltu","gt","gtu",
    "ge","geu","le","leu",
    "eq","ne","inv","neg",
    "jl","va","lv","sv",
};

void dasm(char *name) {
    unsigned short pc, sz;
    FILE *fp = fopen(name, "rb");
    if(!fp) { printf("failed to open %s\n", name); return; }
    fread(&pc, 2, 1, fp);
    fread(&sz, 2, 1, fp);
    printf("%.4x  ", pc); int x = 0;
    for(;;) {
        unsigned short i;
        fread(&i, 2, 1, fp);
        int w = 0;
        w = printf("%.4x ", i);
        pc++;
        if(i < 0x8000) {
            w += printf("mov r0,%d", i);
        } else if(i < 0xc000) {
            w += printf("%s r%d,", ins[i>>8&0x1f], i>>4&0xf);
            if(i&0x2000) {
                if((i&0xf) == 15) w += printf("~r0");
                else w += printf("%d", i&0xf);
            } else w += printf("r%d", i&0xf);
        } else if((i&0xff00) == 0xc000) {
            w += printf("sys %d", i&0xff);
        } else if(i < 0xe000) {
            w += printf("???");
        } else {
            w += printf("j%sz 0x%.4x", ((i&0x1000)?"n":""),
                    (unsigned short)(pc+((short)(i<<4)>>(short)4)));
        }
        if(pc >= sz) break;
        if(++x < 3) {
            while(w < IW) { w++; printf(" "); }
        } else { x = 0; printf("\n%.4x  ", pc); }
    }
    printf("\n");
    fclose(fp);
}

int main(int argc, char **args) {
    for(int i = 1; i < argc; i++)
        dasm(args[i]);
    return 0;
}
