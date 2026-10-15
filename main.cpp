// ============================================================================
//  DELD Virtual Breadboard Lab  -  single file, only <iostream> and <graphics.h>
//  Works in a small window (e.g. 640x480): the view follows the action,
//  A/D/W/X pans the view.
//  Colours (permanent):  HIGHLIGHT = RED , DEFAULT WIRE = GREEN
// ============================================================================
#include <iostream>
#include <graphics.h>
using namespace std;

#define HICOL    RED
#define WIRECOL  GREEN
#define BOARDCOL LIGHTGRAY

// ---------------------------------------------------------------- helpers ---
int slen(const char* s) { int i = 0; while (s[i]) i++; return i; }
void scat(char* d, const char* s) { int i = slen(d), j = 0; while (s[j]) d[i++] = s[j++]; d[i] = 0; }
void sint(char* d, int v) {
    char t[12], o[12]; int n = 0, m = 0;
    if (v < 0) { scat(d, "-"); v = -v; }
    if (v == 0) t[n++] = '0';
    while (v > 0) { t[n++] = '0' + v % 10; v /= 10; }
    while (n > 0) o[m++] = t[--n];
    o[m] = 0; scat(d, o);
}
// text at fixed SCREEN position
void txt(int x, int y, const char* s) {
    char b[256]; int i = 0;
    while (s[i] && i < 255) { b[i] = s[i]; i++; }
    b[i] = 0; outtextxy(x, y, b);
}

// ------------------------------------------------- camera / view window ------
// Virtual breadboard is 1200 x 720. The screen shows a window of it.
int VW = 640, VH = 388, VY0 = 72;      // view width, height, top (screen y)
int camX = 0, camY = 112;              // virtual coords shown at the view's top-left
int CX1 = 0, CX2 = 639, CY1 = 72, CY2 = 459;

void setView() {
    VW = getmaxx() + 1;
    VH = getmaxy() + 1 - VY0 - 20;
    CX1 = 0; CX2 = VW - 1; CY1 = VY0; CY2 = VY0 + VH - 1;
}
void clampCam() {
    int maxX = 1200 - VW; if (maxX < 0) maxX = 0;
    int maxY = 640 - VH;  if (maxY < 85) maxY = 85;
    if (camX < 0) camX = 0;       if (camX > maxX) camX = maxX;
    if (camY < 85) camY = 85;     if (camY > maxY) camY = maxY;
}
int sx(int x) { return x - camX; }
int sy(int y) { return y - camY + VY0; }

int outcode(int x, int y) {
    int c = 0;
    if (x < CX1) c |= 1; else if (x > CX2) c |= 2;
    if (y < CY1) c |= 4; else if (y > CY2) c |= 8;
    return c;
}
// Cohen-Sutherland clipping in screen space, then plain line()
void clipLine(int x1, int y1, int x2, int y2) {
    int o1 = outcode(x1, y1), o2 = outcode(x2, y2);
    while (true) {
        if (!(o1 | o2)) { line(x1, y1, x2, y2); return; }
        if (o1 & o2) return;
        bool first = (o1 != 0);
        int o = first ? o1 : o2, x = 0, y = 0;
        if (o & 8)      { x = x1 + (int)((long)(x2 - x1) * (CY2 - y1) / (y2 - y1)); y = CY2; }
        else if (o & 4) { x = x1 + (int)((long)(x2 - x1) * (CY1 - y1) / (y2 - y1)); y = CY1; }
        else if (o & 2) { y = y1 + (int)((long)(y2 - y1) * (CX2 - x1) / (x2 - x1)); x = CX2; }
        else            { y = y1 + (int)((long)(y2 - y1) * (CX1 - x1) / (x2 - x1)); x = CX1; }
        if (first) { x1 = x; y1 = y; o1 = outcode(x1, y1); }
        else       { x2 = x; y2 = y; o2 = outcode(x2, y2); }
    }
}
// drawing in VIRTUAL coordinates (camera applied + clipped)
void vline(int x1, int y1, int x2, int y2) { clipLine(sx(x1), sy(y1), sx(x2), sy(y2)); }
void vrect(int x1, int y1, int x2, int y2) {
    vline(x1, y1, x2, y1); vline(x2, y1, x2, y2); vline(x2, y2, x1, y2); vline(x1, y2, x1, y1);
}
void vfill(int x1, int y1, int x2, int y2, int col) {
    setcolor(col);
    for (int y = y1; y <= y2; y++) {
        int Y = sy(y); if (Y < CY1 || Y > CY2) continue;
        int a = sx(x1), b = sx(x2);
        if (a < CX1) a = CX1; if (b > CX2) b = CX2;
        if (a <= b) line(a, Y, b, Y);
    }
}
void vdot(int cx, int cy, int r, int col) {
    setcolor(col);
    for (int dy = -r; dy <= r; dy++) {
        int Y = sy(cy + dy); if (Y < CY1 || Y > CY2) continue;
        int dx = 0;
        while ((dx + 1) * (dx + 1) + dy * dy <= r * r) dx++;
        int a = sx(cx - dx), b = sx(cx + dx);
        if (a < CX1) a = CX1; if (b > CX2) b = CX2;
        if (a <= b) line(a, Y, b, Y);
    }
}
// text in virtual coordinates, drawn character by character, only if fully visible
void vtxt(int x, int y, const char* s) {
    for (int i = 0; s[i]; i++) {
        int X = sx(x) + 8 * i, Y = sy(y);
        if (X >= CX1 && X + 8 <= CX2 + 1 && Y >= CY1 && Y + 8 <= CY2 + 1) {
            char b[2]; b[0] = s[i]; b[1] = 0; outtextxy(X, Y, b);
        }
    }
}
// wrapped text in the fixed header area; returns next y
int wrapText(int y, const char* s, int col) {
    int maxc = VW / 8 - 2; if (maxc > 120) maxc = 120;
    int n = slen(s), i = 0; char b[128];
    while (i < n && y <= VY0 - 14) {
        int len = n - i;
        if (len > maxc) { len = maxc; int k = len; while (k > 0 && s[i + k] != ' ') k--; if (k > 0) len = k; }
        for (int j = 0; j < len; j++) b[j] = s[i + j];
        b[len] = 0; setcolor(col); outtextxy(10, y, b);
        y += 10; i += len; while (s[i] == ' ') i++;
    }
    return y;
}

