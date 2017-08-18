/// Pong game ported to the PandA framework by Fabrizio Ferrandi.
/// The original code has been taken from http://www.aaroncox.net/tutorials/arcade/PaddleBattle.html
/// Pong was the first game developed by Atari Inc and was designed and built by Allan Alcorn. Further information can be found at https://en.wikipedia.org/wiki/Pong.
/// Random functions taken from http://burtleburtle.net/bob/rand/smallprng.html

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

// Locations of paddles in the game
#define COMPUTER_Y 30
#define PLAYER_Y   430

// Dimensions of a paddle
#define PADDLE_WIDTH  32
#define PADDLE_HEIGHT 8

// Diameter of the ball
#define BALL_DIAMETER 8

// Paddle speeds
#define PLAYER_SPEED    2
#define COMPUTER_SPEED  2

// Ball speeds
#define BALL_SPEED_MODIFIER 4  // divide location on paddle by this
#define BALL_SPEED_Y        2 // max speed of ball along y axis

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

// Global data
entity             computer;       // The computer's paddle
entity             player;         // The player's paddle
entity             ball;           // The game ball
unsigned char      computer_score;  // AI's score
unsigned char      player_score;    // player's score
static int last_speed = 0;
static int decision = 3;
int    timer;              // Game timer

// Random functions
//taken from http://burtleburtle.net/bob/rand/smallprng.html
typedef struct _random_state_t
{
    unsigned int a;
    unsigned int b;
    unsigned int c;
    unsigned int d;
} random_state_t;

#define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))
unsigned int random_number( random_state_t *x )
{
    unsigned int e = x->a - rot(x->b, 27);
    x->a = x->b ^ rot(x->c, 17);
    x->b = x->c + x->d;
    x->c = x->d + e;
    x->d = e + x->a;
    return x->d;
}

void random_number_init( random_state_t *x, unsigned int seed )
{
    unsigned int i;
    x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
    for (i=0; i<20; ++i)
        (void)random_number(x);
}

