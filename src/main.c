#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <string.h>

int SCREEN_WIDTH = -1;
int SCREEN_HEIGHT = -1;
int SCREEN_BPP = -1;

int PADDLE_WIDTH = -1;
int PADDLE_HEIGHT = -1;
int PADDLE_OFFSET = -1;

int fbfd = -1;

void *fbmem = NULL;

static int running = 1;

typedef int ErrNo;

void fb_clear()
{
    memset(fbmem, 0, SCREEN_WIDTH * SCREEN_HEIGHT * SCREEN_BPP / 8);
}

void fb_draw_rect(int x, int y, int w, int h, int color)
{

    // clip the rectangle to the screen
    if (x < 0)
    {
        w += x;
        x = 0;
    }
    if (y < 0)
    {
        h += y;
        y = 0;
    }
    if (x + w > SCREEN_WIDTH)
    {
        w = SCREEN_WIDTH - x;
    }
    if (y + h > SCREEN_HEIGHT)
    {
        h = SCREEN_HEIGHT - y;
    }

    for (int i = y; i < y + h; i++)
    {
        for (int j = x; j < x + w; j++)
        {
            *((int *)fbmem + i * SCREEN_WIDTH + j) = color;
        }
    }
}

ErrNo init_fb()
{
    // open the framebuffer
    fbfd = -1;
    printf("Opening framebuffer...\n");
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1)
    {
        printf("Error: cannot open framebuffer device.\n");
        return -1;
    }

    // get the screen size
    struct fb_var_screeninfo info = {};

    printf("Getting screen info...\n");
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &info) == -1)
    {
        printf("Error: cannot get screen info.\n");
        return -1;
    }

    SCREEN_WIDTH = info.xres;
    SCREEN_HEIGHT = info.yres;
    SCREEN_BPP = info.bits_per_pixel;

    PADDLE_WIDTH = SCREEN_WIDTH / 32;
    PADDLE_HEIGHT = SCREEN_HEIGHT / 8;
    PADDLE_OFFSET = SCREEN_WIDTH / 32;

    fbmem = mmap(NULL, info.xres * info.yres * info.bits_per_pixel / 8,
                 PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

    if (fbmem == MAP_FAILED)
    {
        printf("Error: cannot mmap framebuffer.\n");
        return -1;
    }

    // disable cursor
    printf("Disabling cursor...\n");
    system("setterm -cursor off");

    // disable echo
    printf("Disabling echo...\n");
    system("stty -echo");

    fb_clear();

    return 0;
}

void cleanup_fb()
{
    // enable cursor
    printf("Enabling cursor...\n");
    system("setterm -cursor on");

    // enable echo
    printf("Enabling echo...\n");
    system("stty echo");

    return;
}

typedef struct
{
    int ball_x; // Ball position
    int ball_y;

    int ball_vx; // Ball velocity
    int ball_vy;

    int left_paddle_y;  // Left paddle y position
    int right_paddle_y; // Right paddle  y position

    int left_score;  // Left player score
    int right_score; // Right player score

    int left_ai;  // Is the left paddle controlled by the computer?
    int right_ai; // Is the right paddle controlled by the computer?
} State;

State state = {};

void draw_state_fb(State *state)
{
    fb_clear();
    fb_draw_rect(PADDLE_OFFSET, state->left_paddle_y, PADDLE_WIDTH, PADDLE_HEIGHT, 0xFFFFFFFF);
    fb_draw_rect(SCREEN_WIDTH - PADDLE_OFFSET - PADDLE_WIDTH, state->right_paddle_y, PADDLE_WIDTH, PADDLE_HEIGHT, 0xFFFFFFFF);
    fb_draw_rect(state->ball_x, state->ball_y, 10, 10, 0xFFFFFFFF);
}

// void draw_state(State *state)
// {

// draw the left paddle
// SDL_Rect left_paddle = {PADDLE_OFFSET,
//                         state->left_paddle_y,
//                         PADDLE_WIDTH, PADDLE_HEIGHT};
// SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
// SDL_RenderFillRect(renderer, &left_paddle);

// // draw the right paddle
// SDL_Rect right_paddle = {SCREEN_WIDTH - PADDLE_OFFSET - PADDLE_WIDTH,
//                          state->right_paddle_y, PADDLE_WIDTH, PADDLE_HEIGHT};
// SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
// SDL_RenderFillRect(renderer, &right_paddle);

// // draw the ball
// SDL_Rect ball = {state->ball_x, state->ball_y, 10, 10};
// SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
// SDL_RenderFillRect(renderer, &ball);

// // draw the center line (dashed)
// SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
// for (int i = 0; i < SCREEN_HEIGHT; i += 10)
// {
//     SDL_RenderDrawPoint(renderer, SCREEN_WIDTH / 2, i);
// }

// // draw the score
// char score_text[32]; // buffer to hold the score text
// sprintf(score_text, "%d - %d", state->left_score, state->right_score);

// SDL_Color color = {0xFF, 0xFF, 0xFF, 0xFF}; // white
// SDL_Surface *score_surface = TTF_RenderText_Solid(font, score_text, color);
// SDL_Texture *score_texture = SDL_CreateTextureFromSurface(renderer, score_surface);

// SDL_Rect score_rect = {SCREEN_WIDTH / 2 - score_surface->w / 2, 0,
//                        score_surface->w, score_surface->h};
// SDL_RenderCopy(renderer, score_texture, NULL, &score_rect);
// }

