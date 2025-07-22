#include <vnscreen.h>

unsigned img[] = {
    0x3ff0,
    0xfffc,
    0xf03c,
    0xf47c,
    0xf47c,
    0xe01c,
};

int main() {
    unsigned scr[320*200/8];
    vn_pix(scr, 10, 10, 1);
    vn_pix(scr, 10, 12, 1);
    vn_pix(scr, 12, 10, 1);
    vn_blit(scr, img, 0, 8, 6, 30, 20);
    vn_blit(scr, img, 0, 8, 6, 32, 28);
    for(int x = 50; x < 80; x++)
        for(int y = 30; y < 50; y++)
            vn_pix(scr, x, y, 2);
    for(;;) {
        while(vn_getticks() < 20);
        vn_show(scr);
        vn_setticks(0);
    }
}
