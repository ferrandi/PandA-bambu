/// Breakout game ported to the PandA framework by Fabrizio Ferrandi on April 11th 2016.
/// The original code has been taken from http://www.aaroncox.net/tutorials/arcade/BRICKBreaker.html
/// The game was designed by Nolan Bushnell, Steve Wozniak, and Steve Bristow. History of Breakout game can be found at this link: https://en.wikipedia.org/wiki/Breakout_%28video_game%29

#include "plot.h"
#include "btn_ctrl.h"
#include "get_ticks.h"
#include "sevensegments_ctrl.h"

// Window related defines
#define WINDOW_WIDTH   640
#define WINDOW_HEIGHT  480

// Game related defines
#define FRAMES_PER_SECOND 60
#define FRAME_RATE        1000/FRAMES_PER_SECOND 

// RGB colors
#define YELLOW_COLOR 6
#define RED_COLOR    4
#define BLUE_COLOR   1
#define GREEN_COLOR  2
#define WHITE_COLOR  7
#define BLACK_COLOR  0

// Minimum distance from the side of the screen to a BRICK
#define BRICK_SCREEN_BUFFER_X 41
#define BRICK_SCREEN_BUFFER_Y 30

// Maximum number of bricks allowed
#define NUM_ROWS   6
#define NUM_COLS    9
#define MAX_BRICKS NUM_ROWS*NUM_COLS

// Location of the player's paddle in the game
#define PLAYER_Y 430


// Dimensions of a BRICK
#define BRICK_WIDTH  62
#define BRICK_HEIGHT 16

// Dimensions of a paddle
#define PADDLE_WIDTH  32
#define PADDLE_HEIGHT 8

// Diameter of the ball
#define BALL_DIAMETER 8

// Paddle speeds
#define PLAYER_SPEED    2

// Ball speeds
#define BALL_SPEED_MODIFIER 4  // divide location on paddle by this
#define BALL_SPEED_Y        2 // max speed of ball along y axis

// Maximum number of times the player can miss the ball
#define NUM_LIVES 5

// Number of levels, increase this value to add new levels
#define NUM_LEVELS 3


#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3

#define sgn(x) ((x<0)?-1:((x>0)?1:0))

typedef struct _obj_description
{
    short int x;
    short int y;
    short int w;
    short int h;
} obj_description;

// Struct that represents an entity in our game
typedef struct _entity
{
    obj_description screen_location;  // location on screen
    int x_speed;
    int y_speed;
} entity;

typedef struct _brick
{
    obj_description screen_location; // location on screen
    int num_hits; // health
    unsigned char color; //color of the brick
} brick;

// Global data
entity player;             // The player's paddle
entity ball;               // The game ball
int    lives;              // Player's lives
int    level;              // Current level
int    num_bricks;         // Keep track of number of bricks
brick  bricks[MAX_BRICKS]; // The bricks we're breaking
int    timer;              // Game timer

unsigned char convert_char_digit_2_sseg(char c)
{
    static unsigned char table[] = {63/*0*/, 6/*1*/, 91/*2*/, 79/*3*/, 102/*4*/, 109/*5*/, 125/*6*/, 7/*7*/, 127/*8*/, 111/*9*/};
    return table[c-'0'];
}


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

void rect(int left,int top, int right, int bottom, unsigned int color)
{
    line(left,top,right,top,color);
    line(left,top,left,bottom,color);
    line(right,top,right,bottom,color);
    line(left,bottom,right,bottom,color);
}

void circle(int x0, int y0, int radius, unsigned int color)
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

void rect_fill(int left,int top, int right, int bottom, unsigned int color)
{
    int currentline;
    for (currentline=top; currentline<=bottom; currentline++)
        line(left,currentline,right,currentline,color);
}



