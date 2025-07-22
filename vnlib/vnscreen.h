#ifndef VNSCREEN_H
#define VNSCREEN_H

void vn_show(unsigned *a) {
    #dw 0xc010
}

unsigned vn_getticks() {
    #dw 0xc011
}

void vn_setticks(unsigned t) {
    #dw 0xc012
}

void vn_pix(unsigned *scr, int x, int y, int c) {
    unsigned i = y*320+x;
    unsigned m = 14-(7-(i&7))*2;
    i >>= 3;
    scr[i] &= ~(0xc000>>m);
    scr[i] |= c<<(14-m);
}

void vn_blit(unsigned *scr,unsigned *img,int m,int w,int h, int x,int y) {
    for(int i = 0; i < h; i++)
        for(int j = 0; j < w; j++) {
            int k = i*w+j;
            int c = img[k>>3]>>((7-(k&7))*2)&3;
            if(c != m) vn_pix(scr, x+j, y+i, c);
        }
}

#endif