// ------------------------------------------------------------- IC database ---
struct ChipDef { const char* ic; const char* desc; int n; const char* lab[16]; int vcc, gnd; };
enum { T7408, T7432, T7486, T7404, T7483, T74151, T7476, T7490 };
ChipDef defs[8] = {
 {"7408","Quad 2-input AND",14,{"1A","1B","1Y","2A","2B","2Y","GND","3Y","3A","3B","4Y","4A","4B","VCC"},14,7},
 {"7432","Quad 2-input OR",14,{"1A","1B","1Y","2A","2B","2Y","GND","3Y","3A","3B","4Y","4A","4B","VCC"},14,7},
 {"7486","Quad 2-input XOR",14,{"1A","1B","1Y","2A","2B","2Y","GND","3Y","3A","3B","4Y","4A","4B","VCC"},14,7},
 {"7404","Hex Inverter",14,{"1A","1Y","2A","2Y","3A","3Y","GND","4Y","4A","5Y","5A","6Y","6A","VCC"},14,7},
 {"7483","4-bit Binary Adder",16,{"A4","S3","A3","B3","VCC","S2","B2","A2","S1","A1","B1","GND","C0","C4","S4","B4"},5,12},
 {"74151","8:1 Multiplexer",16,{"D3","D2","D1","D0","Y","W","E'","GND","C","B","A","D7","D6","D5","D4","VCC"},16,8},
 {"7476","Dual JK Flip-Flop",16,{"1CK","1PR","1CL","1J","VCC","2CK","2PR","2CL","2J","2Q'","2Q","2K","GND","1Q'","1Q","1K"},5,13},
 {"7490","Decade Counter",14,{"INB","R01","R02","NC","VCC","R91","R92","QC","QB","GND","QD","QA","NC","INA"},5,10}
};

// endpoint ids: chip pin = chip*100+pin ; specials below
#define P(c,p) ((c)*100+(p))
#define SW(k)  (1000+(k))
#define LD(k)  (1100+(k))
const int VCCR = 1200, GNDR = 1201, CLKB = 1202;

struct Wire { int a, b; };
struct Pt { int x, y; bool top, freeX; };

// ================================================================ BASE CLASS ==
class Experiment {
protected:
    const char* title;
    int nc, ct[5];
    Wire w[80]; int nw;
    const char* swn[8]; int nsw;
    const char* ledn[6]; int nled;
    int swv[8], ledv[6];
    bool showClk, quit, skip, wide;
    int shown, highlight;

    void addChip(int t) { ct[nc++] = t; }
    void conn(int a, int b) { w[nw].a = a; w[nw].b = b; nw++; }
    void power() {
        for (int c = 0; c < nc; c++) {
            conn(P(c, defs[ct[c]].vcc), VCCR);
            conn(P(c, defs[ct[c]].gnd), GNDR);
        }
    }

    // ---------- geometry ----------
    void pinPos(int c, int p, int& x, int& y, bool& top) {
        ChipDef& d = defs[ct[c]]; int x0 = 30 + c * 232, half = d.n / 2;
        if (p <= half) { x = x0 + 12 + (p - 1) * 22; y = 360; top = false; }
        else           { x = x0 + 12 + (d.n - p) * 22; y = 260; top = true; }
    }
    Pt endpoint(int id) {
        Pt q; q.freeX = false; q.x = 0;
        if (id < 1000)      pinPos(id / 100, id % 100, q.x, q.y, q.top);
        else if (id < 1100) { q.x = 40 + (id - 1000) * 52; q.y = 570; q.top = false; }
        else if (id < 1200) { q.x = 560 + (id - 1100) * 60; q.y = 570; q.top = false; }
        else if (id == 1200){ q.y = 100; q.top = true;  q.freeX = true; }
        else if (id == 1201){ q.y = 505; q.top = false; q.freeX = true; }
        else                { q.x = 470; q.y = 570; q.top = false; }
        return q;
    }
    int route(int ia, int ib, int k, int* px, int* py) {
        Pt A = endpoint(ia), B = endpoint(ib);
        if (!A.top && B.top) { Pt t = A; A = B; B = t; int ti = ia; ia = ib; ib = ti; }
        int lt = 240 - (k % 12) * 4, lb = 380 + (k % 12) * 6;
        if (A.top == B.top) {
            if (A.freeX) A.x = B.x;
            if (B.freeX) B.x = A.x;
            if (A.freeX || B.freeX) { px[0]=A.x; py[0]=A.y; px[1]=B.x; py[1]=B.y; return 2; }
            int lane = A.top ? lt : lb;
            px[0]=A.x; py[0]=A.y; px[1]=A.x; py[1]=lane;
            px[2]=B.x; py[2]=lane; px[3]=B.x; py[3]=B.y; return 4;
        }
        int c = (ia < 1000) ? ia / 100 : ((ib < 1000) ? ib / 100 : 0);
        int gx = 30 + c * 232 + 186 + (k % 4) * 9;
        if (A.freeX) A.x = gx;
        if (B.freeX) B.x = gx;
        px[0]=A.x; py[0]=A.y; px[1]=A.x; py[1]=lt; px[2]=gx; py[2]=lt;
        px[3]=gx;  py[3]=lb;  px[4]=B.x; py[4]=lb; px[5]=B.x; py[5]=B.y;
        return 6;
    }
    void describe(int id, char* o) {
        if (id < 1000) {
            int c = id / 100, p = id % 100;
            scat(o, "IC"); sint(o, c + 1); scat(o, " ("); scat(o, defs[ct[c]].ic);
            scat(o, ") pin "); sint(o, p); scat(o, " ["); scat(o, defs[ct[c]].lab[p - 1]); scat(o, "]");
        }
        else if (id < 1100) { scat(o, "Switch "); scat(o, swn[id - 1000]); }
        else if (id < 1200) { scat(o, "LED ");    scat(o, ledn[id - 1100]); }
        else if (id == 1200) scat(o, "+5V rail");
        else if (id == 1201) scat(o, "GND rail");
        else scat(o, "Clock pulse button");
    }

