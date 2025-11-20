/*
 * game_ui.h
 */

#ifndef INC_GAME_UI_H_
#define INC_GAME_UI_H_

#include <stdint.h>
#include "lcd.h"

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define UI_BAR_HEIGHT 30
#define BRICK_GAP 2

#define BRICK_ROWS 5
#define BRICK_COLS 7
#define BRICK_WIDTH 32
#define BRICK_HEIGHT 16

#define PADDLE_WIDTH 60
#define PADDLE_HEIGHT 10

#define BALL_SIZE 8
#define MAX_BALLS 4

#define MAX_LIVES 4

// Enum for Game Status
typedef enum {
    GAME_START_SCREEN,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_OVER
} GameStatus;

// Enum for brick state
typedef enum {
    BRICK_STATE_INCOMING,   // dropping in from above screen
    BRICK_STATE_ACTIVE,     // ready for collision
    BRICK_STATE_DESTROYED
} BrickState;

// Enum for special brick types
typedef enum {
    BRICK_SPECIAL_NONE,
    BRICK_SPECIAL_BALL,
    BRICK_SPECIAL_PLUS
} BrickSpecial;

// Structure for a single brick
typedef struct {
    uint16_t x, y;
    uint16_t width, height;
    uint16_t color;
    BrickState state;
    BrickSpecial special;
    int16_t final_y;         // target y position after drop completes
    uint8_t drop_speed;      // pixels per frame to drop
    // uint8_t hp; // Use in the future for multi-hit bricks
} Brick;

// Structure for the ball
typedef struct {
    int16_t x, y;
    int16_t prev_x, prev_y; // Previous position for efficient redraw
    int16_t dx, dy;
    uint16_t color;
    uint8_t radius;
} Ball;

// Structure for the paddle
typedef struct {
    uint16_t x, y; // Top-left corner
    uint16_t prev_x; // Previous position for efficient redraw
    uint16_t width;
    uint16_t height;
    uint16_t color;
    uint16_t speed; // Pixels per frame
} Paddle;

// Structure for the entire game state
typedef struct {
    Brick bricks[BRICK_ROWS][BRICK_COLS];
    Ball balls[MAX_BALLS];
    uint8_t ball_count;
    Paddle paddle;
    uint32_t score;
    uint8_t lives;
    uint8_t level;
    int16_t brick_drop_offset; // negative while bricks are dropping in
    uint8_t brick_dropping;
    GameStatus status;
    uint8_t show_potentiometer_prompt;
} GameState;

// --- Function Prototypes ---

// Initialization
void game_init_state(GameState *state);

// Start Screen
void game_draw_start_screen(void);

// Full Draw (called once at the beginning)
void game_draw_initial_scene(const GameState *state);
void game_update_screen(GameState *state);

// Pause Screen
void game_draw_pause_screen(const GameState *state);
void game_clear_pause_screen(const GameState *state);

// Game Over Screen
void game_draw_game_over_screen(const GameState *state);

// Partial Updates (called in the game loop)
void game_update_paddle(Paddle *paddle);
void game_update_ball(Ball *ball);
void game_erase_brick(const Brick *brick);
void game_update_ui_bar(uint32_t score, uint8_t lives, uint8_t level);
void draw_game_border(void);
void draw_potentiometer_prompt();
void game_erase_ball(const Ball *ball);

// Special brick effects
void spawn_extra_ball(GameState *state, const Ball *template_ball);
void apply_plus_powerup(GameState *state);
// Move paddle using two hardware buttons (button_count[5] = left, [6] = right)
void game_handle_paddle_buttons(GameState *state);
// Level management
void advance_level(GameState *state);
void init_bricks_for_level(GameState *state, uint8_t level, uint8_t animate);
#endif /* INC_GAME_UI_H_ */
