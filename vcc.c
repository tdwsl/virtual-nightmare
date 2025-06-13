// special c subset compiler for virtual nightmare

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int mstart;

char *defsyms[] = {
    "+","-","&","|","^","<<",">>","*","/","%",
    "+=","-=","&=","|=","^=","<<=",">>=","*=","/=","%=",
    "=","==","!=","<",">","<=",">=","!","~",
    "&&","||","++","--","(",")","[","]","{","}",
    "?",":",";",",",".","->",
    "void","char","short","int","unsigned","const","static","extern",
    "struct","union","enum","typedef","sizeof",
    "if","else","for","while","do","switch","case","default",
    "break","continue","return","goto",
    "#include", "#define", "#if", "#ifdef", "#ifndef", "#endif",
    "#dw",
};

enum {
    P=0,M,AND,OR,XOR,SHL,SHR,MUL,DIV,REM,
    PE,ME,ANDE,ORE,XORE,SHLE,SHRE,MULE,DIVE,REME,
    E,EE,NE,LT,GT,LTE,GTE,NOT,INV,
    AAND,OOR,PP,MM,LP,RP,LB,RB,LC,RC,
    TERN,COL,SEMI,COM,DOT,AR,
    VOID,CHAR,SHORT,INT,UNSIGNED,CONST,STATIC,EXTERN,
    STRUCT,UNION,ENUM,TYPEDEF,SIZEOF,
    IF,ELSE,FOR,WHILE,DO,SWITCH,CASE,DEFAULT,
    BREAK,CONTINUE,RETURN,GOTO,
    INCLUDE,DEFINE,MIF,IFDEF,IFNDEF,ENDIF,
    DW,
    NDEF,
};

enum {
    BRK = 0xdfff, CONT = 0xdffe, RET = 0xdffd, GO = 0xdffc,
};

struct pair { int sym, a; };

#define MAXSYMBOLS 1000
char *symbols[MAXSYMBOLS]; int nsymbols = 0;

struct pair constants[120]; int nconstants = 0;
int enumn = 0;

char strdata[8192]; char *string = strdata;

#define DORG 0x4000
#define BSSORG (DORG+4096)
unsigned short text[16384]; int ntext = 0;
unsigned short data[BSSORG-DORG]; int ndata = 0;
int nbss = BSSORG;

#define MAXTOKENS 2000
short tok[MAXTOKENS]; unsigned short linen[MAXTOKENS];
int toki;

char ahc = 0;
char stopnl = 0;

int lineno;
char *filename;
char *outfile = "a.out";

char *incdirs[20]; int nincdirs = 0;

enum {
    TPTR=0, TFUN, TUNKNOWN, TNEXT,
    TVOID=-1, TINT=-2, TUINT=-3, TVAR=-4,
};

int els[200]; int nels = 0;
struct pair structs[30]; int nstructs = 0;
struct pair typedefs[30]; int ntypedefs = 0;

struct datatype {
    char typetype;
    int *e;
    union { int type, ne; };
};

struct datatype types[80]; int ntypes = 0;
enum {
    TSINGLE=0, TARR=2, TEXTERN, TEXFUN,
};

struct global {
    int sym;
    char gt;
    int dt, a;
};

struct global globals[200]; int nglobals = 0;

int statc[40], nstatc = 0;

struct local {
    int sym, t, a, dim;
};

struct local locals[30]; int nlocals, locala, maxlocala;
int loffs = 0;

int rtype, fname = 0;

struct pair labels[20]; int nlabels;

int mend;

char *toString(int s) {
    if(s == EOF) return "<EOF>";
    if(s < 0) return "?invalid";
    if((s-=NDEF) < 0) return defsyms[s+NDEF];
    if((unsigned)s >= nsymbols) return "?invalid";
    return symbols[s];
}

int toSymbol(char *s) {
    int i;
    for(i = 0; i < NDEF; i++) if(!strcmp(defsyms[i], s)) return i;
    for(i = 0; i < nsymbols; i++) if(!strcmp(symbols[i], s)) return i+NDEF;
    strcpy(string, s);
    symbols[nsymbols] = string;
    string += strlen(s)+1;
    return nsymbols+++NDEF;
}

void errh(int l) {
    printf("%s:%d: ", filename, l);
    if(fname) printf("in %s: ", toString(fname));
}

void erra(int l, const char *msg) {
    errh(l); printf("%s\n", msg); exit(1);
}

int printSymbol(int sym) {
    char *s = toString(sym);
    if(!s) { printf("ERROR ERROR %d\n", sym); exit(1); }
    int n = printf("%s", s);
    if(s[0] == '"') { ++n; printf("\""); }
    else if(s[0] == '\'') { ++n; printf("'"); }
    printf(" ");
    return n+1;
}

void tokerrh() {
    int i;
    if(tok[toki] == EOF) return;
    for(i = toki; i >= 0 && linen[i] == linen[toki]; i--); i++;
    int x = 0;
    for(; i < toki; i++) x += printSymbol(tok[i]);
    for(; linen[i] == linen[toki] && tok[i] != EOF; i++)
        printSymbol(tok[i]);
    printf("\n");
    for(i = 0; i < x; i++) printf(" "); printf("^^\n");
}

void err(const char *msg) {
    tokerrh();
    erra(linen[toki], msg);
}

void errv(const char *msg, int v) {
    tokerrh();
    errh(linen[toki]); printf("%s", msg);
    printSymbol(v); printf("\n"); exit(1);
}

int digit(char c) {
    if(c >= '0' && c <= '9') return (c-'0');
    if(c >= 'A' && c <= 'F') return (c-'A'+10);
    if(c >= 'a' && c <= 'f') return (c-'a'+10);
    return 0;
}

char escapeChar(char **pbuf) {
    char c, d;
    char *buf = *pbuf;
    switch(c = *(buf++)) {
    case 'n': c = 10; break;
    case 't': c = '\t'; break;
    case 'r': c = '\r'; break;
    case 'b': c = '\b'; break;
    case 'v': c = '\v'; break;
    case 'f': c = '\f'; break;
    case 'x':
        if((c = digit(*(buf++))) == -1) {
            buf--; c = 'x';
        } else if((d = digit(*(buf++))) == -1) {
            buf -= 2; c = 'x';
        } else c = c<<4|d;
        break;
    default:
        if(c >= '0' && c <= '7') {
            d = 0;
            do {
                d = d<<3 | (c-'0');
                c = *(buf++);
            } while(c >= '0' && c <= '7');
        }
        break;
    }
    *pbuf = buf;
    return c;
}

void parseString(FILE *fp, char *buf, char q) {
    char c;
    *(buf++) = q;
    if(ahc) { c = ahc; ahc = 0; goto got; }
    for(;;) {
        c = fgetc(fp);
got:
        if(c == q) { *buf = 0; return; }
        if(c == '\n' || c == EOF) {
            err("unterminated quote");
        } if(c == '\\') {
            *(buf++) = c;
            c = fgetc(fp);
            if(c == '\n' || c == EOF)
                err("unterminated quote");
        }
        *(buf++) = c;
    }
}

int extractChar(char *buf) {
    int d, c = 0;
    while(*buf) {
        if((d = *(buf++)) == '\\') d = escapeChar(&buf);
        c = c<<8|d;
    }
    return c;
}

