#include <vwindow.h>

/* siezure warning */

int img[] = {
    0xffff,0x8001,0x8001,0x8001,0x8001,0x8001,0xffff,
};

int pts[] = {
    180,120,
    100,177,
    117, 300,
    50, 140,
    150, 40,
    67,67,
    10,20,
    100,200,
    300,200,
    190,190,
    170,160,
    145,234,
};

#define NPTS 12

int main() {
    int c = 0;
    vinit(320, 240);
    vclear(c);
    vshow();
    vsettick(25);
    int n = NPTS;
    char k = 0;
    int x0 = 0, y0 = 0;
    for(;;) {
        if(vtick()) {
            vclear(c = c+0x081&0xfff);
            for(int i = 0; i < n; i++) {
                vblit(img, 0xf00, 16, 7, pts[i*2]+x0, pts[i*2+1]+y0, 16, 7);
                vblit(img, 0xfff, 16,7,pts[i*2]+x0+10,pts[i*2+1]+y0+30,16,7);
                vblit(img, 0xfff, 16,7,pts[i*2]+x0+70,pts[i*2+1]+y0-10,16,7);
            }
            vshow();
            int j = vjoy(0);
            if(j&0x8000) y0--;
            if(j&0x4000) y0++;
            if(j&0x2000) x0--;
            if(j&0x1000) x0++;
        }
        if(!k) k = vkey();
        else if(!vkey()) k = 0;
        else continue;
        if(k == 'o') { if(n > 0) n--; }
        else if(k == 'p') { if(n < NPTS) n++; }
        else if(k == 27 || k == 'q') break;
    }
    vquit();
    return 0;
}
