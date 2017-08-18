#include "leds_ctrl.h"
#include "sw_ctrl.h"
#include "btn_ctrl.h"
#include "plot.h"
#include "delay.h"

#define sgn(x) ((x<0)?-1:((x>0)?1:0))
#define RESOLUTION_X 640
#define RESOLUTION_Y 480

void line(int x1, int y1, int x2, int y2, unsigned int color)
{
    int i,dx,dy,sdx,sdy,dxabs,dyabs,x,y,px,py;

    dx=x2-x1;      /* the horizontal distance of the line */
    dy=y2-y1;      /* the vertical distance of the line */
    dxabs=abs(dx);
    dyabs=abs(dy);
    sdx=sgn(dx);
    sdy=sgn(dy);
    x=dyabs>>1;
    y=dxabs>>1;
    px=x1;
    py=y1;

    plot(color,px,py);
    if (dxabs>=dyabs) /* the line is more horizontal than vertical */
    {
        for(i=0; i<dxabs; i++)
        {
            y+=dyabs;
            if (y>=dxabs)
            {
                y-=dxabs;
                py+=sdy;
            }
            px+=sdx;
            plot(color,px,py);
        }
    }
    else /* the line is more vertical than horizontal */
    {
        for(i=0; i<dyabs; i++)
        {
            x+=dxabs;
            if (x>=dyabs)
            {
                x-=dyabs;
                px+=sdx;
            }
            py+=sdy;
            plot(color,px,py);
        }
    }
}

void Rect(int left,int top, int right, int bottom, unsigned int color)
{
    line(left,top,right,top,color);
    line(left,top,left,bottom,color);
    line(right,top,right,bottom,color);
    line(left,bottom,right,bottom,color);
}
void RectFill(int left,int top, int right, int bottom, unsigned int color)
{
    int currentline;
    for (currentline=top; currentline<=bottom; currentline++) {
        line(left,currentline,right,currentline,color);
    }
}




void Circle(int x0, int y0, int radius, unsigned int color)
{
    int x = radius, y = 0;
    int radiusError = 1-x;

    while(x >= y)
    {
        plot(color,x + x0, y + y0);
        plot(color,y + x0, x + y0);
        plot(color,-x + x0, y + y0);
        plot(color,-y + x0, x + y0);
        plot(color,-x + x0, -y + y0);
        plot(color,-y + x0, -x + y0);
        plot(color,x + x0, -y + y0);
        plot(color,y + x0, -x + y0);
        y++;
        if(radiusError<0)
            radiusError+=2*y+1;
        else
        {
            x--;
            radiusError+=2*(y-x+1);
        }
    }
}

void CircleFill(int x0, int y0, int radius, unsigned int color)
{
    int x = radius, y = 0;
    int radiusError = 1-x;

    while(x >= y)
    {
        line(x + x0,y + y0,-x + x0, y + y0,color);
        line(y + x0, x + y0,y + x0, -x + y0,color);
        line(-y + x0, x + y0,-y + x0, -x + y0,color);
        line(-x + x0, -y + y0,x + x0, -y + y0,color);
        y++;
        if(radiusError<0)
            radiusError+=2*y+1;
        else
        {
            x--;
            radiusError+=2*(y-x+1);
        }
    }
}


void plot_test() {
    int x,y,k;
    int radius=10;

#ifdef C_SIMULATION
    for (x=0; x<1; x++) {
#else
    for (x=0; x<RESOLUTION_X; x++) {
#endif
#ifdef C_SIMULATION
        for (y=0; y<1; y++) {
#else
        for (y=0; y<RESOLUTION_Y; y++) {
#endif
            unsigned short color = sw_ctrl();
            unsigned char dir = btn_ctrl();
            if(color==0) color = 7;
            if(color>7) color = 7;
            if(dir==BUTTON_UP) ++radius;
            else if(dir==BUTTON_DOWN) --radius;
            if(radius < 8) radius = 8;
            if(radius > 31) radius = 31;
            leds_ctrl(x);
            leds_ctrl(y);
            Circle(x,y,radius,color);
#ifdef C_SIMULATION
            delay(1);
#else
            delay(1000000);
#endif
            Circle(x,y,radius,0);
        }
    }
}

void main() {
    plot_test();
}