void parseNext(FILE *fp, char *buf) {
    char c;
    int i = 0;
    if(ahc) { c = ahc; ahc = 0; goto got; }
    for(;;) {
        c = fgetc(fp);
got:
        if(c == EOF) { buf[i] = 0; return; }
        if(c <= 32) {
            if(i) { ahc = c; buf[i] = 0; return; }
            if(c == 10) {
                if(stopnl) { ahc = 10; buf[0] = 0; return; } lineno++;
            }
        } else if(strchr("+-&|^~!*/%<>=", c)) {
            if(i) { ahc = c; buf[i] = 0; return; }
            buf[0] = c;
            c = fgetc(fp);
            if(buf[0] == '/' && c == '*') {
                for(;;) {
                    c = fgetc(fp);
                    if(c == '*') { if((c = fgetc(fp)) == '/') break; }
                    if(c == '\n') lineno++;
                }
                continue;
            }
            if(c == '=') { buf[1] = c; buf[2] = 0; return; }
            if(buf[0] == c && strchr("<>+-&|/", c)) {
                if(c == '/') {
                    while(fgetc(fp) != '\n'); lineno++; continue;
                }
                buf[1] = c;
                if(c == '<' || c == '>') {
                    if((c = fgetc(fp)) == '=') { buf[2] = c; buf[3] = 0; }
                    else { ahc = c; buf[2] = 0; }
                } else buf[2] = 0;
                return;
            }
            if(buf[0] == '-' && c == '>') {
                buf[1] = '>'; buf[2] = 0; return;
            }
            ahc = c; buf[1] = 0; return;
        } else if(strchr("(){}[]:;,.?", c)) {
            if(i) { ahc = c; buf[i] = 0; }
            else { buf[0] = c; buf[1] = 0; }
            return;
        } else if(c == '"' || c == '\'') {
            if(i) { ahc = c; buf[i] = 0; }
            else parseString(fp, buf, c);
            return;
        } else buf[i++] = c;
    }
}

int nextSymbol(FILE *fp, char *buf) {
    parseNext(fp, buf);
    if(!buf[0]) return EOF;
    return toSymbol(buf);
}

void terrh() {
    tokerrh();
    errh(linen[toki]);
}

void closing(int sym) {
    if(tok[toki] != sym) {
        terrh();
        printf("expected closing %s\n", toString(sym)); exit(1);
    }
    toki++;
}

void after(int b, int a) {
    if(tok[toki] != a) {
        terrh();
        printf("expected %s after %s\n", toString(a), toString(b)); exit(1);
    }
    toki++;
}

void expect(int sym) {
    if(tok[toki] != sym) {
        terrh();
        printf("expected %s before %s\n", toString(sym), toString(tok[toki]));
        exit(1);
    }
    toki++;
}

void readTokens(FILE *fp) {
    char buf[256];
    int n = 1;
    int s = nextSymbol(fp, buf);
    tok[0] = s;
    linen[0] = lineno;
    if(s == EOF) return;
    if(s == INCLUDE) {
        s = nextSymbol(fp, buf);
        if(s == LT) {
            parseString(fp, buf, '>');
            buf[0] = '"';
            s = toSymbol(buf);
        }
        tok[1] = s; tok[2] = EOF;
        linen[2] = linen[1] = lineno;
        return;
    }
    if(s > INCLUDE && s < ENDIF) {
        stopnl = 1;
        do {
            s = tok[n] = nextSymbol(fp, buf);
            linen[n++] = lineno;
        } while(s != EOF);
        stopnl = 0;
        return;
    }
    if(s == ENDIF) { tok[1] = EOF; linen[1] = lineno; return; }
    int d = 0;
    char f = 0;
    for(;;) {
        if(n >= MAXTOKENS)
            err("token buffer overrun");
        tok[n] = s = nextSymbol(fp, buf);
        linen[n++] = lineno;
        if(s == RP && !(d|f)) {
            tok[n] = s = nextSymbol(fp, buf);
            linen[n++] = lineno;
            if(s == LC) { f = 1; d++; continue; }
        }
        if(s == EOF) return;
        if(s == LC) d++;
        else if(s == RC) { if(!--d && f) break; }
        else if(s == SEMI && !d) break;
    }
    tok[n] = EOF;
}

void printTokens() {
    int p = linen[0];
    printf("%d", p);
    int i;
    for(i = 0; tok[i] != EOF; i++) {
        if(linen[i] != p) printf("\n%d", (p = linen[i]));
        printf(" %s", toString(tok[i]));
    }
    printf("\n\n");
}

int findPair(struct pair *p, int n, int sym) {
    for(int i = 0; i < n; i++)
        if(p[i].sym == sym) return i;
    return -1;
}

int findConstant(int sym) {
    return findPair(constants, nconstants, sym);
}

int findGlobal(int sym) {
    for(int i = 0; i < nglobals; i++)
        if(globals[i].sym == sym) return i;
    return -1;
}

int findLocal(int sym) {
    for(int i = nlocals-1; i >= 0; i--)
        if(locals[i].sym == sym) return i;
    return -1;
}

int findTypedef(int sym) {
    for(int i = 0; i < ntypedefs; i++)
        if(typedefs[i].sym == sym) return i;
    return -1;
}

int number(char *s, int *n) {
    *n = 0;
    if(*s == '0') {
        if(s[1] == 'x') {
            s += 2;
            do {
                *n <<= 4;
                if(*s >= '0' && *s <= '9') *n |= *s - '0';
                else if(*s >= 'a' && *s <= 'f') *n |= *s - 'a' + 10;
                else if(*s >= 'A' && *s <= 'F') *n |= *s - 'A' + 10;
                else return 0;
            } while(*++s);
        } else do {
            if(*s >= '0' && *s <= '7') *n = *n << 3 | *s - '0';
            else return 0;
        } while(*++s);
    } else do {
        if(*s >= '0' && *s <= '9') *n = *n * 10 + *s - '0';
        else return 0;
    } while(*++s);
    return 1;
}

int isTypeStart() {
    int t = tok[toki];
    if(findTypedef(t) != -1) return 1;
    if(t == UNSIGNED || t == CONST || t == VOID || t == STRUCT
            || t == UNION || t == INT || t == SHORT || t == CHAR)
        return 1;
    return 0;
}

void compileString(char *s) {
    char c;
    while(c = *s++) {
        if(c == '\\') c = escapeChar(&s);
        data[ndata++] = c;
    }
    data[ndata++] = 0;
}

void id(int toki) {
    int n;
    char *s = toString(tok[toki]);
    if(tok[toki] < NDEF || number(s, &n)) {
        terrh(); printf("invalid identifier %s\n", s); exit(1);
    }
    if(findConstant(tok[toki]) == -1)
        if(findTypedef(tok[toki]) == -1) return;
    terrh(); printf("%s is already defined\n", s); exit(1);
}

struct datatype *nextType() {
    for(int i = 0; i < ntypes; i++)
        if(types[i].typetype == TNEXT) return &types[i];
    types[ntypes].typetype = TNEXT;
    return &types[ntypes++];
}

int findStruct(int name, int t) {
    for(int i = 0; i < nstructs; i++) {
        if(structs[i].sym == name) {
            int a = structs[i].a;
            if(types[a].typetype == t || types[a].typetype == TUNKNOWN)
                return a;
        }
    }
    struct datatype *d = nextType();
    d->typetype = TUNKNOWN;
    if(name == EOF) return d-types;
    structs[nstructs].sym = name;
    return structs[nstructs++].a = d-types;
}

int sameType(struct datatype *a, struct datatype *b) {
    if(a->typetype != b->typetype) return 0;
    if(a->ne != b->ne) return 0;
    int ne = a->ne;
    if(a->typetype == STRUCT || a->typetype == UNION) ne *= 3;
    for(int i = 0; i < ne; i++)
        if(a->e[i] != b->e[i]) return 0;
    return 1;
}

int readType();

short evalExpr();

