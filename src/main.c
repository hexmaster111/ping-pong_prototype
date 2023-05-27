// Ping-poing game
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

static int running = 1;

typedef int ErrNo;

ErrNo init_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Ping-Pong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    return 0;
}

void cleanup_sdl()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return;
}

typedef struct state
{
    int ball_x; // Ball position
    int ball_y;

    int ball_vx; // Ball velocity
    int ball_vy;

    int left_paddle_y;  // Left paddle y position
    int right_paddle_y; // Right paddle  y position

    int left_score;  // Left player score
    int right_score; // Right player score
} State;

State state = {};

// the paddles should be 1/8 of the screen height, and 1/32 of the screen width
// there offset from the edge of the screen should be 1/32 of the screen width
const int paddle_width = SCREEN_WIDTH / 32;
const int paddle_height = SCREEN_HEIGHT / 8;
const int paddle_offset = SCREEN_WIDTH / 32;

void draw_state(State *state)
{

    // draw the left paddle
    SDL_Rect left_paddle = {paddle_offset, state->left_paddle_y, paddle_width, paddle_height};
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderFillRect(renderer, &left_paddle);

    // draw the right paddle
    SDL_Rect right_paddle = {SCREEN_WIDTH - paddle_offset - paddle_width,
                             state->right_paddle_y, paddle_width, paddle_height};
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderFillRect(renderer, &right_paddle);

    // draw the ball
    SDL_Rect ball = {state->ball_x, state->ball_y, 10, 10};
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderFillRect(renderer, &ball);
}

#define BALL_SPEED 10
#define PADDLE_SPEED 15

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
    if (state->ball_x < SCREEN_WIDTH / 32 + 10 &&
        state->ball_y > state->left_paddle_y &&
        state->ball_y < state->left_paddle_y + SCREEN_HEIGHT / 8)
    {
        state->ball_vx = -state->ball_vx;
    }

    // bounce the ball off the paddles
    if (state->ball_x > SCREEN_WIDTH - SCREEN_WIDTH / 32 - 10 &&
        state->ball_y > state->right_paddle_y &&
        state->ball_y < state->right_paddle_y + SCREEN_HEIGHT / 8)
    {
        state->ball_vx = -state->ball_vx;
    }

    // update the paddle positions
    const Uint8 *state_array = SDL_GetKeyboardState(NULL);
    if (state_array[SDL_SCANCODE_W])
        state->left_paddle_y -= PADDLE_SPEED;

    if (state_array[SDL_SCANCODE_S])
        state->left_paddle_y += PADDLE_SPEED;

    if (state_array[SDL_SCANCODE_UP])
        state->right_paddle_y -= PADDLE_SPEED;

    if (state_array[SDL_SCANCODE_DOWN])
        state->right_paddle_y += PADDLE_SPEED;

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

    // clamp the paddle positions
    if (state->left_paddle_y < 0)
        state->left_paddle_y = 0;

    if (state->left_paddle_y > SCREEN_HEIGHT - SCREEN_HEIGHT / 8)
        state->left_paddle_y = SCREEN_HEIGHT - SCREEN_HEIGHT / 8;

    if (state->right_paddle_y < 0)
        state->right_paddle_y = 0;

    if (state->right_paddle_y > SCREEN_HEIGHT - SCREEN_HEIGHT / 8)
        state->right_paddle_y = SCREEN_HEIGHT - SCREEN_HEIGHT / 8;

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
    ErrNo err = init_sdl();

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

    SDL_Event e;
    while (running)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                running = 0;
            }
        }

        // Update the state
        update_state(&state);

        // Current time for calculating FPS, our target is 24 fps
        Uint32 ticks = SDL_GetTicks();
        Uint32 target = ticks + 1000 / 24;

        if (ticks < target)
        {
            SDL_Delay(target - ticks);
        }

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);
        draw_state(&state);
        SDL_RenderPresent(renderer);
    }

    cleanup_sdl();
    return 0;
}