    // ---------- camera control ----------
    void camIcs() { camX = 0; camY = 306 - VH / 2; wide = false; clampCam(); }
    void camDemo() {
        int lo = nsw > 0 ? 20 : (showClk ? 440 : 540);
        int hi = nled > 0 ? 560 + (nled - 1) * 60 + 30 : lo + 100;
        if (showClk && hi < 500) hi = 500;
        camY = 245;
        if (hi - lo <= VW) camX = lo - (VW - (hi - lo)) / 2; else camX = hi - VW + 10;
        wide = false; clampCam();
    }
    void camWire(int i) {
        int px[8], py[8]; int n = route(w[i].a, w[i].b, i, px, py);
        int mnx = px[0], mxx = px[0], mny = 240, mxy = 372;      // always include the chip band
        for (int j = 0; j < n; j++) {
            if (px[j] < mnx) mnx = px[j]; if (px[j] > mxx) mxx = px[j];
            if (py[j] < mny) mny = py[j]; if (py[j] > mxy) mxy = py[j];
        }
        wide = false;
        if (mxx - mnx + 60 <= VW) camX = (mnx + mxx) / 2 - VW / 2; else { camX = mnx - 30; wide = true; }
        if (mxy - mny + 10 <= VH) camY = (mny + mxy) / 2 - VH / 2; else { camY = mny - 10; wide = true; }
        clampCam();
    }