int defStruct(int name, int t) {
    int d;
    struct datatype *st;
    st = &types[findStruct(name, t)];
    st->typetype = t;
    st->ne = 0;
    d = 0;
    for(int i = toki; tok[i] != EOF; i++) {
        int t = tok[i];
        if(t == LC || t == LP) d++;
        else if(t == RC || t == RP) { if(!d--) break; }
        else if(!d) {
            if(t == COM || t == SEMI) {
                st->ne++;
                if(tok[i] == SEMI) {
                    while(tok[i] == SEMI) i++; i--;
                }
            }
        }
    }
    st->e = &els[nels];
    nels += st->ne * 3;
    int ne = 0;
    do {
        t = readType();
        if(tok[toki] == SEMI) {
            if(types[t].typetype == TUNKNOWN
                    || types[t].typetype == STRUCT
                    || types[t].typetype == UNION) {
                st->e[ne*3] = EOF;
                st->e[ne*3+1] = t;
                st->e[ne*3+2] = 0;
                ne++;
            } else err("expected identifier");
        } else for(;;) {
            id(toki); st->e[ne*3] = tok[toki++];
            if(tok[toki] == LB) {
                toki++; st->e[ne*3+2] = evalExpr(); closing(RB);
            } else st->e[ne*3+2] = 0;
            st->e[ne*3+1] = t;
            ne++;
            if(tok[toki] == SEMI) break;
            expect(COM);
        }
        do toki++; while(tok[toki] == SEMI);
    } while(tok[toki] != RC);
    toki++;
    for(t = 0; t < ntypes
            && (st == &types[t] || !sameType(&types[t], st)); t++);
    if(t < ntypes) {
        nels -= st->ne * 3;
        if(st == &types[ntypes-1]) ntypes--;
        else st->typetype = TNEXT;
    } else {
        t = st-types;
    }
    return t;
}

int readType0() {
    if(tok[toki] == CONST) toki++;
    int t = tok[toki++];
    if(t == VOID) return TVOID;
    if(t == UNSIGNED) {
        t = tok[toki++];
        if(t == INT || t == SHORT || t == CHAR) return TUINT;
        toki--;
        return TUINT;
    }
    if(t == INT || t == SHORT || t == CHAR) return TINT;
    if(t == STRUCT || t == UNION) {
        int name;
        if(tok[toki] != LC) name = tok[toki++];
        else name = EOF;
        if(tok[toki] == LC) { toki++; return defStruct(name, t); }
        return findStruct(name, t);
    }
    if((t = findTypedef(t)) != -1) return typedefs[t].a;
    toki--; err("expected datatype");
}

int pointer(int t) {
    for(int i = 0; i < ntypes; i++)
        if(types[i].typetype == TPTR && types[i].type == t)
            return i;
    types[ntypes].typetype = TPTR;
    types[ntypes].type = t;
    return ntypes++;
}

int readType() {
    int t = readType0();
    while(tok[toki] == MUL) {
        toki++;
        t = pointer(t);
    }
    return t;
}

int countEls(int term) {
    int n = 1;
    int i, d = 0;
    if(tok[toki] == EOF || tok[toki] == term) return 0;
    for(i = toki; tok[i] != EOF && (d || tok[i] != term); i++) {
        if(tok[i] == RP) d++;
        else if(tok[i] == LP) d--;
        else if(d == 0 && tok[i] == COM) n++;
    }
    if(tok[i-1] == COM) n--;
    return n;
}

int readFptrArgs(int t, int *name) {
    toki++; after(LP, MUL);
    id(toki); *name = tok[toki++];
    closing(RP);
    expect(LP);
    struct datatype *d = &types[ntypes++];
    d->typetype = TFUN;
    d->e = &els[nels];
    nels += (d->ne = countEls(RP)+1);
    d->e[0] = t;
    if(tok[toki] != RP) for(int i = 1;;) {
        if(tok[toki] == DOT) {
            toki++; after(DOT, DOT); after(DOT, DOT);
            d->e[i++] = TVAR; expect(RP); toki--; break;
        }
        d->e[i++] = readType();
        if(tok[toki] == RP) break;
        expect(COM);
    }
    toki++;
    int i;
    for(i = 0; i < ntypes; i++)
        if(&types[i] != d && sameType(&types[i], d))
            break;
    if(i < ntypes) {
        nels -= d->ne;
        ntypes--;
        return pointer(i);
    }
    return pointer(d-types);
}

int readName(int t, int *name) {
    if(tok[toki] == LP) {
        t = readFptrArgs(t, name);
    } else {
        id(toki);
        *name = tok[toki++];
    }
    return t;
}

void printType(int t) {
    printf("%d: ", t);
    if(t < 0) { printf("T%d\n", t); return; }
    struct datatype *d = &types[t];
    int i;
    switch(d->typetype) {
    case TPTR: printf("*%d", d->type); break;
    case STRUCT: case UNION:
        if(d->typetype == STRUCT) printf("struct");
        else printf("union");
        printf(" {%s:%d", toString(d->e[0]), d->e[1]);
        if(d->e[2]) printf("[%d]", d->e[2]);
        for(i = 1; i < d->ne; i++) {
            printf(", %s:%d", toString(d->e[i*3]), d->e[i*3+1]);
            if(d->e[i*3+2]) printf("[%d]", d->e[i*3+2]);
        }
        printf("}");
        break;
    case TFUN:
        printf("fun(");
        if(d->ne > 1) printf("%d", d->e[1]);
        for(int i = 2; i < d->ne; i++)
            printf(", %d", d->e[i]);
        printf(") -> %d", d->e[0]);
        break;
    case TNEXT: printf("uh-oh");
    }
    printf("\n");
}

int typeSize(int t) {
    if(t < 0) return 1;
    struct datatype *d = &types[t];
    if(d->typetype == TPTR) return 1;
    if(d->typetype == TFUN) return 1;
    if(d->typetype == UNION) {
        int max = 0;
        for(int i = 0; i < d->ne; i++) {
            int sz = typeSize(d->e[i*3+1]);
            if(d->e[i*3+2]) sz *= d->e[i*3+2];
            if(sz > max) max = sz;
        }
        return max;
    }
    if(d->typetype == STRUCT) {
        int total = 0;
        for(int i = 0; i < d->ne; i++) {
            int sz = typeSize(d->e[i*3+1]);
            if(d->e[i*3+2]) sz *= d->e[i*3+2];
            total += sz;
        }
        return total;
    }
    err("undefined type");
}

int atomicSize(int t) {
    int sz = typeSize(t);
    if(sz != 1) err("invalid type for assignment");
    return sz;
}

int evalTern(int *n, int *t);

int evalValue(int *n, int *t) {
    int ti = toki, nd = ndata;
    switch(tok[toki++]) {
    case SIZEOF:
        expect(LP);
        if(isTypeStart()) {
            *t = TINT; *n = typeSize(readType())*2;
        } else err("expected type");
        closing(RP);
        break;
    case NOT:
        if(!evalValue(n, t)) { toki--; return 0; }
        *n = !*n;
        break;
    case INV:
        if(!evalValue(n, t)) { toki--; return 0; }
        *n = ~*n;
        break;
    case M:
        if(!evalValue(n, t)) { toki--; return 0; }
        *n = -*n;
        break;
    case LP:
        if(isTypeStart()) {
            int ta = readType();
            closing(RP);
            if(ta >= 0 && types[ta].typetype != TPTR) {
                toki = ti; ndata = nd; return 0;
            }
            if(!evalValue(n, t)) { toki = ti; ndata = nd; return 0; }
            *t = ta;
        } else {
            if(!evalTern(n, t)) { toki = ti; ndata = nd; return 0; }
            if(tok[toki] != RP) { toki = ti; ndata = nd; return 0; }
            toki++;
        }
        break;
    default:
        {
            int i, k;
            char *s;
            k = tok[toki-1];
            *t = TINT;
            if((i = findConstant(k)) != -1) {
                *n = constants[i].a;
            } else if((i = findGlobal(k)) != -1 && findLocal(k) == -1) {
                struct global *g = &globals[i];
                *n = g->a; *t = g->dt;
                if(g->gt == TARR) *t = pointer(g->dt);
                else if(*t >= 0 && types[*t].typetype == TFUN
                        && tok[toki] != LP)
                    *t = pointer(*t);
                else { toki = ti; ndata = nd; return 0; }
            } else if((i = findConstant(k)) != -1) {
                *n = constants[i].a;
            } else if((s = toString(k))[0] == '"') {
                *n = ndata+DORG;
                *t = pointer(TINT);
                compileString(s+1);
            } else if(*s == '\'') {
                *n = extractChar(s+1);
            } else if(!number(s, n)) {
                toki = ti; ndata = nd; return 0;
            }
            switch(k = tok[toki]) {
            default: if(k < PE || k > E) break;
            case PP: case MM: case LP: case LB:
                toki = ti; ndata = nd; return 0;
            }
            break;
        }
    }
    return 1;
}

