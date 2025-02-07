
void putc(char c) {
    #dw 0xc001
}

void puts(char *s) {
    for(int i = 0; s[i]; i++) putc(s[i]);
}

char data[] = {64, 65, 64, 65, 66, 67, 0};

void test();

void var(int a, char *b, ...) {
    a + b;
}

int main(int argc, char **args) {
    int a = 10, b = 15 + ((a != 10) ? -27 : 1) + (a && 1);
    for(int i = 0; i < 8; i++) putc(64);
    putc(a+b-17);
    for(int i = 0; data[i]; i++) putc(data[i]);
    putc('\n');
    for(int i = 0; i < argc; i++) puts(args[i]);
    putc('\n');
    int g = 0;
lbl:
    for(int i = 0; i < 20; i++) {
        switch(i) {
        case 7: puts("seven"); break;
        case 10: case 11: puts("10-11"); break;
        default: if(i > 15) i++; continue;
        }
        putc('\n');
    }
    if(g++ < 2) goto lbl;
    test();
    var(0xa, "10");
    var(0xa, "10", 0xb, "17");
}

void test() {
    putc(89); putc(10);
}