    // ---------- drawing ----------
    void drawBoard() {
        vfill(20, 85, 1180, 520, BOARDCOL);
        vfill(20, 303, 1180, 317, DARKGRAY);
        for (int x = 30; x < 1170; x += 22) {
            vdot(x, 260, 1, DARKGRAY); vdot(x, 360, 1, DARKGRAY);
            vdot(x, 100, 1, DARKGRAY); vdot(x, 505, 1, DARKGRAY);
        }
        setcolor(DARKGRAY); setlinestyle(SOLID_LINE, 0, THICK_WIDTH);
        vline(40, 108, 1160, 108); vline(40, 497, 1160, 497);
        setlinestyle(SOLID_LINE, 0, NORM_WIDTH);
        setcolor(BLACK);
        vtxt(24, 90, "+5V RAIL"); vtxt(24, 510, "GND RAIL");
    }
    void drawChip(int c) {
        ChipDef& d = defs[ct[c]];
        int half = d.n / 2, x0 = 30 + c * 232, w2 = half * 22 + 2;
        setcolor(BLACK); setlinestyle(SOLID_LINE, 0, THICK_WIDTH);
        for (int p = 1; p <= d.n; p++) {
            int px, py; bool tp; pinPos(c, p, px, py, tp);
            if (tp) vline(px, 260, px, 272); else vline(px, 348, px, 360);
        }
        setlinestyle(SOLID_LINE, 0, NORM_WIDTH);
        vfill(x0, 270, x0 + w2, 350, BLUE);
        setcolor(WHITE); vrect(x0, 270, x0 + w2, 350);
        vline(x0, 303, x0 + 7, 303); vline(x0 + 7, 303, x0 + 7, 317); vline(x0 + 7, 317, x0, 317);
        setcolor(YELLOW);                                   // IC number (drawn twice = bold)
        vtxt(x0 + (w2 - slen(d.ic) * 8) / 2, 295, d.ic);
        vtxt(x0 + (w2 - slen(d.ic) * 8) / 2 + 1, 295, d.ic);
        setcolor(WHITE);
        vtxt(x0 + (w2 - slen(d.desc) * 8) / 2, 312, d.desc);
        for (int p = 1; p <= d.n; p++) {
            int px, py; bool tp; pinPos(c, p, px, py, tp);
            char nb[8]; nb[0] = 0; sint(nb, p);
            setcolor(BLACK); vtxt(px - slen(nb) * 4, tp ? 248 : 364, nb);
            setcolor(LIGHTCYAN);
            int row = (p % 2) ? 0 : 9;
            vtxt(px - slen(d.lab[p - 1]) * 4, tp ? 273 + row : 337 - row, d.lab[p - 1]);
            vdot(px, py, 3, DARKGRAY);
        }
    }
    void drawWire(int i, int col) {
        int px[8], py[8]; int n = route(w[i].a, w[i].b, i, px, py);
        setcolor(col); setlinestyle(SOLID_LINE, 0, THICK_WIDTH);
        for (int j = 0; j < n - 1; j++) vline(px[j], py[j], px[j + 1], py[j + 1]);
        setlinestyle(SOLID_LINE, 0, NORM_WIDTH);
        vdot(px[0], py[0], 4, col); vdot(px[n - 1], py[n - 1], 4, col);
    }
    void drawDevices() {
        setcolor(WHITE);
        if (nsw > 0) vtxt(40, 540, "INPUT SWITCHES");
        if (showClk) vtxt(440, 540, "CLOCK");
        if (nled > 0) vtxt(560, 540, "OUTPUT LEDs");
        for (int k = 0; k < nsw; k++) {
            int x = 40 + k * 52;
            vdot(x, 570, 4, WHITE); setcolor(WHITE); vline(x, 574, x, 585);
            vfill(x - 15, 585, x + 15, 607, swv[k] ? LIGHTGREEN : DARKGRAY);
            setcolor(WHITE); vrect(x - 15, 585, x + 15, 607);
            setcolor(swv[k] ? BLACK : WHITE); vtxt(x - 4, 592, swv[k] ? "1" : "0");
            setcolor(WHITE); vtxt(x - slen(swn[k]) * 4, 615, swn[k]);
        }
        if (showClk) {
            int x = 470;
            vdot(x, 570, 4, WHITE); setcolor(WHITE); vline(x, 574, x, 585);
            vfill(x - 25, 585, x + 25, 607, LIGHTBLUE);
            setcolor(WHITE); vrect(x - 25, 585, x + 25, 607); vtxt(x - 12, 592, "CLK");
        }
        for (int k = 0; k < nled; k++) {
            int x = 560 + k * 60;
            vdot(x, 570, 4, WHITE); setcolor(WHITE); vline(x, 574, x, 586);
            vdot(x, 598, 12, ledv[k] ? YELLOW : DARKGRAY);
            setcolor(ledv[k] ? BLACK : WHITE); vtxt(x - 4, 594, ledv[k] ? "1" : "0");
            setcolor(WHITE); vtxt(x - slen(ledn[k]) * 4, 618, ledn[k]);
        }
    }
    void render(const char* l1, const char* l2) {
        setbkcolor(BLACK); cleardevice();
        // board area first, then the fixed header/footer over it
        drawBoard();
        for (int c = 0; c < nc; c++) drawChip(c);
        for (int i = 0; i < shown; i++) drawWire(i, (i == highlight) ? HICOL : WIRECOL);
        drawDevices();
        // header (fixed)
        setcolor(BLACK);
        for (int y = 0; y < VY0 - 2; y++) line(0, y, VW - 1, y);
        for (int y = VY0 + VH + 2; y < VY0 + VH + 22; y++) line(0, y, VW - 1, y);
        setcolor(LIGHTCYAN); txt(10, 2, title);
        int y = wrapText(12, l1, YELLOW);
        wrapText(y, l2, WHITE);
        setcolor(LIGHTGRAY);
        line(0, VY0 - 2, VW - 1, VY0 - 2); line(0, VY0 + VH + 1, VW - 1, VY0 + VH + 1);
        if (wide) { setcolor(YELLOW); txt(10, VY0 + VH + 6, "WIRE EXTENDS OFF-SCREEN: press A/D/W/X to pan (ENTER=next)"); }
        else      { setcolor(LIGHTGRAY); txt(10, VY0 + VH + 6, "ENTER next  S skip wiring  ESC menu  A/D/W/X pan view"); }
    }
    int waitEnter(const char* l1, const char* l2) {
        while (true) {
            int k = getch();
            if (k == 13 || k == 10) return 0;
            if (k == 's' || k == 'S') return 1;
            if (k == 27) return 2;
            bool pan = true;
            if (k == 'a' || k == 'A') camX -= 80;
            else if (k == 'd' || k == 'D') camX += 80;
            else if (k == 'w' || k == 'W') camY -= 60;
            else if (k == 'x' || k == 'X') camY += 60;
            else pan = false;
            if (pan) { clampCam(); render(l1, l2); }
        }
    }
    bool step(const char* l1, const char* l2) {
        cout << l1 << "\n    " << l2 << endl;
        render(l1, l2);
        int r = waitEnter(l1, l2);
        if (r == 2) quit = true;
        return !quit;
    }

    // ---------- combinational demo ----------
    virtual void eval() {}
    virtual int ncombo() { return 1 << nsw; }
    virtual int combo(int i) { return i; }
    void comboDemo() {
        int n = ncombo();
        if (!step("DEMONSTRATION: circuit is ready",
                  "Switches will be set to one input combination per ENTER; watch the LEDs.")) return;
        for (int i = 0; i < n; i++) {
            int m = combo(i);
            for (int k = 0; k < nsw; k++) swv[k] = (m >> (nsw - 1 - k)) & 1;
            eval();
            char l1[256], l2[256]; l1[0] = 0; l2[0] = 0;
            scat(l1, "TEST "); sint(l1, i + 1); scat(l1, " of "); sint(l1, n);
            for (int k = 0; k < nsw; k++) { scat(l2, swn[k]); scat(l2, "="); sint(l2, swv[k]); scat(l2, "  "); }
            scat(l2, "=>  ");
            for (int k = 0; k < nled; k++) { scat(l2, ledn[k]); scat(l2, "="); sint(l2, ledv[k]); scat(l2, "  "); }
            if (!step(l1, l2)) return;
        }
    }
public:
    Experiment() {
        nc = 0; nw = 0; nsw = 0; nled = 0; showClk = false; quit = false; skip = false; wide = false;
        shown = 0; highlight = -1;
        for (int i = 0; i < 8; i++) swv[i] = 0;
        for (int i = 0; i < 6; i++) ledv[i] = 0;
    }
    virtual ~Experiment() {}
    virtual void build() = 0;
    virtual void demo() { comboDemo(); }