int doOp(int op, int *n, int *t, int a, int ta) {
    switch(op) {
    case MUL: *n *= a; break;
    case DIV:
        if(!a) return 0;
        if(*t != TINT || ta != TINT)
            *n = (unsigned)*n / (unsigned)a;
        else *n /= a;
        break;
    case REM:
        if(!a) return 0;
        if(*t != TINT || ta != TINT)
            *n = (unsigned)*n % (unsigned)a;
        else *n %= a;
        break;
    case P:
    case M:
        if(*t < 0 && ta >= 0) {
            *n *= typeSize(types[ta].type);
            *t = ta;
        } else if(*t >= 0 && ta < 0) {
            ta *= typeSize(types[*t].type);
        }
        if(op == P) *n += a;
        else *n -= a;
        break;
    case SHR:
        if(*t != TINT || ta != TINT) {
            *n = (unsigned)*t >> (unsigned)ta;
            if(*t == INT) *t = TUINT;
        } else *n >>= ta;
        break;
    case SHL: *n <<= a; break;
    case LT: *t = TINT; *n = *n < a; break;
    case GT: *t = TINT; *n = *n > a; break;
    case LTE: *t = TINT; *n = *n <= a; break;
    case GTE: *t = TINT; *n = *n >= a; break;
    case EE: *t = TINT; *n = *n == a; break;
    case NE: *t = TINT; *n = *n != a; break;
    case AND: *n &= a; break;
    case XOR: *n ^= a; break;
    case OR: *n |= a; break;
    case AAND: *t = TINT; *n = *n && a; break;
    case OOR: *t = TINT; *n = *n || a; break;
    }
    return 1;
}

int precedence(int op) {
    switch(op) {
    case MUL: case DIV: case REM: return 1;
    case P: case M: return 2;
    case SHL: case SHR: return 3;
    case LT: case GT: case LTE: case GTE: return 4;
    case EE: case NE: return 5;
    case AND: return 6;
    case XOR: return 7;
    case OR: return 8;
    case AAND: return 9;
    case OOR: return 10;
    case TERN: return 11;
    default: return 0;
    }
}

int evalTern(int *n, int *t);

int evalOp(int p, int *n, int *t) {
    int ti = toki;
    if(!p) return evalValue(n, t);
    if(p == 11) return evalTern(n, t);
    if(!evalOp(p-1, n, t)) { toki = ti; return 0; }
    if(p == 10 || p == 9) {
        if(precedence(tok[toki]) == p) {
            ti = toki;
            int op = tok[toki++];
            int a, ta;
            if(evalOp(p, &a, &ta) && doOp(op, n, t, a, ta));
            else { toki = ti; return 1; }
        }
    } else while(precedence(tok[toki]) == p) {
        ti = toki;
        int op = tok[toki++];
        int a, ta;
        if(evalOp(p-1, &a, &ta) && doOp(op, n, t, a, ta));
        else { toki = ti; return 1; }
    }
    return 1;
}

int evalTern(int *n, int *t) {
    int ti = toki;
    if(!evalOp(10, n, t)) { toki = ti; return 0; }
    if(tok[toki] == TERN) {
        int a, b, ta, tb;
        toki++;
        if(!evalTern(&a, &ta)) { toki = ti; return 0; }
        expect(COL);
        if(!evalTern(&b, &tb)) { toki = ti; return 0; }
        if(*n) { *n = a; *t = ta; }
        else { *n = b; *t = tb; }
    }
    /*int k = tok[toki];
    if(k != EOF && k <= MM) return 0;
    if(k == LP || k == LB) return 0;*/
    return 1;
}

short evalExpr() {
    int n, t;
    if(!evalTern(&n, &t)) err("failed to evaluate constant");
    return n;
}

int ins(int op, int ra, int rb) {
    return 0x8000|op<<8|ra<<4|rb;
}

int mins(int op, int ra, int b) {
    return 0xa000|op<<8|ra<<4|b;
}

void opLit(int o, int rn, short l) {
    if(l >= 0 && l < 15) text[ntext++] = mins(o, rn, l);
    else if(l < 0) { text[ntext++] = ~l; text[ntext++] = mins(o, rn, 15); }
    else { text[ntext++] = l; text[ntext++] = ins(o, rn, 0); }
}

int llocal(int rn, int i) {
    struct local *l = &locals[i];
    if(l->dim) {
        opLit(29, rn, l->a+loffs);
        return pointer(l->t);
    }
    if(l->t >= 0 && types[l->t].typetype == STRUCT) {
        opLit(29, rn, l->a+loffs);
    } else {
        opLit(30, rn, l->a+loffs);
    }
    return l->t;
}

void compileLitR(int rn, int n) {
    if(n >= -14 && n < 0) text[ntext++] = mins(26, rn, ~n);
    else opLit(0, rn, n);
}

int lglobal(int rn, int i) {
    struct global *g = &globals[i];
    if(g->gt == TARR) { compileLitR(rn, g->a); return pointer(g->dt); }
    if(g->dt >= 0) {
        switch(types[g->dt].typetype) {
        case STRUCT: case TFUN:
            compileLitR(rn, g->a); return g->dt;
        }
    }
    opLit(2, rn, g->a);
    return g->dt;
}

int rb(int rn) {
    if(rn == 13) err("register overflow");
    return rn+1;
}

int isR0Load(int n) {
    return (n&0xf) == 0 || (n&0x200f) == 0x200f;
}

int isRLoad(int n) {
    return !(isR0Load(n) || (n&0x1f00) == 0x1e00);
}

void storeLval(int rn, int a) {
    int o = text[a]&0x9f00;
    if(o != 0x8200 && o != 0x9e00)
        err("expected lvalue");
    if(isR0Load(o=text[a])) text[ntext++] = text[a-1];
    text[ntext++] = (o&0xff0f)+0x0100|rn<<4;
}

int typesOk(int ta, int tb) {
    if(ta >= 0 && types[ta].typetype != TPTR) return 0;
    if(tb >= 0 && types[tb].typetype != TPTR) return 0;
    if(ta == tb) return 1;
    if(ta == TVOID || tb == TVOID) return 0;
    if(tb < 0 && ta < 0) return 1;
    if(ta < 0) return 1;
    if(types[ta].type == TVOID) return 1;
    if(tb >= 0 && types[tb].type == TVOID) return 1;
    if(tb < 0) return 1;
    return 0;
}