void output_score()
{
    char a, b, c, d;
    short int low_byte = level;
    short int high_byte = lives;
    unsigned long long sseg_val = 0;
    for ( b = '0' - 1; high_byte >= 0; high_byte -= 10    ) ++b;
    a = '0' + high_byte + 10;
    sseg_val |= ((unsigned long long)convert_char_digit_2_sseg(a))<<4*8;
    sseg_val |= ((unsigned long long)convert_char_digit_2_sseg(b))<<5*8;
    for ( d = '0' - 1; low_byte >=  0; low_byte -= 10    ) ++d;
    c = '0' + low_byte + 10;
    sseg_val |= ((unsigned long long)convert_char_digit_2_sseg(c))<<0*8;
    sseg_val |= ((unsigned long long)convert_char_digit_2_sseg(d))<<1*8;
    sevensegments_ctrl(sseg_val,~0ULL);
}



// Check to see if a game object is going to hit the side of the screen
_Bool check_wall_collisions(entity *entity, unsigned char dir)
{
    int temp_x=0; // stores the location of the entity after moving

    // Get the location of the entity after it moves
    switch (dir)
    {
        case LEFT:
        {
            temp_x = entity->screen_location.x - entity->x_speed;
            break;
        }
        case RIGHT:
        {
            // Notice that we have to add the entities width to get its right(->) coordinate
            temp_x = entity->screen_location.x + entity->screen_location.w + entity->x_speed;
            break;
        }
    }
    if ( (temp_x <= 0) || (temp_x >= WINDOW_WIDTH) )
        return 1;
    else
        return 0;
}

// Check to see if the ball is going to hit a paddle
_Bool check_ball_collisions()
{
    int ball_x      = ball.screen_location.x;
    int ball_y      = ball.screen_location.y;
    int ball_width  = ball.screen_location.w;
    int ball_height = ball.screen_location.h;
    int ball_speed  = ball.y_speed;
    int paddle_x      = player.screen_location.x;
    int paddle_y      = player.screen_location.y;
    int paddle_width  = player.screen_location.w;
    int paddle_height = player.screen_location.h;
    // Check to see if ball is in Y range of the player's paddle.
    // We check its speed to see if it's even moving towards the player's paddle.
    if ( (ball_speed > 0) && (ball_y + ball_height >= paddle_y) &&
            (ball_y + ball_height <= paddle_y + paddle_height) )        // side hit
    {
        // If ball is in the X range of the paddle, return true.
        if ( (ball_x <= paddle_x + paddle_width) && (ball_x + ball_width >= paddle_x) )
            return 1;
    }
    return 0;
}


void move_ball()
{
    ball.screen_location.x += ball.x_speed;
    ball.screen_location.y += ball.y_speed;
    // If the ball is moving left, we see if it hits the wall. If does,
    // we change its direction. We do the same thing if it's moving right.
    if ( ( (ball.x_speed < 0) && check_wall_collisions(&ball, LEFT) ) ||
            ( (ball.x_speed > 0) && check_wall_collisions(&ball, RIGHT) ) )
        ball.x_speed = -ball.x_speed;
}


static unsigned char level_hits[NUM_LEVELS][MAX_BRICKS] = {
    {
        0, 3, 3, 3, 3, 3, 3, 3, 0,
        2, 0, 3, 3, 3, 3, 3, 0, 2,
        2, 2, 0, 3, 3, 3, 0, 2, 2,
        2, 2, 2, 0, 3, 0, 2, 2, 2,
        2, 2, 2, 2, 0, 2, 2, 2, 2,
        2, 2, 2, 0, 4, 0, 2, 2, 2
    }, {
        3, 3, 3, 3, 3, 3, 3, 3, 3,
        0, 4, 0, 4, 0, 4, 0, 4, 0,
        2, 2, 2, 2, 2, 2, 2, 2, 2,
        0, 4, 0, 4, 0, 4, 0, 4, 0,
        3, 3, 3, 3, 3, 3, 3, 3, 3,
        0, 4, 0, 4, 0, 4, 0, 4, 0
    }, {
        1, 2, 1, 2, 1, 2, 1, 2, 1,
        1, 2, 1, 2, 1, 2, 1, 2, 1,
        1, 2, 1, 2, 1, 2, 1, 2, 1,
        1, 2, 1, 1, 1, 2, 1, 2, 1,
        1, 2, 1, 2, 1, 2, 1, 2, 1,
        1, 2, 1, 2, 1, 2, 1, 2, 1
    }
};