    void run() {
        build();
        int gd = DETECT, gm = 0;
        char path[] = "";
        initgraph(&gd, &gm, path);
        setView();
        cout << "Graphics window: " << (getmaxx() + 1) << " x " << (getmaxy() + 1) << endl;
        shown = 0; highlight = -1;
        camIcs();
        if (step("STEP 1: Place the required ICs on the breadboard",
                 "Each block shows the IC number, pin numbers and pin names (pin 1 = bottom-left, notch on left).")) {
            for (int i = 0; i < nw && !skip && !quit; i++) {
                char l1[256], l2[256]; l1[0] = 0; l2[0] = 0;
                scat(l1, "STEP "); sint(l1, i + 2); scat(l1, " of "); sint(l1, nw + 1);
                scat(l1, " : make this connection (shown in RED)");
                scat(l2, "FROM  "); describe(w[i].a, l2); scat(l2, "   TO   "); describe(w[i].b, l2);
                shown = i + 1; highlight = i;
                camWire(i);
                cout << l1 << "\n    " << l2 << endl;
                render(l1, l2);
                int r = waitEnter(l1, l2);
                if (r == 1) skip = true;
                if (r == 2) quit = true;
            }
            highlight = -1; shown = nw;
            if (!quit) {
                camDemo();
                demo();
                if (!quit) step("EXPERIMENT COMPLETE", "Press ENTER to close this window and return to the menu.");
            }
        }
        closegraph();
    }
};

// ===================================================== 1. FULL ADDER =========
class FullAdder : public Experiment {
public:
    void build() {
        title = "Exp 1: Full Adder using Logic Gates";
        addChip(T7486); addChip(T7408); addChip(T7432); power();
        swn[0] = "A"; swn[1] = "B"; swn[2] = "Cin"; nsw = 3;
        ledn[0] = "Sum"; ledn[1] = "Cout"; nled = 2;
        conn(SW(0), P(0,1)); conn(SW(1), P(0,2)); conn(P(0,3), P(0,4)); conn(SW(2), P(0,5)); conn(P(0,6), LD(0));
        conn(SW(0), P(1,1)); conn(SW(1), P(1,2)); conn(P(0,3), P(1,4)); conn(SW(2), P(1,5));
        conn(P(1,3), P(2,1)); conn(P(1,6), P(2,2)); conn(P(2,3), LD(1));
    }
    void eval() { int a = swv[0], b = swv[1], c = swv[2];
        ledv[0] = a ^ b ^ c; ledv[1] = (a & b) | ((a ^ b) & c); }
};

// ================================================ 2. BINARY TO GRAY ==========
class BinGray : public Experiment {
public:
    void build() {
        title = "Exp 2: 4-bit Binary to Gray Code Converter";
        addChip(T7486); power();
        swn[0]="B3"; swn[1]="B2"; swn[2]="B1"; swn[3]="B0"; nsw = 4;
        ledn[0]="G3"; ledn[1]="G2"; ledn[2]="G1"; ledn[3]="G0"; nled = 4;
        conn(SW(0), P(0,1)); conn(SW(1), P(0,2)); conn(SW(1), P(0,4)); conn(SW(2), P(0,5));
        conn(SW(2), P(0,9)); conn(SW(3), P(0,10));
        conn(SW(0), LD(0)); conn(P(0,3), LD(1)); conn(P(0,6), LD(2)); conn(P(0,8), LD(3));
    }
    void eval() { ledv[0]=swv[0]; ledv[1]=swv[0]^swv[1]; ledv[2]=swv[1]^swv[2]; ledv[3]=swv[2]^swv[3]; }
};

// ================================================== 3. BCD ADDER =============
class BcdAdder : public Experiment {
public:
    void build() {
        title = "Exp 3: BCD Adder using 4-bit Binary Adder IC 7483";
        addChip(T7483); addChip(T7483); addChip(T7408); addChip(T7432); power();
        swn[0]="A3"; swn[1]="A2"; swn[2]="A1"; swn[3]="A0";
        swn[4]="B3"; swn[5]="B2"; swn[6]="B1"; swn[7]="B0"; nsw = 8;
        ledn[0]="Cout"; ledn[1]="D3"; ledn[2]="D2"; ledn[3]="D1"; ledn[4]="D0"; nled = 5;
        conn(SW(0),P(0,1)); conn(SW(1),P(0,3)); conn(SW(2),P(0,8)); conn(SW(3),P(0,10));
        conn(SW(4),P(0,16)); conn(SW(5),P(0,4)); conn(SW(6),P(0,7)); conn(SW(7),P(0,11));
        conn(GNDR,P(0,13));
        // correction logic: K = C4 + S4.S3 + S4.S2
        conn(P(0,15),P(2,1)); conn(P(0,2),P(2,2)); conn(P(0,15),P(2,4)); conn(P(0,6),P(2,5));
        conn(P(2,3),P(3,1)); conn(P(2,6),P(3,2)); conn(P(3,3),P(3,4)); conn(P(0,14),P(3,5));
        // second adder adds 0110 when K=1
        conn(P(0,15),P(1,1)); conn(P(0,2),P(1,3)); conn(P(0,6),P(1,8)); conn(P(0,9),P(1,10));
        conn(GNDR,P(1,16)); conn(P(3,6),P(1,4)); conn(P(3,6),P(1,7)); conn(GNDR,P(1,11)); conn(GNDR,P(1,13));
        conn(P(3,6),LD(0)); conn(P(1,15),LD(1)); conn(P(1,2),LD(2)); conn(P(1,6),LD(3)); conn(P(1,9),LD(4));
    }
    int ncombo() { return 6; }
    int combo(int i) { static int pa[6][2] = {{3,4},{5,6},{7,8},{9,9},{0,0},{9,1}}; return (pa[i][0] << 4) | pa[i][1]; }
    void eval() {
        int a = swv[0]*8+swv[1]*4+swv[2]*2+swv[3], b = swv[4]*8+swv[5]*4+swv[6]*2+swv[7];
        int raw = a + b, s = raw & 15, c4 = raw >> 4;
        int s4 = (s >> 3) & 1, s3 = (s >> 2) & 1, s2 = (s >> 1) & 1;
        int K = c4 | (s4 & s3) | (s4 & s2);
        int r = (s + (K ? 6 : 0)) & 15;
        ledv[0] = K; ledv[1] = (r >> 3) & 1; ledv[2] = (r >> 2) & 1; ledv[3] = (r >> 1) & 1; ledv[4] = r & 1;
    }
};

