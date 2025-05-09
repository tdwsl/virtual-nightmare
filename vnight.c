// virtual nightmare interpreter

#include <stdio.h>
#include <string.h>

#ifdef VN_SDL
#include <SDL2/SDL.h>

int joykeys[] = {
    SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT,
    SDLK_z, SDLK_x, SDLK_a, SDLK_s, SDLK_c, SDLK_v,
    SDLK_TAB, SDLK_RETURN,
};

unsigned scale = 1;
unsigned scrw, scrh;

SDL_Renderer *renderer;
SDL_Window *window;

void setColor(unsigned rgb) {
    SDL_SetRenderDrawColor(renderer,
        (rgb>>8&0xf)*17, (rgb>>4&0xf)*17, (rgb&0xf)*17, 0xff);
}

void pix(int x, int y) {
    SDL_Rect r = (SDL_Rect) { x*scale, y*scale, scale, scale, };
    SDL_RenderFillRect(renderer, &r);
}

void blit(short *a, short sw, short sh,
        short x0, short y0, short dw, short dh) {
    for(int x = 0; x < dw; x++)
        for(int y = 0; y < dh; y++) {
            int ix = x*sw/dw, iy = y*sh/dh;
            int i = iy*sw+ix;
            char p = a[i>>4]>>(15-(i&0xf))&1;
            if(p) pix(x+x0, y+y0);
        }
}

int initWindow() {
    window = SDL_CreateWindow("Virtual Nightmare",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        scrw*scale, scrh*scale, SDL_WINDOW_SHOWN);
    if(!window) return 4;
    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_SOFTWARE);
    if(!renderer) return 4;
    return 0;
}

void rescale(int s) {
    scale = s;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    initWindow(scrw, scrh);
}

#endif

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
    if(!f) return -1;
    f--;
    if(f >= MAXFILES) return -1;
    if(!files[f]) return -1;
    if(fclose(files[f])) return -1;
    files[f] = 0;
    return 0;
}

int writeByte(char b, unsigned short f) {
    if(!f) return -1;
    f--;
    if(f >= MAXFILES) return -1;
    if(!files[f]) return -1;
    return fputc(b, files[f]);
}

int readByte(unsigned short f) {
    if(!f) return -1;
    f--;
    if(f >= MAXFILES) return -1;
    if(!files[f]) return -1;
    return fgetc(files[f]);
}

int run() {
#ifdef VN_SDL
    SDL_StartTextInput();
    unsigned key, joy[5];
    size_t lastUpdate = SDL_GetTicks();
    size_t tick = 20;
    int init = 0;
#endif
    for(unsigned pctk = 0;; pctk++) {
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
#ifdef VN_SDL
            case 16:
                scrw = regs[1]; scrh = regs[2];
                if(SDL_Init(SDL_INIT_EVERYTHING) < 0) return 4;
                if(initWindow()) return 4;
                init = 1;
                break;
            case 17:
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                init = 0;
                break;
            case 18:
                setColor(regs[1]);
                SDL_RenderClear(renderer);
                break;
            case 19:
                SDL_RenderPresent(renderer);
                break;
            case 20:
                setColor(regs[2]);
                blit(&mem[regs[1]], regs[3], regs[4], regs[5],
                    regs[6], regs[7], regs[8]);
                break;
            case 21:
                regs[1] = key;
                break;
            case 22:
                if(regs[1] > 4) regs[1] = 0;
                else regs[1] = joy[regs[1]];
                break;
            case 23:
                tick = regs[1];
                break;
            case 24:
                {
                    size_t ticks = SDL_GetTicks();
                    if(ticks-lastUpdate >= tick) {
                        regs[1] = 1;
                        lastUpdate = ticks;
                    } else regs[1] = 0;
                    break;
                }
#endif
            }
            nz = regs[1];
        } else {
            short a = (short)(ins<<4)>>(short)4;
            if(ins&0x1000) { if(nz) regs[15] += a; }
            else if(!nz) regs[15] += a;
        }
#ifdef VN_SDL
        if(!init || (pctk&0x1f) != 0x1f) continue;
        SDL_Event ev;
        while(SDL_PollEvent(&ev))
            switch(ev.type) {
            case SDL_TEXTINPUT:
                if(!(*ev.text.text&~0xffff)) key = *ev.text.text;
                break;
            case SDL_KEYUP:
                for(int i = 0; i < 12; i++)
                    if(ev.key.keysym.sym == joykeys[i]) {
                        joy[0] |= 0x8000>>i;
                        break;
                    }
                key = 0;
                break;
            case SDL_KEYDOWN:
                switch(ev.key.keysym.sym) {
                case SDLK_F4: if(scale > 1) rescale(scale-1); break;
                case SDLK_F5: if(scale < 12) rescale(scale+1); break;
                case SDLK_ESCAPE: key = 27; break;
                }
                for(int i = 0; i < 12; i++)
                    if(ev.key.keysym.sym == joykeys[i]) {
                        joy[0] &= ~(0x8000>>i);
                        break;
                    }
                break;
            case SDL_QUIT:
                key = 27;
                break;
            }
#endif
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
    case 4: printf("graphics error"); break;
    default: printf("error %d\n", n); break;
    }
    printf(" at 0x%.4x\n", regs[15]-1);
    return 1;
}
