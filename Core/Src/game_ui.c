#include "game_ui.h"
#include "main.h" // For SCREEN_WIDTH, SCREEN_HEIGHT if defined there
#include <stdio.h>

// --- Private Function Prototypes ---
static void draw_ui_bar(uint8_t lives, uint32_t score);
static void draw_game_border(void);
static void draw_bricks(const Brick bricks[BRICK_ROWS][BRICK_COLS]);
static void draw_paddle(const Paddle *paddle);
static void draw_ball(const Ball *ball);

// Calculated horizontal padding to center the grid
#define GRID_PADDING_X ((SCREEN_WIDTH - (BRICK_COLS * BRICK_WIDTH) - ((BRICK_COLS - 1) * BRICK_GAP)) / 2)
#define GRID_START_Y (UI_BAR_HEIGHT + 20)


/**
 * @brief Initializes the game state for a new game or level.
 */
void game_init_state(GameState *state) {
    // 1. Initialize Paddle
    state->paddle.width = 70;
    state->paddle.height = 10;
    state->paddle.x = (SCREEN_WIDTH - state->paddle.width) / 2.0f;
    state->paddle.prev_x = state->paddle.x;
    state->paddle.y = SCREEN_HEIGHT - state->paddle.height - 5;
    state->paddle.color = WHITE;

    // 2. Initialize Ball
    state->ball.radius = 7;
    state->ball.x = state->paddle.x + state->paddle.width / 2.0f;
    state->ball.prev_x = state->ball.x;
    state->ball.y = state->paddle.y - state->ball.radius - 1;
    state->ball.prev_y = state->ball.y;
    state->ball.dx = 2.0f;  // Initial velocity
    state->ball.dy = -2.0f;
    state->ball.color = WHITE;

    // 3. Initialize Score and Lives
    state->score = 0;
    state->lives = 3;
    state->status = GAME_PLAYING;

    // 4. Initialize Bricks
    uint16_t brick_colors[BRICK_ROWS] = {BLUE, RED, YELLOW, GREEN, MAGENTA};
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            Brick *brick = &state->bricks[row][col];
            brick->width = BRICK_WIDTH;
            brick->height = BRICK_HEIGHT;
            brick->x = GRID_PADDING_X + col * (BRICK_WIDTH + BRICK_GAP);
            brick->y = GRID_START_Y + row * (BRICK_HEIGHT + BRICK_GAP); // Corrected
            brick->color = brick_colors[row];
            brick->state = BRICK_STATE_ACTIVE;
            brick->special = BRICK_SPECIAL_NONE;

            // Assign special bricks as per user request
            if (row == 1 && col == 3) { // Example: Brick with extra ball
                brick->special = BRICK_SPECIAL_BALL;
            } else if (row == 2 && col == 4) { // Example: Brick with power-up
                brick->special = BRICK_SPECIAL_PLUS;
            }
        }
    }
}

/**
 * @brief Draws the entire game screen for the first time.
 * This function clears the screen and draws all static and dynamic elements.
 */
void game_draw_initial_scene(const GameState *state) {
    lcd_clear(BLACK);
    draw_ui_bar(state->lives, state->score);
    draw_game_border();
    draw_bricks(state->bricks);
    draw_paddle(&state->paddle);
    draw_ball(&state->ball);
}

/**
 * @brief Updates moving objects on the screen efficiently without flickering.
 * Erases objects at their previous positions and redraws them at new positions.
 */
void game_update_screen(GameState *state) {
    // Erase old paddle
    lcd_fill(state->paddle.prev_x, state->paddle.y, state->paddle.prev_x + state->paddle.width, state->paddle.y + state->paddle.height, BLACK);

    // Erase old ball
    lcd_draw_circle(state->ball.prev_x, state->ball.prev_y, BLACK, state->ball.radius, 1);

    // Draw new paddle and ball
    draw_paddle(&state->paddle);
    draw_ball(&state->ball);

    // Update previous positions for the next frame
    state->paddle.prev_x = state->paddle.x;
    state->ball.prev_x = state->ball.x;
    state->ball.prev_y = state->ball.y;
}

/**
 * @brief Erases a single brick from the screen.
 */
void game_erase_brick(const Brick* brick) {
    lcd_fill(brick->x, brick->y, brick->x + brick->width, brick->y + brick->height, BLACK);
}

/**
 * @brief Displays the pause screen.
 */
void game_draw_pause_screen(const GameState *state) {
    // Draw a semi-transparent overlay (optional, if you have blending)
    // For now, just a solid color box
    lcd_fill(40, 100, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 100, DARKGRAY);
    lcd_draw_rectangle(40, 100, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 100, WHITE);

    // Show "PAUSED" text
    lcd_show_string_center(0, 120, "PAUSED", WHITE, DARKGRAY, 24, 1);

    // Show current score
    char score_str[20];
    sprintf(score_str, "Score: %05lu", state->score);
    lcd_show_string_center(0, 160, score_str, WHITE, DARKGRAY, 16, 0);

    // Show current lives
    char lives_str[15];
    sprintf(lives_str, "Lives: %d", state->lives);
    lcd_show_string_center(0, 180, lives_str, WHITE, DARKGRAY, 16, 0);
}

/**
 * @brief Clears the pause screen and redraws the game elements.
 */
void game_clear_pause_screen(const GameState *state) {
    // Simply redraw the entire scene
    game_draw_initial_scene(state);
}

// --- Private Drawing Functions ---

static void draw_ui_bar(uint8_t lives, uint32_t score) {
    // Draw lives as filled circles
    for (int i = 0; i < 3; i++) {
        // Draw filled circle if lives > i, otherwise hollow
        lcd_draw_circle(15 + i * 20, 10, WHITE, 5, (i < lives));
    }

    // Draw score (example format)
    char score_str[10];
    sprintf(score_str, "%05lu", score);
    lcd_show_string_center(0, 2, score_str, WHITE, BLACK, 16, 0);
}

static void draw_game_border(void) {
    lcd_draw_rectangle(0, UI_BAR_HEIGHT, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, WHITE);
}

static void draw_bricks(const Brick bricks[BRICK_ROWS][BRICK_COLS]) {
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            const Brick *brick = &bricks[row][col];
            if (brick->state == BRICK_STATE_ACTIVE) {
                // Draw the main brick body
                lcd_fill(brick->x, brick->y, brick->x + brick->width, brick->y + brick->height, brick->color);

                // Draw special features if any
                switch (brick->special) {
                    case BRICK_SPECIAL_BALL:
                        // Draw a white circle inside the brick
                        lcd_draw_circle(brick->x + brick->width / 2, brick->y + brick->height / 2, WHITE, 6, 0);
                        break;
                    case BRICK_SPECIAL_PLUS:
                        // Draw a black plus sign inside the brick
                        lcd_draw_line(brick->x + brick->width / 2, brick->y + 3, brick->x + brick->width / 2, brick->y + brick->height - 3, BLACK);
                        lcd_draw_line(brick->x + 3, brick->y + brick->height / 2, brick->x + brick->width - 3, brick->y + brick->height / 2, BLACK);
                        break;
                    case BRICK_SPECIAL_NONE:
                    default:
                        // No special feature
                        break;
                }
            }
        }
    }
}

static void draw_paddle(const Paddle *paddle) {
    lcd_fill(paddle->x, paddle->y, paddle->x + paddle->width, paddle->y + paddle->height, paddle->color);
}

static void draw_ball(const Ball *ball) {
    lcd_draw_circle(ball->x, ball->y, ball->color, ball->radius, 1); // Filled circle
}