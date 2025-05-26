#ifndef VWINDOW_H
#define VWINDOW_H

void vinit(int w, int h) {
    #dw 0xc010
}
void vquit() {
    #dw 0xc011
}
void vclear(int c) {
    #dw 0xc012
}
void vshow() {
    #dw 0xc013
}
void vblit(short *im, int c, int w, int h, int x, int y, int dw, int dh) {
    #dw 0xc014
}
int vkey() {
    #dw 0xc015
}
int vjoy(int n) {
    #dw 0xc016
}
void vsettick(int t) {
    #dw 0xc017
}
int vtick() {
    #dw 0xc018
}

#endif