int whichType(int o, int ta, int tb) {
    if(ta >= 0 && types[ta].typetype == TFUN) ta = pointer(ta);
    if(tb >= 0 && types[tb].typetype == TFUN) tb = pointer(tb);
    if(o == M && ta == tb && ta >= 0 && types[ta].typetype == TPTR) {
        return TINT;
    } else if(o == P || o == M) {
        if(ta >= 0 && types[ta].typetype == TPTR && ta == tb);
        else if(ta >= 0 && types[ta].typetype == TPTR && tb >= 0
                || tb >= 0 && types[tb].typetype == TPTR && ta >= 0)
            errv("invalid types for ", o);
    } else if(precedence(o) == 4 || precedence(o) == 5
            || o == E || o == RETURN) {
        if(typesOk(ta, tb));
        else if(ta != tb) errv("expected matching types for ", o);
        else if(types[ta].typetype != TPTR) errv("invalid type for ", o);
        if(o == RETURN) return ta;
    } else if(ta >= 0 || tb >= 0) errv("expected arithmetic types for ", o);
    if(ta == TVOID || tb == TVOID) err("expected type, got void");
    if(ta == tb) return ta;
    if(ta >= 0) return ta;
    if(tb >= 0) return tb;
    return TUINT;
}

int op(int o, int ta, int tb) {
    int u = (ta != TINT || tb != TINT);
    switch(o) {
    case LT: return 16+u;
    case GT: return 18+u;
    case GTE: return 20+u;
    case LTE: return 22+u;
    case EE: return 24;
    case NE: return 25;
    case AND: return 5;
    case OR: return 6;
    case XOR: return 7;
    case SHR: return 8+u;
    case SHL: return 10;
    case P: return 1;
    case M: return 4;
    case MUL: return 11;
    case DIV: return 12+u;
    case REM: return 14+u;
    default: errv("invalid operator ", o);
    }
}

int typeMul(int ta, int tb) {
    if(ta < 0 && tb >= 0) return typeMul(tb, ta);
    if(ta >= 0 && tb < 0 && types[ta].typetype == TPTR)
        return typeSize(types[ta].type);
    return 1;
}

int structMember(int ti, int id, int *ta, int *a) {
    struct datatype *t = &types[ti];
    int m;
    m = (t->typetype == STRUCT);
    int o = 0;
    for(int i = 0; i < t->ne*3; i += 3) {
        if(t->e[i] == -1 && !t->e[i+2])
            if(structMember(t->e[i+1], id, ta, a)) {
                *a += o; return 1;
            }
        if(t->e[i] == id) {
            *a = o;
            *ta = t->e[i+1];
            if(t->e[i+2]) *ta = pointer(*ta);
            return 1;
        }
        if(m) {
            int sz = typeSize(t->e[i+1]);
            if(t->e[i+2]) sz *= t->e[i+2];
            o += sz;
        }
    }
    return 0;
}

void getStructMember(int ti, int id, int *ta, int *a) {
    int i;
    if(ti < 0 || (i = types[ti].typetype) != STRUCT && i != UNION)
        err("expected struct");
    if(!structMember(ti, id, ta, a)) err("invalid struct member");
}

int pointerType(int t) {
    if(t < 0 || types[t].typetype != TPTR) err("expected pointer");
    t = types[t].type;
    return t;
}

void der(int rn, int t) {
    int o;
    if(t >= 0 && (o = types[t].typetype) == STRUCT || o == UNION) return;
    text[ntext++] = ins(2, rn, rn);
}

int compileCom(int rn);
int compileAss(int rn);

void addR(int o, int ra, int ta, int rb, int tb) {
    if(o == M && ta == tb && ta >= 0 && types[ta].typetype == TPTR) {
        text[ntext++] = ins(4, ra, rb);
        opLit(13, ra, typeSize(types[ta].type));
        return;
    }
    int m = typeMul(ta, tb);
    if(m != 1) opLit(11, rb, m);
    text[ntext++] = ins((o == M) ? 4 : 1, ra, rb);
}

void addLit(int o, int ra, int ta, int n, int tb) {
    if(n) {
        n *= typeMul(ta, tb);
        opLit((o == M) ? 4 : 1, ra, n);
        if(o == M && ta == tb && ta >= 0 && types[ta].typetype == TPTR)
            opLit(13, ra, typeSize(types[ta].type));
    }
}

int compileValue(int rn, int te) {
    int ti = toki, nd = ndata, nt = ntext;
    int n, t = tok[toki++];
    if(t == LP) {
        t = compileCom(rn); closing(RP);
    } else if((n = findLocal(t)) != -1) {
        t = llocal(rn, n);
    } else if((n = findGlobal(t)) != -1) {
        t = lglobal(rn, n);
    } else { toki--; errv("expected value, got ", t); }
    for(;;) {
        int o = tok[toki++];
        if(te && toki >= te) return t;
        switch(o) {
        case LP:
            {
                int ti0;
                ndata = nd; ntext = nt;
                if(t >= 0 && types[t].typetype == TPTR &&
                        (ti0 = types[t].type) >= 0
                        && types[ti0].typetype == TFUN) {
                    t = ti0;
                }
                ti0 = toki;
                if(t < 0 || types[t].typetype != TFUN) {
                    toki -= 2; err("expected function");
                }
                struct datatype *d = &types[t];
                if(rn != 1) text[ntext++] = mins(4, 14, rn-1);
                for(int i = 1; i < rn; i++) text[ntext++] = mins(31, i, i-1);
                loffs += rn-1;
                char var = 0;
                if(n = (tok[toki] != RP)) for(n = 1;; n++) {
                    if(n > 12) err("register overflow");
                    int ta = compileAss(n);
                    if(var);
                    else if(n >= d->ne) err("too many arguments");
                    else if(d->e[n] == TVAR) var = 1;
                    else if(!typesOk(d->e[n], ta)) {
                        toki--; err("argument has wrong type");
                    }
                    if(tok[toki] == RP) break;
                    expect(COM);
                }
                if(n < d->ne-1 && d->e[n+1] != TVAR)
                    err("not enough arguments");
                int ti1 = toki+1; toki = ti;
                compileValue(n = rb(n), ti0); toki = ti1;
                if(text[ntext-1] == mins(0, n, 15)) {
                    text[ntext-1] = mins(28, 13, 15);
                } else if((text[ntext-1]&0xfff0) == mins(0, n, 0)) {
                    text[ntext-1] = mins(28, 13, text[ntext-1]&0xf);
                } else if(text[ntext-1] == ins(0, n, 0)) {
                    text[ntext-1] = ins(28, 13, 0);
                } else text[ntext++] = ins(28, 13, n);
                t = d->e[0];
                if(rn != 1 && t != TVOID) text[ntext++] = ins(0, rn, 1);
                for(int i = rn-1; i >= 1; i--)
                    text[ntext++] = mins(30, i, i-1);
                if(rn != 1) text[ntext++] = mins(1, 14, rn-1);
                loffs -= rn-1;
                break;
            }
        case LB:
            {
                int tb;
                if(evalTern(&n, &tb)) addLit(P, rn, t, n, tb);
                else {
                    tb = compileCom(rb(rn));
                    addR(P, rn, t, rn+1, tb);
                }
                closing(RB);
                t = pointerType(t);
                der(rn, t);
                t = whichType(P, t, tb);
                break;
            }
        case PP: case MM:
           {
                n = toki;
                ndata = nd; ntext = nt; toki = ti;
                compileValue(rb(rn), n);
                text[n = ntext-1] -= 0x0010;
                int r = rn+1+isRLoad(text[n]);
                text[ntext++] = ins(0, r, rn);
                addLit((o == PP) ? P : M, r, t, 1, TINT);
                storeLval(r, n);
                break;
            }
        case DOT: case AR:
            for(int a = 0;;) {
                int b;
                if(o == AR) t = pointerType(t);
                getStructMember(t, tok[toki], &t, &b); toki++;
                a += b;
                if(tok[toki] != DOT) {
                    addLit(P, rn, TINT, a, TINT);
                    der(rn, t);
                    break;
                } else o = tok[toki++];
            }
            break;
        default: toki--; return t;
        }
    }
}