// This function determines which level to load and then iterates through the BRICK structure
// reading in values from the level file and setting up the BRICKs accordingly.
void bricks_initialization()
{
    int index = 0; // used to index bricks in bricks array
    // We'll first read in the number of hits a BRICK has and then determine whether we
    // should create the BRICK (num_hits = 1-4) or if we should skip that BRICK (0)
    int temp_hits;
    int row, col;
    num_bricks=MAX_BRICKS;
    // Iterate through each row and column of BRICKs
    for (row=0; row<NUM_ROWS; row++)
    {
        for (col=0; col<NUM_COLS; col++)
        {
            // Read the next value into temp_hits //
            temp_hits = level_hits[level][row*NUM_COLS+col];

            bricks[index].num_hits = temp_hits;

            // We set the location of the BRICK according to what row and column
            // we're on in our loop. Notice that we use BRICK_SCREEN_BUFFER_[X,Y] to set
            // the BRICKs away from the sides of the screen.
            bricks[index].screen_location.x = col*BRICK_WIDTH + BRICK_SCREEN_BUFFER_X;
            bricks[index].screen_location.y = row*BRICK_HEIGHT + BRICK_SCREEN_BUFFER_Y;
            bricks[index].screen_location.w = BRICK_WIDTH;
            bricks[index].screen_location.h = BRICK_HEIGHT;
            bricks[index].color = BLACK_COLOR;

            // Now we set the colors
            switch (bricks[index].num_hits)
            {
                case 1:
                {
                    bricks[index].color = YELLOW_COLOR;
                    break;
                }
                case 2:
                {
                    bricks[index].color = RED_COLOR;
                    break;
                }
                case 3:
                {
                    bricks[index].color = GREEN_COLOR;
                    break;
                }
                case 4:
                {
                    bricks[index].color = BLUE_COLOR;
                    break;
                }
            }
            // For future use, keep track of how many bricks we have.
            index++; // move to next brick

        }
    }
}


// This function initializes the game.
void game_initialization()
{
    // Initialize the screen locations of paddle and the ball
    player.screen_location.x = (WINDOW_WIDTH / 2) - (PADDLE_WIDTH / 2);   // center screen
    player.screen_location.y = PLAYER_Y;
    player.screen_location.w = PADDLE_WIDTH;
    player.screen_location.h = PADDLE_HEIGHT;
    ball.screen_location.x = (WINDOW_WIDTH / 2) - (BALL_DIAMETER / 2);   // center screen
    ball.screen_location.y = (WINDOW_HEIGHT / 2) - (BALL_DIAMETER / 2);  // center screen
    ball.screen_location.w = BALL_DIAMETER;
    ball.screen_location.h = BALL_DIAMETER;
    // Initialize speeds
    player.x_speed   = PLAYER_SPEED;
    ball.x_speed     = 0;
    ball.y_speed     = 0;
    // init lives and levels
    lives = NUM_LIVES;
    level=0;
    // We'll need to initialize our bricks for each level, so we have a separate function handle it
    bricks_initialization();
    // clear the screen
    rect_fill(0, 0, WINDOW_WIDTH-1, WINDOW_HEIGHT-1, BLACK_COLOR);
    // timer initialization
    timer = get_ticks(1);
}

// This function receives player input and
// handles it for the main game state.
_Bool get_game_input()
{
    _Bool left_pressed  = 0;
    _Bool right_pressed = 0;
    unsigned char button_pressed = btn_ctrl();
    if((button_pressed & BUTTON_UP) != 0)
        return 1;
    if((button_pressed & BUTTON_DOWN) != 0)
        ball.y_speed = BALL_SPEED_Y;
    if((button_pressed & BUTTON_LEFT) != 0)
        left_pressed = 1;
    if((button_pressed & BUTTON_RIGHT) != 0)
        right_pressed = 1;
    // This is where we actually move the paddle
    if (left_pressed)
    {
        if ( !check_wall_collisions(&player, LEFT) )
            player.screen_location.x -= PLAYER_SPEED;
    }
    if (right_pressed)
    {
        if ( !check_wall_collisions(&player, RIGHT) )
            player.screen_location.x += PLAYER_SPEED;
    }
    return 0;
}