// ============================================ 4. MULTIPLEXER (74151) =========
class MuxExp : public Experiment {
public:
    void build() {
        title = "Exp 4: F(X,Y,Z) = Sum m(1,3,5,6) using 8:1 MUX 74151";
        addChip(T74151); power();
        swn[0]="X"; swn[1]="Y"; swn[2]="Z"; nsw = 3; ledn[0]="F"; nled = 1;
        conn(SW(0),P(0,9)); conn(SW(1),P(0,10)); conn(SW(2),P(0,11)); conn(GNDR,P(0,7));
        conn(VCCR,P(0,3)); conn(VCCR,P(0,1)); conn(VCCR,P(0,14)); conn(VCCR,P(0,13));   // D1 D3 D5 D6 = 1
        conn(GNDR,P(0,4)); conn(GNDR,P(0,2)); conn(GNDR,P(0,15)); conn(GNDR,P(0,12));   // D0 D2 D4 D7 = 0
        conn(P(0,5),LD(0));
    }
    void eval() { int i = swv[0]*4+swv[1]*2+swv[2]; ledv[0] = (i==1||i==3||i==5||i==6) ? 1 : 0; }
};

// ============================================== 5. 2-BIT COMPARATOR ==========
class Comparator : public Experiment {
public:
    void build() {
        title = "Exp 5: 2-bit Magnitude Comparator using Logic Gates";
        addChip(T7486); addChip(T7404); addChip(T7408); addChip(T7432); power();
        swn[0]="A1"; swn[1]="A0"; swn[2]="B1"; swn[3]="B0"; nsw = 4;
        ledn[0]="A>B"; ledn[1]="A=B"; ledn[2]="A<B"; nled = 3;
        conn(SW(0),P(0,1)); conn(SW(2),P(0,2)); conn(SW(1),P(0,4)); conn(SW(3),P(0,5));
        conn(P(0,3),P(1,1)); conn(P(0,6),P(1,3)); conn(SW(2),P(1,5)); conn(SW(3),P(1,9));
        conn(P(1,2),P(2,1)); conn(P(1,4),P(2,2));
        conn(SW(0),P(2,4)); conn(P(1,6),P(2,5)); conn(SW(1),P(2,9)); conn(P(1,8),P(2,10));
        conn(P(1,2),P(2,12)); conn(P(2,8),P(2,13));
        conn(P(2,6),P(3,1)); conn(P(2,11),P(3,2)); conn(P(3,3),P(3,4)); conn(P(2,3),P(3,5));
        conn(P(3,6),P(1,11));
        conn(P(3,3),LD(0)); conn(P(2,3),LD(1)); conn(P(1,10),LD(2));
    }
    void eval() { int a = swv[0]*2+swv[1], b = swv[2]*2+swv[3];
        ledv[0] = a > b; ledv[1] = a == b; ledv[2] = a < b; }
};

// ====================================== 6. PARITY GENERATOR AND CHECKER ======
class Parity : public Experiment {
public:
    void build() {
        title = "Exp 6: Even Parity Generator and Checker";
        addChip(T7486); addChip(T7486); power();
        swn[0]="A"; swn[1]="B"; swn[2]="C"; swn[3]="Err"; nsw = 4;
        ledn[0]="Par"; ledn[1]="Flag"; nled = 2;
        conn(SW(0),P(0,1)); conn(SW(1),P(0,2)); conn(P(0,3),P(0,4)); conn(SW(2),P(0,5)); conn(P(0,6),LD(0));
        conn(P(0,6),P(1,1)); conn(SW(3),P(1,2));             // error injection: Prx = P xor Err
        conn(SW(2),P(0,9)); conn(P(1,3),P(0,10));            // C xor Prx
        conn(P(0,3),P(0,12)); conn(P(0,8),P(0,13));          // (A^B) xor (C^Prx)
        conn(P(0,11),LD(1));
    }
    void eval() { int p = swv[0]^swv[1]^swv[2]; int prx = p ^ swv[3];
        ledv[0] = p; ledv[1] = swv[0]^swv[1]^swv[2]^prx; }
};

