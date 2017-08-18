#define sgn(x) ((x<0)?-1:((x>0)?1:0))
extern void plot(int color, int x, int y);
extern void leds_ctrl(unsigned int id, unsigned int val); 
extern int delay(int ritardo);
#if 1
#define RESOLUTION_X 160
#define RESOLUTION_Y 120
#else
#define RESOLUTION_X 320
#define RESOLUTION_Y 240
#endif

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
//Uncomment desired test function
# if 1
    // Multiple circle test:
#ifdef C_SIMULATION
    for (x=0; x<1; x++) {
#else
    for (x=0; x<RESOLUTION_X; x++) {
#endif
        for (y=0; y<RESOLUTION_Y; y++) {
            leds_ctrl(4,x);
            leds_ctrl(5,y);
            Circle(x,y,10,7);
            delay(500000);
            Circle(x,y,10,0);
        }
    }
#endif
# if 0
    k=0;
    //Pattern test:
    for (x=0; x<RESOLUTION_X; x++) {
        for (y=0; y<RESOLUTION_Y; y++) {
            k++;
            plot((k*x*y)%8,x,y);
        }
    }
#endif
# if 0
    //Multiple Square test:
    for (x=0; x<RESOLUTION_X; x++) {
        for (y=0; y<RESOLUTION_Y; y++) {
            Rect(x,y,x+10,y+10,4);
            delay(500000);
            Rect(x,y,x+10,y+10,0);
        }
    }
#endif
# if 0
    // Multiple circle test:
    for (x=0; x<RESOLUTION_X; x++) {
        for (y=0; y<RESOLUTION_Y; y++) {
            CircleFill(x,y,10,4);
            delay(500000);
            CircleFill(x,y,10,0);
        }
    }
#endif
# if 0
    //Multiple Square test:
    for (x=0; x<RESOLUTION_X; x++) {
        for (y=0; y<RESOLUTION_Y; y++) {
            RectFill(x,y,x+10,y+10,4);
            delay(500000);
            RectFill(x,y,x+10,y+10,0);
        }
    }
#endif
# if 0
    //Multiple Polygons test:
    for (x=0; x<RESOLUTION_X; x++) {
        for (y=0; y<RESOLUTION_Y; y++) {
            Circle(x,y,10,4);
            CircleFill(x-4,y-1,1,3);
            CircleFill(x+4,y-1,1,3);
            RectFill(x-2,y+4,x+2,y+5,2);
            delay(500000);
            Circle(x,y,10,0);
            CircleFill(x-4,y-1,1,0);
            CircleFill(x+4,y-1,1,0);
            RectFill(x-2,y+4,x+2,y+5,0);
        }
    }
#endif
    return;
}

int main() {
    plot_test();
    return 0;
}