// Check to see if a point is within a rectangle
_Bool check_point_in_rect(int x, int y, obj_description *rect)
{
    if ( (x >= rect->x) && (x <= rect->x + rect->w) &&
            (y >= rect->y) && (y <= rect->y + rect->h) )
        return 1;
    else
        return 0;
}

_Bool change_level()
{
    level++;

    // Check to see if the player has won
    if (level >= NUM_LEVELS)
        return 1;

    // Reset the ball
    ball.x_speed = 0;
    ball.y_speed = 0;

    ball.screen_location.x = WINDOW_WIDTH/2 - ball.screen_location.w/2;
    ball.screen_location.y = WINDOW_HEIGHT/2 - ball.screen_location.h/2;

    num_bricks = 0; // Set this to zero before calling bricks_initialization()
    bricks_initialization();    // it will load the proper level
    return 0;
}


// This function changes the brick's hit count. We also need it to change
// the color of the brick and check to see if the hit count reached zero.
_Bool manage_brick_collision(int index)
{
    bricks[index].num_hits--;

    // If num_hits is 0, the brick needs to be erased
    if (bricks[index].num_hits == 0)
    {
        // Since order isn't important in our brick array, we can quickly erase a brick
        // simply by copying the last brick into the deleted brick's space. Note that we
        // have to decrement the brick count so we don't keep trying to access the
        // last brick (the one we just moved).
        bricks[index].color = BLACK_COLOR;
        num_bricks--;

        // Check to see if it's time to change the level
        if (num_bricks == 0)
        {
            if (change_level())
                return 1;
        }
    }
    // If the hit count hasn't reached zero, we need to change the brick's color
    else
    {
        switch (bricks[index].num_hits)
        {
            case 0:
            {
                bricks[index].color = BLACK_COLOR;
                break;
            }
            case 1:
            {
                bricks[index].color = YELLOW_COLOR;
                break;
            }
            case 2:
            {
                bricks[index].color = RED_COLOR;
                break;
            }
            case 3:
            {
                bricks[index].color = GREEN_COLOR;
                break;
            }
        }
    }
    return 0;
}


// This function checks to see if the ball has hit one of the bricks. It also checks
// what part of the ball hit the brick so we can adjust the ball's speed acoordingly.
_Bool check_brick_collisions()
{
    // collision points
    int left_x   = ball.screen_location.x;
    int left_y   = ball.screen_location.y + ball.screen_location.h/2;
    int right_x  = ball.screen_location.x + ball.screen_location.w;
    int right_y  = ball.screen_location.y + ball.screen_location.h/2;
    int top_x    = ball.screen_location.x + ball.screen_location.w/2;
    int top_y    = ball.screen_location.y;
    int bottom_x = ball.screen_location.x + ball.screen_location.w/2;
    int bottom_y = ball.screen_location.y + ball.screen_location.h;

    _Bool top = 0;
    _Bool bottom = 0;
    _Bool left = 0;
    _Bool right = 0;
    int brick_index;
    for (brick_index=0; brick_index<MAX_BRICKS; brick_index++)
    {
        if(bricks[brick_index].color == BLACK_COLOR) continue;
        // top
        if ( check_point_in_rect(top_x, top_y, &bricks[brick_index].screen_location) )
        {
            top = 1;
            if(manage_brick_collision(brick_index)) return 1;
        }
        // bottom
        if ( check_point_in_rect(bottom_x, bottom_y, &bricks[brick_index].screen_location) )
        {
            bottom = 1;
            if(manage_brick_collision(brick_index)) return 1;
        }
        // left
        if ( check_point_in_rect(left_x, left_y, &bricks[brick_index].screen_location) )
        {
            left = 1;
            if(manage_brick_collision(brick_index)) return 1;
        }
        // right
        if ( check_point_in_rect(right_x, right_y, &bricks[brick_index].screen_location) )
        {
            right = 1;
            if(manage_brick_collision(brick_index)) return 1;
        }
    }

    if (top)
    {
        ball.y_speed = -ball.y_speed;
        ball.screen_location.y += BALL_DIAMETER;
    }
    if (bottom)
    {
        ball.y_speed = -ball.y_speed;
        ball.screen_location.y -= BALL_DIAMETER;
    }
    if (left)
    {
        ball.x_speed = -ball.x_speed;
        ball.screen_location.x += BALL_DIAMETER;
    }
    if (right)
    {
        ball.x_speed = -ball.x_speed;
        ball.screen_location.x -= BALL_DIAMETER;
    }
    return 0;
}