int lpointer(int t) {
    int n = text[ntext-1]&0xdf00;
    if(n == 0x8200) {
        text[ntext-1] &= ~0x1f00;
    } else if(n == 0x9e00) {
        text[ntext-1] -= 0x0100;
    } else err("expected lvalue");
    return pointer(t);
}

int compilePre(int rn) {
    int t, n;
    //if(evalValue(&n, &t)) { compileLitR(rn, n); return t; }
    switch(t = tok[toki++]) {
    case P: return compilePre(rn);
    case M: t = compilePre(rn); text[ntext++] = ins(27, rn, rn); break;
    case NOT:
        compilePre(rn); text[ntext++] = mins(24, rn, 0); t = TINT; break;
    case INV: t = compilePre(rn); text[ntext++] = ins(26, rn, rn); break;
    case PP: case MM:
        {
            int tb = compilePre(rb(rn));
            text[n = ntext-1] -= 0x0010;
            addLit((t == PP) ? P : M, rn, tb, 1, TINT);
            storeLval(rn, n);
            t = tb;
            break;
        }
    case MUL:
        t = compilePre(rn);
        t = pointerType(t);
        der(rn, t);
        break;
    case AND:
        t = compilePre(rn);
        if(t >= 0 && types[t].typetype == STRUCT) t = pointer(t);
        else t = lpointer(t);
        break;
    case LP:
        if(isTypeStart()) {
            n = readType(); closing(RP);
            t = compilePre(rn);
            t = n;
            return t;
        }
    default:
        toki--; return compileValue(rn, 0);
    }
    return t;
}

void jz(int from, int to) {
    text[from] = (to-1-from)&0xfff|0xe000;
}

void jnz(int from, int to) {
    text[from] = (to-1-from)&0xfff|0xf000;
}

void testR(int rn) {
    if((text[ntext-1]&0xc0f0) != (0x8000|rn<<4))
        text[ntext++] = ins(0, rn, rn);
}

int compileOp(int p, int rn) {
    int n, t;
    if(!p) return compilePre(rn);
    if(tok[toki] != LP && evalOp(p, &n, &t)) {
        compileLitR(rn, n);
        if((n = precedence(tok[toki])) && n < p) p = n;
    } else t = compileOp(p-1, rn);
    while(precedence(tok[toki]) == p) {
        int o = tok[toki++];
        int n, tb;
        switch(p) {
        case 11:
            testR(rn);
            n = ntext++;
            t = compileOp(11, rn);
            jz(n, ntext+2);
            n = ntext++; ntext++;
            expect(COL);
            tb = compileOp(11, rn);
            if(t < 0 && tb < 0 && t != TVOID && tb != TVOID) t = TUINT;
            else if(t != tb) err("expected matching type");
            jnz(n, ntext); jz(n+1, ntext);
            return t;
        case 10:
            testR(rn);
            n = ntext++;
            compileOp(9, rn);
            testR(rn);
            tb = ntext++;
            compileLitR(rn, 0);
            ntext++;
            jnz(n, ntext);
            jnz(tb, ntext);
            compileLitR(rn, 1);
            jz(ntext-2, ntext);
            t = TINT;
            break;
        case 9:
            testR(rn);
            n = ntext++;
            compileOp(8, rn);
            testR(rn);
            tb = ntext++;
            compileLitR(rn, 1);
            ntext++;
            jz(n, ntext);
            jz(tb, ntext);
            compileLitR(rn, 0);
            jnz(ntext-2, ntext);
            t = TINT;
            break;
        case 2:
            if(/*evalOp(2, &n, &tb) ||*/ evalOp(1, &n, &tb))
                addLit(o, rn, t, n, tb);
            else { tb = compileOp(1, rb(rn)); addR(o, rn, t, rn+1, tb); }
            t = whichType(o, t, tb);
            break;
        default:
            if(evalOp(p, &n, &tb) || evalOp(p-1, &n, &tb)) {
                t = whichType(o, t, tb);
                opLit(op(o, t, tb), rn, n);
            } else {
                tb = compileOp(p-1, n = rb(rn));
                text[ntext++] = ins(op(o, t, tb), rn, n);
                t = whichType(o, t, tb);
            }
            break;
        }
    }
    return t;
}

int compileAss(int rn) {
    int ti = toki, nt = ntext, nd = ndata;
    int t = compileOp(11, rn);
    int o;
    if((o = tok[toki]) >= PE && o <= E) {
        toki++; ntext = nt; ndata = nd;
        if(o == E) {
            whichType(E, t, compileAss(rn));
            nt = toki; toki = ti;
            compileOp(11, rb(rn));
            storeLval(rn, --ntext);
            if(isR0Load(text[ntext-1])) {
                ntext--; text[ntext-1] = text[ntext];
            }
            toki = nt;
        } else {
            toki = ti;
            compileOp(11, rb(rn)); toki++;
            text[nd = ntext-1] -= 0x0010;
            int n, tb;
            o -= PE;
            if(evalTern(&n, &tb)) {
                if(o == P || o == M) addLit(o, rn, t, n, tb);
                else opLit(op(o, t, tb), rn, n);
            } else {
                n = rb(rn+1);
                tb = compileAss(n);
                if(o == P || o == M) addR(o, rn, t, n, tb);
                else text[ntext++] = ins(op(o, t, tb), rn, n);
            }
            storeLval(rn, nd);
            whichType(o, t, tb);
        }
    }
    return t;
}

int compileCom(int rn) {
    int t;
    for(;;) {
        t = compileAss(rn);
        if(tok[toki] != COM) break;
        toki++;
    }
    return t;
}

void setAll(int i, int a, int b) {
    for(; i < ntext; i++) if(text[i] == a) text[i] = b;
}

int countCases() {
    int d = 0;
    int n = 0;
    for(int i = toki; tok[i] != EOF; i++) {
        int t = tok[i];
        if(t == LC) d++;
        else if(t == RC) { if(!d--) return n; }
        else if(!d && t == CASE) n++;
    }
    return -1;
}