#define BALL_SPEED 10
#define PADDLE_SPEED 15
void update_ai_player(State *state)
{
    // if the left ai is enabled, and the ball is on the left side of the screen
    if (state->left_ai == 1 && state->ball_x < SCREEN_WIDTH / 2)
    {

        if (state->ball_y < state->left_paddle_y + PADDLE_HEIGHT / 2)
            state->left_paddle_y -= PADDLE_SPEED;

        if (state->ball_y > state->left_paddle_y + PADDLE_HEIGHT / 2)
            state->left_paddle_y += PADDLE_SPEED;
    }

    if (state->right_ai == 1 && state->ball_x > SCREEN_WIDTH / 2)
    {

        if (state->ball_y < state->right_paddle_y + PADDLE_HEIGHT / 2)
            state->right_paddle_y -= PADDLE_SPEED;

        if (state->ball_y > state->right_paddle_y + PADDLE_HEIGHT / 2)
            state->right_paddle_y += PADDLE_SPEED;
    }
}

void update_state(State *state)
{
    // update the ball position
    state->ball_x += state->ball_vx;
    state->ball_y += state->ball_vy;

    // bounce the ball off the top and bottom of the screen
    if (state->ball_y < 0 || state->ball_y > SCREEN_HEIGHT)
    {
        state->ball_vy = -state->ball_vy;
    }

    // bounce the ball off the paddles
    if (state->ball_x < PADDLE_OFFSET + PADDLE_WIDTH &&
        state->ball_y > state->left_paddle_y &&
        state->ball_y < state->left_paddle_y + PADDLE_HEIGHT)
    {
        state->ball_vx = -state->ball_vx;
    }

    if (state->ball_x > SCREEN_WIDTH - PADDLE_OFFSET - PADDLE_WIDTH &&
        state->ball_y > state->right_paddle_y &&
        state->ball_y < state->right_paddle_y + PADDLE_HEIGHT)
    {
        state->ball_vx = -state->ball_vx;
    }

    // update the paddle positions
    // const Uint8 *kb_state = SDL_GetKeyboardState(NULL);
    // if (state->left_ai == 0)
    // {

    //     if (kb_state[SDL_SCANCODE_W])
    //         state->left_paddle_y -= PADDLE_SPEED;

    //     if (kb_state[SDL_SCANCODE_S])
    //         state->left_paddle_y += PADDLE_SPEED;
    // }

    // if (state->right_ai == 0)
    // {

    //     if (kb_state[SDL_SCANCODE_UP])
    //         state->right_paddle_y -= PADDLE_SPEED;

    //     if (kb_state[SDL_SCANCODE_DOWN])
    //         state->right_paddle_y += PADDLE_SPEED;
    // }

    update_ai_player(state);

    // update the score
    if (state->ball_x < 0)
    {
        state->right_score++;
        state->ball_x = SCREEN_WIDTH / 2;
        state->ball_y = SCREEN_HEIGHT / 2;
        state->ball_vx = BALL_SPEED;
        state->ball_vy = BALL_SPEED;
    }

    if (state->ball_x > SCREEN_WIDTH)
    {
        state->left_score++;
        state->ball_x = SCREEN_WIDTH / 2;
        state->ball_y = SCREEN_HEIGHT / 2;
        state->ball_vx = -BALL_SPEED;
        state->ball_vy = -BALL_SPEED;
    }
    // keep the ball within the screen
    if (state->ball_x < 0)
        state->ball_x = 0;

    if (state->ball_x > SCREEN_WIDTH)
        state->ball_x = SCREEN_WIDTH;

    if (state->ball_y < 0)
        state->ball_y = 0;

    if (state->ball_y > SCREEN_HEIGHT)
        state->ball_y = SCREEN_HEIGHT;

    // clamp the paddle positions
    if (state->left_paddle_y < 0)
        state->left_paddle_y = 0;

    if (state->left_paddle_y > SCREEN_HEIGHT - PADDLE_HEIGHT)
        state->left_paddle_y = SCREEN_HEIGHT - PADDLE_HEIGHT;

    if (state->right_paddle_y < 0)
        state->right_paddle_y = 0;

    if (state->right_paddle_y > SCREEN_HEIGHT - PADDLE_HEIGHT)
        state->right_paddle_y = SCREEN_HEIGHT - PADDLE_HEIGHT;

    // reset the ball if it goes off the screen
    if (state->ball_x < 0 || state->ball_x > SCREEN_WIDTH)
    {
        state->ball_x = SCREEN_WIDTH / 2;
        state->ball_y = SCREEN_HEIGHT / 2;
        state->ball_vx = BALL_SPEED;
        state->ball_vy = BALL_SPEED;
    }
}

ErrNo main(int argc, char *argv[])
{
    printf("Starting up...\n");
    ErrNo err = init_fb();

    if (err != 0)
    {
        printf("Error initializing SDL: %d\n", err);
        return 1;
    }

    // setup the initial state
    state.ball_x = SCREEN_WIDTH / 2;
    state.ball_y = SCREEN_HEIGHT / 2;
    state.ball_vx = BALL_SPEED;
    state.ball_vy = BALL_SPEED;
    state.left_paddle_y = SCREEN_HEIGHT / 2;
    state.right_paddle_y = SCREEN_HEIGHT / 2;
    state.left_ai = 1;
    state.right_ai = 1;

    // SDL_Event e;
    while (running)
    {
        // while (SDL_PollEvent(&e) != 0)
        // {
        //     if (e.type == SDL_QUIT)
        //         running = 0;
        // }

        // Update the state
        update_state(&state);

        // Current time for calculating FPS
        // int ticks = SDL_GetTicks();
        // int target = ticks + 1000 / 30;

        time_t ts;
        ctime(&ts);
        int ticks = ts / 1000000;
        int target = ticks + 1000 / 30;

        if (ticks < target)
        {
            usleep((target - ticks) * 1000);
        }

        fb_clear();
        draw_state_fb(&state);
    }

    cleanup_fb();
    return 0;
}