#include <tamtypes.h>
#include <kernel.h>
#include <libpad.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <math.h>
#include <string.h>

#define SCREEN_W 640
#define SCREEN_H 448

static GSGLOBAL *gs;
static unsigned char padBuf[256] __attribute__((aligned(64)));
static padButtonStatus pad;
static u64 C_SKY, C_HILL, C_HILL2, C_GRASS, C_ROAD, C_EDGE, C_WHITE;
static u64 C_YELLOW, C_RED, C_BLUE, C_DARK, C_CAR, C_GLASS;

static const unsigned char font5x7[40][7] = {
 {0,0,0,0,0,0,0}, /* space */
 {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17}, {14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
 {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30}, {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
 {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14},
 {4,4,4,4,4,0,4}, {0,0,0,31,0,0,0}, {0,0,0,0,0,0,0}
};
/* Character lookup: space, A-Z, 0-9, colon, hyphen, period. */
static int glyphIndex(char c) {
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c == ' ') return 0;
    if (c >= 'A' && c <= 'Z') return 1 + c - 'A';
    if (c >= '0' && c <= '9') return 27 + c - '0';
    if (c == ':') return 37;
    if (c == '-') return 38;
    if (c == '.') return 39;
    return 0;
}
static u64 rgba(int r,int g,int b) { return GS_SETREG_RGBAQ(r,g,b,0x80,0); }
static void rect(float x1,float y1,float x2,float y2,u64 col) {
    gsKit_prim_sprite(gs,x1,y1,x2,y2,1,col);
}
static void quad(float x1,float y1,float x2,float y2,float x3,float y3,float x4,float y4,u64 col) {
    gsKit_prim_quad(gs,x1,y1,x2,y2,x3,y3,x4,y4,1,col);
}
static void textDraw(int x,int y,const char *s,int scale,u64 col) {
    while (*s) {
        int gi = glyphIndex(*s++);
        int row, bit;
        for (row=0; row<7; row++) {
            for (bit=0; bit<5; bit++) {
                if (font5x7[gi][row] & (1u << (4-bit)))
                    rect((float)(x+bit*scale),(float)(y+row*scale),
                         (float)(x+(bit+1)*scale),(float)(y+(row+1)*scale),col);
            }
        }
        x += 6*scale;
    }
}
static void initColors(void) {
    C_SKY=rgba(75,155,220); C_HILL=rgba(68,130,88); C_HILL2=rgba(42,100,64);
    C_GRASS=rgba(40,130,60); C_ROAD=rgba(65,68,72); C_EDGE=rgba(230,230,215);
    C_WHITE=rgba(255,255,245); C_YELLOW=rgba(255,210,40); C_RED=rgba(230,55,45);
    C_BLUE=rgba(35,95,220); C_DARK=rgba(20,25,30); C_CAR=rgba(210,35,40);
    C_GLASS=rgba(80,185,220);
}
static void drawMenu(void) {
    gsKit_clear(gs,C_SKY);
    rect(0,260,640,448,C_GRASS);
    quad(0,245,0,300,230,235,285,300,C_HILL);
    quad(400,230,390,305,640,240,640,310,C_HILL2);
    /* Stylized road receding to the horizon. */
    quad(286,230,75,448,354,230,570,448,C_ROAD);
    quad(315,230,300,448,320,230,340,448,C_EDGE);
    textDraw(100,55,"PS2 HORIZON",5,C_WHITE);
    textDraw(166,132,"OPEN ROAD",3,C_YELLOW);
    rect(150,190,490,245,C_DARK);
    textDraw(185,207,"PRESS X TO START",2,C_WHITE);
    textDraw(105,385,"ARROWS DRIVE  X ACCEL",2,C_WHITE);
    textDraw(155,412,"O BRAKE  START MENU",2,C_WHITE);
}
static void drawCar(float cx) {
    /* Low-poly, fixed chase-view car silhouette. */
    quad(cx-42,320,cx-34,375,cx+34,320,cx+42,375,C_DARK);
    quad(cx-34,308,cx-28,355,cx+28,308,cx+34,355,C_CAR);
    quad(cx-25,305,cx-18,329,cx+18,305,cx+25,329,C_GLASS);
    rect(cx-39,337,cx-29,356,C_DARK);
    rect(cx+29,337,cx+39,356,C_DARK);
    rect(cx-27,354,cx-14,362,C_YELLOW);
    rect(cx+14,354,cx+27,362,C_YELLOW);
}
static void drawWorld(float carX,float speed,float progress,int checkpoint,int remaining) {
    int i;
    float center = 320.0f + (float)sin(progress*0.014f)*48.0f - carX*0.25f;
    gsKit_clear(gs,C_SKY);
    rect(0,205,640,448,C_GRASS);
    quad(0,190,0,285,170,205,245,275,C_HILL);
    quad(350,205,330,278,640,185,640,285,C_HILL2);
    /* Perspective road with moving bends. */
    quad(center-14,205,center-230,448,center+14,205,center+230,448,C_ROAD);
    quad(center-18,205,center-244,448,center-14,205,center-230,448,C_EDGE);
    quad(center+14,205,center+230,448,center+18,205,center+244,448,C_EDGE);
    for(i=0;i<7;i++) {
        float t=(float)((i+(int)(progress/12.0f)%2)%7)/7.0f;
        float y=220.0f+t*220.0f;
        float half=10.0f+t*220.0f;
        rect(center-half*0.035f-2,y,center-half*0.035f+2,y+7+t*8,C_WHITE);
        rect(center+half*0.035f-2,y,center+half*0.035f+2,y+7+t*8,C_WHITE);
    }
    /* Checkpoint gate ahead; grows as progress approaches the next target. */
    {
        float d = (float)(checkpoint+1)*100.0f-progress;
        float size = 14.0f + (100.0f-d)*0.45f;
        float gy = 210.0f + (100.0f-d)*0.65f;
        if (d < 100.0f && d > 0.0f) {
            if(size<18) size=18;
            if(size>70) size=70;
            rect(center-size,gy,center-size+7,gy+size*1.5f,C_YELLOW);
            rect(center+size-7,gy,center+size,gy+size*1.5f,C_YELLOW);
            rect(center-size,gy,center+size,gy+7,C_YELLOW);
        }
    }
    drawCar(320.0f+carX);
    rect(12,12,245,78,C_DARK);
    textDraw(24,22,"PS2 HORIZON",2,C_WHITE);
    textDraw(24,44,"SPEED",1,C_YELLOW);
    {
        int s=(int)speed;
        char buf[20]="000";
        buf[0]=(char)('0'+(s/100)%10); buf[1]=(char)('0'+(s/10)%10); buf[2]=(char)('0'+s%10);
        textDraw(80,44,buf,2,C_WHITE);
        textDraw(128,44,"KM H",1,C_WHITE);
    }
    rect(420,12,628,78,C_DARK);
    textDraw(432,22,"MISSION 01",2,C_YELLOW);
    textDraw(432,44,"GATES",1,C_WHITE);
    rect(475,44,610,56,C_DARK);
    rect(475,44,475+checkpoint*32,56,C_YELLOW);
    if (remaining < 0) remaining=0;
    textDraw(432,61,"TIME",1,C_WHITE);
    {
        char tb[8]={'0'+(remaining/60)%10,':','0'+(remaining%60)/10,'0'+remaining%10,0};
        textDraw(475,61,tb,1,C_WHITE);
    }
    textDraw(12,420,"ARROWS STEER  X GAS  O BRAKE",1,C_WHITE);
}
int main(int argc, char *argv[]) {
    int menu=1, done=0, checkpoint=0, timeLeft=90, frame=0;
    float carX=0.0f, speed=0.0f, progress=0.0f;
    (void)argc; (void)argv;
    padInit(0);
    padPortOpen(0,0,padBuf);
    gs=gsKit_init_global();
    if(!gs) return 1;
    gs->Mode=GS_MODE_NTSC;
    gs->Interlace=GS_INTERLACED;
    gs->Field=GS_FIELD;
    gs->Width=640; gs->Height=448;
    dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF,D_CTRL_STS_UNSPEC,D_CTRL_STD_OFF,D_CTRL_RCYC_8,1<<DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    gsKit_init_screen(gs);
    gsKit_mode_switch(gs,GS_PERSISTENT);
    gsKit_set_test(gs,GS_ZTEST_OFF);
    initColors();
    while(!done) {
        int state=padGetState(0,0);
        if(state==PAD_STATE_STABLE || state==PAD_STATE_FINDCTP1) padRead(0,0,&pad);
        if(!(pad.btns & PAD_START)) { menu=1; speed=0; progress=0; checkpoint=0; timeLeft=90; }
        if(menu) {
            drawMenu();
            if(!(pad.btns & PAD_CROSS)) { menu=0; frame=0; }
        } else {
            if(!(pad.btns & PAD_LEFT)) carX-=2.0f;
            if(!(pad.btns & PAD_RIGHT)) carX+=2.0f;
            if(!(pad.btns & PAD_CROSS)) speed+=0.35f; else speed-=0.06f;
            if(!(pad.btns & PAD_CIRCLE)) speed-=0.8f;
            if(speed<0) speed=0;
            if(speed>220) speed=220;
            if(carX < -120) carX=-120;
            if(carX > 120) carX=120;
            progress += speed*0.012f;
            if(checkpoint<4 && progress >= (float)(checkpoint+1)*100.0f) checkpoint++;
            if(frame>0 && frame%60==0 && timeLeft>0) timeLeft--;
            drawWorld(carX,speed,progress,checkpoint,timeLeft);
            if(checkpoint>=4) {
                rect(120,170,520,250,C_DARK);
                textDraw(175,190,"MISSION COMPLETE",3,C_YELLOW);
                textDraw(175,225,"PRESS START FOR MENU",2,C_WHITE);
            } else if(timeLeft<=0) {
                rect(120,170,520,250,C_DARK);
                textDraw(220,190,"TIME UP",3,C_RED);
                textDraw(175,225,"PRESS START FOR MENU",2,C_WHITE);
            }
            frame++;
        }
        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }
    padPortClose(0,0);
    padEnd();
    return 0;
}