void compileStatement() {
    switch(tok[toki++]) {
    case LC:
        {
            int n = nlocals; int a = locala;
            while(tok[toki] != RC) compileStatement();
            if(locala > maxlocala) maxlocala = locala;
            nlocals = n; locala = a;
            toki++;
            break;
        }
    case SEMI:
        break;
    case DW:
        text[ntext++] = evalExpr();
        break;
    case BREAK:
        text[ntext++] = BRK; text[ntext++] = ins(0, 15, 0);
        expect(SEMI); break;
    case CONTINUE:
        text[ntext++] = CONT; text[ntext++] = ins(0, 15, 0);
        expect(SEMI); break;
    case RETURN:
        {
            int t;
            if(tok[toki] != SEMI) {
                t = compileCom(1); expect(SEMI);
            } else { toki++; t = TVOID; }
            toki -= 2; whichType(RETURN, t, rtype); toki += 2;
            text[ntext++] = RET;
            text[ntext++] = ins(0, 15, 0);
            break;
        }
    case FOR:
        {
            expect(LP);
            int n = nlocals, a = locala;
            if(tok[toki] == SEMI) toki++;
            else compileStatement();
            int a0 = ntext;
            if(tok[toki] == SEMI) { compileLitR(1,1); toki++; }
            else { compileCom(1); testR(1); expect(SEMI); }
            int a1 = ntext++;
            int nd = ndata, nt = ntext, ti = toki;
            if(tok[toki] == RP) toki++;
            else { compileCom(1); closing(RP); }
            ntext = nt; ndata = nd;
            compileStatement();
            setAll(a1, CONT, ntext);
            nt = toki; toki = ti;
            if(tok[toki] != RP) compileCom(1);
            toki = nt;
            text[ntext++] = a0;
            text[ntext++] = ins(0, 15, 0);
            setAll(a1, BRK, ntext);
            jz(a1, ntext);
            if(locala > maxlocala) maxlocala = locala;
            nlocals = n; locala = a;
            break;
        }
    case IF:
        {
            expect(LP);
            compileCom(1); testR(1); closing(RP);
            int a = ntext++;
            compileStatement();
            if(tok[toki] == ELSE) {
                toki++;
                jz(a, ntext+2);
                a = ntext++;
                text[ntext++] = ins(0, 15, 0);
                compileStatement();
                text[a] = ntext;
            } else jz(a, ntext);
            break;
        }
    case ELSE:
        toki--; expect(IF);
    case SWITCH:
        {
            int nloc = nlocals, loca = locala;
            expect(LP); compileCom(1); expect(RP); expect(LC);
            int a = ntext;
            ntext += countCases()*3+1;
            int o = 0;
            int da = 0;
            int t;
            while((t = tok[toki]) != RC) {
                if(t == CASE) {
                    toki++;
                    int n = evalExpr(); expect(COL);
                    int l = n-o;
                    if(l < 0) { text[a++] = ~l; text[a++] = mins(4, 1, 15); }
                    else {  text[a++] = l; text[a++] = ins(4, 1, 0); }
                    o += l; 
                    jz(a++, ntext);
                } else if(t == DEFAULT) {
                    toki++; expect(COL); da = ntext;
                } else compileStatement();
            }
            toki++;
            jnz(a++, da ? da : ntext);
            setAll(a, BRK, ntext);
            if(loca > maxlocala) maxlocala = loca;
            nlocals = nloc; locala = loca;
            break;
        }
    case WHILE:
        {
            int a0 = ntext;
            expect(LP); compileCom(1); testR(1); closing(RP);
            int a1 = ntext++;
            compileStatement();
            text[ntext++] = a0;
            text[ntext++] = ins(0, 15, 0);
            jz(a1, ntext);
            setAll(a0, BRK, ntext);
            setAll(a0, CONT, a0);
            break;
        }
    case DO:
        {
            int a = ntext;
            compileStatement();
            setAll(a, CONT, ntext);
            expect(WHILE); expect(LP);
            compileCom(1); testR(1); closing(RP); expect(SEMI);
            jnz(ntext++, a);
            setAll(a, BRK, ntext);
            break;
        }
    case GOTO:
        {
            text[ntext++] = GO; id(toki);
            int i = findPair(labels, nlabels, tok[toki++]);
            expect(SEMI);
            if(i == -1) {
                i = nlabels++;
                labels[i].sym = tok[toki-2];
                labels[i].a = 0;
            }
            text[ntext++] = i;
            break;
        }
    default:
        if(tok[toki] == COL) {
            id(toki-1);
            int i = findPair(labels, nlabels, tok[toki-1]);
            if(i != -1) {
                if(labels[i].a) {
                    toki--; err("label already defined");
                }
            } else {
                i = nlabels++;
                labels[i].sym = tok[toki-1];
            }
            labels[i].a = ntext;
            toki++;
            break;
        }
        toki--;
        if(isTypeStart()) {
            int t = readType();
            for(;;) {
                struct local *l = &locals[nlocals++];
                if(tok[toki] == LP) {
                    l->t = readName(t, &l->sym);
                    l->a = locala;
                    l->dim = 0;
                    locala += typeSize(l->t);
                } else {
                    id(toki);
                    l->sym = tok[toki++];
                    l->a = locala;
                    l->t = t;
                    if(tok[toki] == LB) {
                        toki++; l->dim = evalExpr(); closing(RB);
                        locala += typeSize(t)*l->dim;
                    } else {
                        l->dim = 0;
                        locala += typeSize(t);
                    }
                }
                if(!l->dim && tok[toki] == E) {
                    int i, ol = tok[i=--toki]; tok[toki] = l->sym;
                    compileAss(1);
                    tok[i] = ol;
                }
                if(tok[toki] == SEMI) break;
                expect(COM);
            }
            toki++;
        } else {
            compileCom(1);
            expect(SEMI);
        }
        break;
    }
}

void endif(FILE *fp) {
    int n = 0;
    for(;;) {
        readTokens(fp);
        if(tok[0] == ENDIF) { if(!n--) break; }
        else if(tok[0] == MIF || tok[0] == IFNDEF || tok[0] == IFDEF) n++;
        else if(tok[0] == EOF)
            erra(linen[0], "expected #endif before EOF");
    }
}

int compileGlobal(int t) {
    struct global *g = &globals[nglobals];
    if(tok[toki] == LP) {
        t = readFptrArgs(t, &g->sym);
    } else {
        id(toki);
        g->sym = tok[toki++];
    }
    int sz = typeSize(t);
    g->dt = t;
    int dim = -1;
    if(tok[toki] == LB) {
        toki++;
        g->gt = TARR;
        if(tok[toki] == RB) {
            g->a = ndata+DORG;
            if(t >= 0 && types[t].typetype != TPTR)
                err("wrong type for data declaration");
            toki++; expect(E); expect(LC);
            int i = ndata;
            ndata += (dim = countEls(RC))*sz;
            for(;;) {
                data[i] = evalExpr();
                i += sz;
                if(tok[toki] == RC) break;
                expect(COM);
                if(tok[toki] == RC) break;
            }
            toki++;
        } else {
            g->a = nbss;
            nbss += (dim = evalExpr())*sz; closing(RB);
        }
    } else if(tok[toki] == E) {
        g->gt = TSINGLE; g->a = ndata+DORG;
        toki++;
        if(t >= 0 && types[t].typetype != TPTR)
            err("wrong type for data declaration");
        ndata += sz;
        data[g->a-DORG] = evalExpr();
    } else {
        g->gt = TSINGLE; g->a = nbss;
        nbss += sz;
    }
    int i;
    for(i = 0; i < nglobals; i++)
        if(globals[i].sym == g->sym) {
            if(globals[i].gt == TEXTERN) {
                if(globals[i].dt != g->dt || dim && !globals[i].a);
                else break;
            }
            terrh();
            printf("%s already defined\n", toString(g->sym)); exit(1);
        }
    if(i >= nglobals) nglobals++;
    else g = &globals[i];
    if(tok[toki] == COM) { toki++; compileGlobal(t); }
    else expect(SEMI);
    return i;
}

void printGlobal(struct global *g) {
    printf("%s(", toString(g->sym));
    switch(g->gt) {
    case TSINGLE: printf("VAR"); break;
    case TARR: printf("VAR[]"); break;
    case TFUN: printf("FUN"); break;
    default: printf("???"); break;
    }
    printf("): %d @ %d\n", g->dt, g->a);
}