void manage_loss()
{
    ball.x_speed = 0;
    ball.y_speed = 0;

    ball.screen_location.x = WINDOW_WIDTH/2 - ball.screen_location.w/2;
    ball.screen_location.y = WINDOW_HEIGHT/2 - ball.screen_location.h/2;

    lives = NUM_LIVES;
    level = 0;
    num_bricks = 0;
    bricks_initialization();
}


_Bool manage_ball()
{
    // Start by moving the ball
    move_ball();

    if ( check_ball_collisions() )
    {
        // Get center location of paddle
        int paddle_center = player.screen_location.x + player.screen_location.w / 2;
        int ball_center = ball.screen_location.x + ball.screen_location.w / 2;
        // Find the location on the paddle that the ball hit
        int paddle_location = ball_center - paddle_center;
        // Increase X speed according to distance from center of paddle.
        ball.x_speed = paddle_location / BALL_SPEED_MODIFIER;
        ball.y_speed = -ball.y_speed;
    }
    // If the ball is moving up, we should check to see if it hits the 'roof'
    if ( (ball.y_speed < 0) && (ball.screen_location.y <= 0) )
    {
        ball.y_speed = -ball.y_speed;
    }

    // Check to see if ball has passed the player
    if ( ball.screen_location.y  >= WINDOW_HEIGHT )
    {
        lives--;
        ball.x_speed = 0;
        ball.y_speed = 0;
        ball.screen_location.x = WINDOW_WIDTH/2 - ball.screen_location.w/2;
        ball.screen_location.y = WINDOW_HEIGHT/2 - ball.screen_location.h/2;
        if (lives == 0)
        {
            manage_loss();
            return 1;
        }
    }
    // Check for collisions with bricks
    return check_brick_collisions();
}


// This function handles the main game. We'll control the
// drawing of the game as well as any necessary game logic.
_Bool game_main_loop()
{
    int i;
    // Here we compare the difference between the current time and the last time we
    // handled a frame. If FRAME_RATE amount of time has, it's time for a new frame.
    if(get_ticks(0) - timer > FRAME_RATE)
    {
        entity player_saved=player;
        entity ball_saved=ball;
        if(get_game_input())
            return 0;
        if(manage_ball())
            return 0;
        // Draw the paddle and the ball
        rect(player_saved.screen_location.x,player_saved.screen_location.y, player_saved.screen_location.x+player_saved.screen_location.w-1, player_saved.screen_location.y+player_saved.screen_location.h-1, BLACK_COLOR);
        rect(player.screen_location.x,player.screen_location.y, player.screen_location.x+player.screen_location.w-1, player.screen_location.y+player.screen_location.h-1, WHITE_COLOR);
        // Iterate through the bricks array, drawing each brick
        for (i=0; i<MAX_BRICKS; i++)
        {
            rect_fill(bricks[i].screen_location.x+1, bricks[i].screen_location.y+1, bricks[i].screen_location.x+bricks[i].screen_location.w-2, bricks[i].screen_location.y+bricks[i].screen_location.h-2, bricks[i].color);
        }
        circle(ball_saved.screen_location.x+ball_saved.screen_location.w/2,ball_saved.screen_location.y+ball_saved.screen_location.h/2, ball_saved.screen_location.w/2, BLACK_COLOR);
        circle(ball.screen_location.x+ball.screen_location.w/2,ball.screen_location.y+ball.screen_location.h/2, ball.screen_location.w/2, YELLOW_COLOR);
        // Draw the boundary
        rect(0, 0, WINDOW_WIDTH-1, WINDOW_HEIGHT-1, BLUE_COLOR);
        // Output the computer and player scores
        output_score();
        // We've processed a frame so we now need to record the time at which we did it.
        // This way we can compare this time the next time our function gets called and
        // see if enough time has passed between iterations.
        timer = get_ticks(0);
    }
    return 1;
}

void main()
{
    game_initialization();
    while (game_main_loop()) {}
}