// ================================================ 7. RIPPLE COUNTER ==========
class Ripple : public Experiment {
public:
    void build() {
        title = "Exp 7: 2-bit Ripple Counter using JK Flip-Flop (7476)";
        addChip(T7476); power(); showClk = true;
        ledn[0]="Q0"; ledn[1]="Q1"; nled = 2;
        conn(CLKB,P(0,1));
        conn(VCCR,P(0,4)); conn(VCCR,P(0,16)); conn(VCCR,P(0,2)); conn(VCCR,P(0,3));
        conn(VCCR,P(0,9)); conn(VCCR,P(0,12)); conn(VCCR,P(0,7)); conn(VCCR,P(0,8));
        conn(P(0,15),P(0,6)); conn(P(0,15),LD(0)); conn(P(0,11),LD(1));
    }
    void demo() {
        int q0 = 0, q1 = 0; ledv[0] = 0; ledv[1] = 0;
        if (!step("DEMONSTRATION: J=K=1 on both flip-flops, negative-edge triggered",
                  "Initial state Q1 Q0 = 0 0 (after CLEAR). Each ENTER = one clock pulse.")) return;
        for (int n = 1; n <= 8; n++) {
            if (q0 == 1) q1 ^= 1;
            q0 ^= 1; ledv[0] = q0; ledv[1] = q1;
            char l1[256], l2[256]; l1[0] = 0; l2[0] = 0;
            scat(l1, "Clock pulse "); sint(l1, n); scat(l1, " applied");
            scat(l2, "Q1 Q0 = "); sint(l2, q1); scat(l2, " "); sint(l2, q0);
            scat(l2, "   (decimal "); sint(l2, q1 * 2 + q0); scat(l2, ")");
            if (!step(l1, l2)) return;
        }
    }
};

// ============================================= 8. UP/DOWN COUNTER ============
class UpDown : public Experiment {
public:
    void build() {
        title = "Exp 8: Synchronous 2-bit Up/Down Counter (JK, mode M)";
        addChip(T7476); addChip(T7486); power(); showClk = true;
        swn[0] = "M"; nsw = 1; ledn[0]="Q0"; ledn[1]="Q1"; nled = 2;
        conn(CLKB,P(0,1)); conn(CLKB,P(0,6));
        conn(VCCR,P(0,2)); conn(VCCR,P(0,3)); conn(VCCR,P(0,7)); conn(VCCR,P(0,8));
        conn(VCCR,P(0,4)); conn(VCCR,P(0,16));                 // J0 = K0 = 1
        conn(SW(0),P(1,1)); conn(P(0,15),P(1,2));              // M xor Q0
        conn(P(1,3),P(0,9)); conn(P(1,3),P(0,12));             // J1 = K1 = M xor Q0
        conn(P(0,15),LD(0)); conn(P(0,11),LD(1));
    }
    void demo() {
        int q0 = 0, q1 = 0; swv[0] = 0; ledv[0] = 0; ledv[1] = 0;
        if (!step("DEMONSTRATION: J0=K0=1 ; J1=K1 = M xor Q0 ; M=0 counts UP, M=1 counts DOWN",
                  "Initial state Q1 Q0 = 0 0, mode M = 0 (UP). Each ENTER = one clock pulse.")) return;
        for (int n = 1; n <= 12; n++) {
            if (n == 7) {
                swv[0] = 1;
                if (!step("Mode switch M changed to 1  ->  DOWN counting", "J1 = K1 = M xor Q0")) return;
            }
            int t1 = swv[0] ^ q0; q0 ^= 1; if (t1) q1 ^= 1;
            ledv[0] = q0; ledv[1] = q1;
            char l1[256], l2[256]; l1[0] = 0; l2[0] = 0;
            scat(l1, "Clock pulse "); sint(l1, n); scat(l1, swv[0] ? "  (DOWN mode)" : "  (UP mode)");
            scat(l2, "Q1 Q0 = "); sint(l2, q1); scat(l2, " "); sint(l2, q0);
            scat(l2, "   (decimal "); sint(l2, q1 * 2 + q0); scat(l2, ")");
            if (!step(l1, l2)) return;
        }
    }
};

// ========================================= 9. MODULO-N COUNTER (7490) ========
class ModN : public Experiment {
    void setLeds(int n) { ledv[0]=(n>>3)&1; ledv[1]=(n>>2)&1; ledv[2]=(n>>1)&1; ledv[3]=n&1; }
public:
    void build() {
        title = "Exp 9: Modulo-6 Counter using Decade Counter IC 7490";
        addChip(T7490); power(); showClk = true;
        ledn[0]="QD"; ledn[1]="QC"; ledn[2]="QB"; ledn[3]="QA"; nled = 4;
        conn(CLKB,P(0,14));            // clock to INA
        conn(P(0,12),P(0,1));          // QA to INB  (BCD counting)
        conn(P(0,9),P(0,2));           // QB to R0(1)
        conn(P(0,8),P(0,3));           // QC to R0(2)  -> reset at 0110 = 6
        conn(GNDR,P(0,6)); conn(GNDR,P(0,7));   // R9 inputs low
        conn(P(0,11),LD(0)); conn(P(0,8),LD(1)); conn(P(0,9),LD(2)); conn(P(0,12),LD(3));
    }
    void demo() {
        int n = 0; setLeds(0);
        if (!step("DEMONSTRATION: QA->INB, R0(1)=QB, R0(2)=QC. Count 0..5 then resets at 6 (0110)",
                  "Initial count = 0000. Each ENTER = one clock pulse.")) return;
        for (int p = 1; p <= 13; p++) {
            n++;
            char l1[256], l2[256]; l1[0] = 0; l2[0] = 0;
            if (n == 6) {
                setLeds(6);
                if (!step("Count reached 6 (QD QC QB QA = 0 1 1 0)",
                          "QB = QC = 1  ->  R0(1) = R0(2) = 1  ->  counter is RESET to 0 immediately")) return;
                n = 0;
            }
            setLeds(n);
            scat(l1, "Clock pulse "); sint(l1, p);
            scat(l2, "Count = "); sint(l2, n); scat(l2, "   (QD QC QB QA = ");
            sint(l2, ledv[0]); sint(l2, ledv[1]); sint(l2, ledv[2]); sint(l2, ledv[3]); scat(l2, ")");
            if (!step(l1, l2)) return;
        }
    }
};