static random_state_t random_state;

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
    short int low_byte = player_score;
    short int high_byte = computer_score;
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
_Bool check_ball_collisions(entity* paddle)
{
    int ball_x      = ball.screen_location.x;
    int ball_y      = ball.screen_location.y;
    int ball_width  = ball.screen_location.w;
    int ball_height = ball.screen_location.h;
    int ball_speed  = ball.y_speed;
    int paddle_x      = paddle->screen_location.x;
    int paddle_y      = paddle->screen_location.y;
    int paddle_width  = paddle->screen_location.w;
    int paddle_height = paddle->screen_location.h;
    // Get which paddle we're checking against
    if ( paddle->screen_location.y == PLAYER_Y)
    {
        // Check to see if ball is in Y range of the player's paddle.
        // We check its speed to see if it's even moving towards the player's paddle.
        if ( (ball_speed > 0) && (ball_y + ball_height >= paddle_y) &&
                (ball_y + ball_height <= paddle_y + paddle_height) )        // side hit
        {
            // If ball is in the X range of the paddle, return true.
            if ( (ball_x <= paddle_x + paddle_width) && (ball_x + ball_width >= paddle_x) )
                return 1;
        }
    }
    else
    {
        // Check to see if ball is in Y range of the computer's paddle.
        // We check its speed to see if it's even moving towards the computer's paddle.
        if ( (ball_speed < 0) && (ball_y >= paddle_y) &&
                (ball_y <= paddle_y + paddle_height) )
        {
            // If ball is in the X range of the paddle, return true.
            if ( (ball_x <= paddle_x + paddle_width) && (ball_x + ball_width >= paddle_x) )
                return 1;
        }
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

// Increase the player's score, reset ball,  and see if player has won.
_Bool manage_player_score()
{
    // Increase score
    player_score++;
    // Reset ball
    ball.x_speed = 0;
    ball.y_speed = 0;
    ball.screen_location.x = (WINDOW_WIDTH / 2) - (BALL_DIAMETER / 2);
    ball.screen_location.y = (WINDOW_HEIGHT / 2) - (BALL_DIAMETER / 2);
    // Check to see if player has won
    if (player_score == 10)
    {
        computer_score = 0;
        player_score   = 0;
        return 1;
    }
    return 0;
}

// Increase computer's score, reset ball, and see it if computer has won.
_Bool manage_computer_score()
{
    // Increase score
    computer_score++;
    // Reset ball
    ball.x_speed = 0;
    ball.y_speed = 0;
    ball.screen_location.x = (WINDOW_WIDTH / 2) - (BALL_DIAMETER / 2);
    ball.screen_location.y = (WINDOW_HEIGHT / 2) - (BALL_DIAMETER / 2);
    // See if computer has won
    if (computer_score == 10)
    {
        computer_score = 0;
        player_score   = 0;
        return 1;
    }
    return 0;
}


// This function initializes the game.
void game_initialization()
{
    // Initialize the screen locations of two paddles and the ball
    computer.screen_location.x = (WINDOW_WIDTH / 2) - (PADDLE_WIDTH / 2);  // center screen
    computer.screen_location.y = COMPUTER_Y;
    computer.screen_location.w = PADDLE_WIDTH;
    computer.screen_location.h = PADDLE_HEIGHT;
    player.screen_location.x = (WINDOW_WIDTH / 2) - (PADDLE_WIDTH / 2);   // center screen
    player.screen_location.y = PLAYER_Y;
    player.screen_location.w = PADDLE_WIDTH;
    player.screen_location.h = PADDLE_HEIGHT;
    ball.screen_location.x = (WINDOW_WIDTH / 2) - (BALL_DIAMETER / 2);   // center screen
    ball.screen_location.y = (WINDOW_HEIGHT / 2) - (BALL_DIAMETER / 2);  // center screen
    ball.screen_location.w = BALL_DIAMETER;
    ball.screen_location.h = BALL_DIAMETER;
    // Initialize speeds
    computer.x_speed = COMPUTER_SPEED;
    player.x_speed   = PLAYER_SPEED;
    ball.x_speed     = 0;
    ball.y_speed     = 0;
    last_speed = 0;
    decision = 3;
    // Set scores to zero
    computer_score = 0;
    player_score   = 0;
    // Random number generator initialization
    random_number_init(&random_state, 0);
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

_Bool manage_ball()
{
    // Start by moving the ball
    move_ball();
    if ( check_ball_collisions(&player) )
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

    if ( check_ball_collisions(&computer) )
    {
        // Get center location of paddle
        int paddle_center = computer.screen_location.x + computer.screen_location.w / 2;
        int ball_center = ball.screen_location.x + ball.screen_location.w / 2;
        // Find the location on the paddle that the ball hit
        int paddle_location = ball_center - paddle_center;
        // Increase X speed according to distance from center of paddle.
        ball.x_speed = paddle_location / BALL_SPEED_MODIFIER;
        ball.y_speed = -ball.y_speed;
    }

    // Check to see if someone has scored
    if (ball.screen_location.y < 0)
        return manage_player_score();
    if (ball.screen_location.y + ball.screen_location.h > WINDOW_HEIGHT)
        return manage_computer_score();
    return 0;
}

// Move the computer's paddle and change its direction if necessary
void manage_AI()
{
    int computer_x=2;
    int ball_center = ball.screen_location.x + ball.screen_location.w / 2;
    // See if ball has changed direction
    if (last_speed != ball.x_speed)
    {
        // 0 == left side, 1 == right side, 2,3 = center
        decision = random_number(&random_state) % 4;
        last_speed = ball.x_speed;
    }
    // Determine part of paddle to hit ball with according to decision
    switch (decision)
    {
      case 0:
      {
        computer_x = computer.screen_location.x;
        break;
      }
      case 1:
      {
        computer_x = computer.screen_location.x + computer.screen_location.w;
        break;
      }
      case 2:
      case 3:
      {
        computer_x = computer.screen_location.x + computer.screen_location.w / 2;
        break;
      }
    }
    // See if ball is near computer's center. Prevents
    // computer from rapidly moving back and forth.   
    if ( abs(computer_x - ball_center) < BALL_SPEED_Y )
        return;
    // Left
    if (computer_x > ball_center)
    {
        if ( !check_wall_collisions(&computer, LEFT) )
            computer.screen_location.x -= COMPUTER_SPEED;
    }
    // Right
    else if (computer_x < ball_center)
    {
        if ( !check_wall_collisions(&computer, RIGHT) )
            computer.screen_location.x += COMPUTER_SPEED;
    }
}


// This function handles the main game. We'll control the  
// drawing of the game as well as any necessary game logic.
_Bool game_main_loop()
{
    // Here we compare the difference between the current time and the last time we
    // handled a frame. If FRAME_RATE amount of time has, it's time for a new frame.
    if(get_ticks(0) - timer > FRAME_RATE)
    {
        entity computer_saved=computer;
        entity player_saved=player;
        entity ball_saved=ball;
        if(get_game_input())
            return 0;
        if(manage_ball())
            return 0;
        manage_AI();
        // Draw the two paddles and the ball
        rect(computer_saved.screen_location.x,computer_saved.screen_location.y, computer_saved.screen_location.x+computer_saved.screen_location.w, computer_saved.screen_location.y+computer_saved.screen_location.h, BLACK_COLOR);
        rect(computer.screen_location.x,computer.screen_location.y, computer.screen_location.x+computer.screen_location.w, computer.screen_location.y+computer.screen_location.h, GREEN_COLOR);
        rect(player_saved.screen_location.x,player_saved.screen_location.y, player_saved.screen_location.x+player_saved.screen_location.w, player_saved.screen_location.y+player_saved.screen_location.h, BLACK_COLOR);
        rect(player.screen_location.x,player.screen_location.y, player.screen_location.x+player.screen_location.w, player.screen_location.y+player.screen_location.h, WHITE_COLOR);
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