int compileFunction(int t) {
    rtype = t;
    nlabels = 0;
    id(toki);
    struct global *g = &globals[nglobals];
    fname = g->sym = tok[toki++]; toki++;
    g->gt = TFUN;
    nlocals = 0; locala = 1;
    g->a = ntext;
    char var = 0;
    int aa = ntext;
    text[ntext++] = 0;
    text[ntext++] = ins(4, 14, 0);
    text[ntext++] = mins(31, 13, 0);
    if(tok[toki] != RP) for(;;) {
        if(tok[toki] == DOT) {
            toki++; after(DOT, DOT); after(DOT, DOT);
            var = 1; expect(RP); toki--; break;
        }
        text[ntext++] = mins(31, nlocals+1, locala);
        struct local *l = &locals[nlocals++];
        l->dim = 0;
        l->t = readName(readType(), &l->sym);
        l->a = locala;
        if(tok[toki] == LB) {
            l->t = pointer(l->t); toki++; after(LB, RB);
        }
        locala += atomicSize(l->t);
        if(tok[toki] == RP) break;
        expect(COM);
    } toki++;
    if(var) {
        locala = 13;
        for(int i = nlocals+1; i < 13; i++)
            text[ntext++] = mins(31, i, i);
    }
    maxlocala = locala;
    struct datatype *d = nextType();
    g->dt = d-types;
    d->typetype = TFUN;
    d->e = &els[nels]; nels += (d->ne = nlocals+1+var);
    for(int i = 0; i < nlocals; i++) d->e[i+1] = locals[i].t;
    if(var) d->e[nlocals+1] = TVAR;
    d->e[0] = t;
    if(tok[toki] == SEMI) { g->gt = TEXFUN; toki++; }
    for(int i = 0; i < ntypes; i++)
        if(&types[i] != d && sameType(&types[i], d)) {
            d->typetype = TNEXT;
            g->dt = i;
            nels -= d->ne;
            if(ntypes-1 == d-types) ntypes--;
            break;
        }
    int i;
    for(i = 0; i < nglobals; i++)
        if(globals[i].sym == g->sym)
            if(globals[i].gt == TEXFUN || g->gt == TEXFUN) {
                if(globals[i].dt != g->dt) {
                    errv("wrong type for ", g->sym);
                }
                break;
            }
    if(g->gt == TEXFUN) {
        if(i >= nglobals) { g->a = ndata+DORG; ndata += 2; nglobals++; }
        return i;
    }
    if(i < nglobals) {
        g = &globals[i];
        g->gt = TFUN;
        data[g->a-DORG] = aa;
        data[g->a-DORG+1] = ins(0, 15, 0);
        g->a = aa;
    } else nglobals++;
    compileStatement();
    setAll(g->a, RET, ntext);
    text[ntext++] = mins(30, 13, 0);
    text[aa] = maxlocala;
    opLit(1, 14, maxlocala);
    text[ntext++] = ins(0, 15, 13);
    for(int i = g->a; i < ntext; i++)
        if(text[i] == GO) {
            int j = text[i+1];
            if(!labels[j].a) {
                j = labels[j].sym;
                for(toki = 0; tok[toki] != EOF; toki++)
                    if(tok[toki] == GOTO && tok[++toki] == j) break;
                errv("undefined label ", j);
            }
            text[i] = labels[j].a;
            text[++i] = ins(0, 15, 0);
        }
    fname = 0;
    return g-globals;
}

void hideStatic(struct pair *sta) {
    for(int i = 0; i < nstatc; i++) {
        sta[i].sym = globals[sta[i].a = statc[i]].sym;
        globals[sta[i].a].sym = EOF;
    }
    nstatc = 0;
}

void showStatic(struct pair *sta, int n) {
    for(int i = 0; i < nstatc; i++)
        globals[statc[i]].sym = EOF;
    for(int i = 0; i < n; i++)
        globals[sta[i].a].sym = sta[i].sym;
    nstatc = n;
}

int compileFile(char *name) {
    filename = name;
    lineno = 1;
    int nif = 0;
    FILE *fp = fopen(name, "r");
    for(int i = 0; !fp && i < nincdirs; i++) {
        char buf[120];
        sprintf(buf, "%s/%s", incdirs[i], name);
        fp = fopen(buf, "r");
    }
    if(!fp) return 1;
    struct pair sta[40]; int nsta = nstatc;
    hideStatic(sta);
    for(;;) {
        readTokens(fp);
        toki = 0;
        if(tok[0] == EOF) break;
        if(tok[0] == DEFINE) {
            id(toki = 1);
            int n;
            if(tok[2] == EOF) n = 0;
            else { toki = 2; n = evalExpr(); }
            constants[nconstants].sym = tok[1];
            constants[nconstants++].a = n;
        } else if(tok[0] == MIF) {
            toki = 1;
            if(!evalExpr()) endif(fp);
            else nif++;
        } else if(tok[0] == IFDEF) {
            if(findConstant(tok[1]) == -1) endif(fp);
            else nif++;
        } else if(tok[0] == IFNDEF) {
            if(findConstant(tok[1]) != -1) endif(fp);
            else nif++;
        } else if(tok[0] == ENDIF) {
            if(!nif--) erra(linen[0], "unexpected #endif");
        } else if(tok[0] == ENUM) {
            toki = 1;
            after(ENUM, LC);
            for(;;) {
                if(tok[toki] == RC) break;
                id(toki);
                constants[nconstants].sym = tok[toki++];
                if(tok[toki] == E) { toki++; enumn = evalExpr(); }
                constants[nconstants++].a = enumn++;
                if(tok[toki] == RC) break;
                expect(COM);
            }
            toki++;
            after(RC, SEMI);
        } else if(tok[0] == INCLUDE) {
            int ln = lineno, rt = rtype, un = fname;
            char *fn = filename;
            if(toki = compileFile(toString(tok[1])+1)) {
                filename = fn; errv("failed to include ", tok[1]);
            }
            filename = fn; lineno = ln; rtype = rt; fname = un;
        } else if(tok[0] == TYPEDEF) {
            toki = 1;
            typedefs[ntypedefs].a =
                readName(readType(), &typedefs[ntypedefs].sym);
            ntypedefs++;
            expect(SEMI);
        } else {
            char st = 0;
            if(tok[toki] == STATIC) { toki++; st = 1; }
            int t = readType();
            if(tok[toki] != SEMI) {
                int i;
                if(tok[toki+1] == LP) i = compileFunction(t);
                else i = compileGlobal(t);
                if(st) statc[nstatc++] = i;
            }
        }
    }
    if(nif) erra(linen[0], "expected #endif before EOF");
    fclose(fp);
    showStatic(sta, nsta);
    return 0;
}

void option(char *cmd, char *op) {
    if(!strcmp(cmd, "o")) {
        outfile = op;
    } else if(!strcmp(cmd, "I")) {
        incdirs[nincdirs++] = op;
    } else {
        printf("unknown option -%s\n", cmd); exit(1);
    }
}

void prepareSave() {
    for(int i = 0; i < nglobals; i++) {
        struct global *g = &globals[i];
        if(g->gt == TEXTERN) errv("global was not defined: ", g->sym);
        if(g->gt == TEXFUN) errv("function was not defined: ", g->sym);
    }
    int i = findGlobal(toSymbol("main"));
    if(i == -1) err("main not defined");
    int n;
    if((n = globals[i].dt) < 0 || types[n].typetype != TFUN)
        err("main should be function");
    text[0] = globals[i].a;
}

void saveFile(const char *filename) {
    prepareSave();
    FILE *fp = fopen(filename, "wb");
    if(!fp) {
        printf("failed to open output file %s\n", filename); exit(1);
    }
    int a = 0;
    fwrite(&a, 2, 1, fp);
    fwrite(&ntext, 2, 1, fp);
    fwrite(text, 2, ntext, fp);
    a = DORG;
    fwrite(&a, 2, 1, fp);
    fwrite(&ndata, 2, 1, fp);
    fwrite(data, 2, ndata, fp);
    fclose(fp);
}

int main(int argc, char **args) {
    int argc1 = 1;
    constants[nconstants++].sym = toSymbol("VCC");
    for(int i = 1; i < argc; i++)
        if(*args[i] == '-') {
            option(args[i]+1, (i+1 == argc) ? "" : args[i+1]); i++;
        } else args[argc1++] = args[i];
    if(argc1 == 1) return 1;
    ntext++;
    text[ntext++] = ins(28, 13, 0);
    text[ntext++] = mins(0, 1, 0);
    text[ntext++] = 0xc000;
    incdirs[nincdirs++] = "vnlib";
    for(int i = 1; i < argc1; i++)
        if(compileFile(args[i])) {
            printf("failed to open input file %s\n", args[i]);
            return 1;
        }
    saveFile(outfile);
    printf("wrote %d bytes\n", (ntext+ndata)*2+8);
    return 0;
}