// =========================== 10. SEQUENCE GENERATOR AND DETECTOR (101) =======
class SeqGenDet : public Experiment {
    int xval(int g) { return ((g >> 1) & 1) | ((g & 1) ? 0 : 1); }   // X = Q1 + Q0'  -> 1,0,1,1
    void upd(int g, int s) {
        int x = xval(g), q1 = (s >> 1) & 1, q0 = s & 1;
        ledv[0] = x; ledv[1] = q1 & x; ledv[2] = q1; ledv[3] = q0;
    }
public:
    void build() {
        title = "Exp 10: Sequence Generator (1011) and Detector (101) using JK FF";
        addChip(T7476); addChip(T7476); addChip(T7404); addChip(T7408); addChip(T7432);
        power(); showClk = true;
        ledn[0]="X"; ledn[1]="OUT"; ledn[2]="Q1"; ledn[3]="Q0"; nled = 4;
        // ---- generator: 2-bit synchronous counter in IC1, X = Q1 + Q0'
        conn(CLKB,P(0,1)); conn(CLKB,P(0,6));
        conn(VCCR,P(0,2)); conn(VCCR,P(0,3)); conn(VCCR,P(0,7)); conn(VCCR,P(0,8));
        conn(VCCR,P(0,4)); conn(VCCR,P(0,16));
        conn(P(0,15),P(0,9)); conn(P(0,15),P(0,12));
        conn(P(0,15),P(2,1)); conn(P(0,11),P(4,1)); conn(P(2,2),P(4,2));
        // ---- detector (Mealy, overlapping 101): J0=X K0=X' J1=Q0.X' K1=1 OUT=Q1.X
        conn(CLKB,P(1,1)); conn(CLKB,P(1,6));
        conn(VCCR,P(1,2)); conn(VCCR,P(1,3)); conn(VCCR,P(1,7)); conn(VCCR,P(1,8));
        conn(P(4,3),P(1,4)); conn(P(4,3),P(2,3)); conn(P(2,4),P(1,16));
        conn(P(1,15),P(3,1)); conn(P(2,4),P(3,2)); conn(P(3,3),P(1,9)); conn(VCCR,P(1,12));
        conn(P(1,11),P(3,4)); conn(P(4,3),P(3,5));
        conn(P(4,3),LD(0)); conn(P(3,6),LD(1)); conn(P(1,11),LD(2)); conn(P(1,15),LD(3));
    }
    void demo() {
        int g = 0, s = 0; upd(g, s);
        if (!step("DEMONSTRATION: generator repeats 1,0,1,1 ; detector looks for pattern 101",
                  "Initial state: generator 00, detector Q1 Q0 = 00. Each ENTER = one clock pulse.")) return;
        for (int n = 1; n <= 12; n++) {
            int x = xval(g), q0 = s & 1;
            int nq0 = x, nq1 = q0 & (x ? 0 : 1);
            s = nq1 * 2 + nq0; g = (g + 1) & 3;
            upd(g, s);
            char l1[256], l2[256]; l1[0] = 0; l2[0] = 0;
            scat(l1, "Clock pulse "); sint(l1, n);
            scat(l2, "X = "); sint(l2, ledv[0]); scat(l2, "   Detector Q1 Q0 = ");
            sint(l2, ledv[2]); sint(l2, ledv[3]); scat(l2, "   OUT = "); sint(l2, ledv[1]);
            if (ledv[1]) scat(l2, "   <<< 101 DETECTED >>>");
            if (!step(l1, l2)) return;
        }
    }
};

// ===================================================================== MAIN ==
int main() {
    int ch;
    while (true) {
        cout << "\n=========== DELD VIRTUAL BREADBOARD LAB ===========\n";
        cout << " 1  Full Adder using logic gates\n";
        cout << " 2  Binary to Gray code converter\n";
        cout << " 3  BCD Adder using 4-bit binary adder IC 7483\n";
        cout << " 4  Boolean expression using Multiplexer (74151)\n";
        cout << " 5  2-bit comparator using logic gates\n";
        cout << " 6  Parity generator and checker\n";
        cout << " 7  2-bit ripple counter using JK flip-flop\n";
        cout << " 8  Synchronous 2-bit Up/Down counter with mode control\n";
        cout << " 9  Modulo-N counter using decade counter IC 7490\n";
        cout << "10  Sequence generator and detector using JK flip-flop\n";
        cout << " 0  Exit\n";
        cout << "Enter your choice: ";
        if (!(cin >> ch)) break;
        if (ch == 0) break;
        Experiment* e = 0;
        switch (ch) {
            case 1:  e = new FullAdder();  break;
            case 2:  e = new BinGray();    break;
            case 3:  e = new BcdAdder();   break;
            case 4:  e = new MuxExp();     break;
            case 5:  e = new Comparator(); break;
            case 6:  e = new Parity();     break;
            case 7:  e = new Ripple();     break;
            case 8:  e = new UpDown();     break;
            case 9:  e = new ModN();       break;
            case 10: e = new SeqGenDet();  break;
            default: cout << "Invalid choice, try again.\n";
        }
        if (e) {
            cout << "\nGraphics window opening... click on it, then ENTER = next step, A/D/W/X = pan.\n";
            e->run();
            delete e;
        }
    }
    cout << "Goodbye!\n";
    return 0;
}