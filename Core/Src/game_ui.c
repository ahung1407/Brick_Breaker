#include "game_ui.h"
#include "main.h" // For SCREEN_WIDTH, SCREEN_HEIGHT if defined there
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "button.h"

// --- Private Function Prototypes ---
static void draw_ui_bar(uint8_t lives, uint32_t score, uint8_t level);
static void draw_bricks(const GameState *state);
static void draw_paddle(const Paddle *paddle);
static void draw_ball(const Ball *ball);
//static void draw_potentiometer_prompt(void);

// Calculated horizontal padding to center the grid
#define GRID_PADDING_X ((SCREEN_WIDTH - (BRICK_COLS * BRICK_WIDTH) - ((BRICK_COLS - 1) * BRICK_GAP)) / 2)
#define GRID_START_Y (UI_BAR_HEIGHT + 20)


/**
 * @brief Draws the prompt to rotate the potentiometer with a dashed border.
 */
void draw_potentiometer_prompt(void) {
    uint16_t box_x1 = 20;
    uint16_t box_y1 = 150;
    uint16_t box_x2 = SCREEN_WIDTH - 20;
    uint16_t box_y2 = 200;
    uint16_t color = WHITE;
    uint16_t dash_length = 5;

    // Draw dashed border
    // Top and bottom
    for (uint16_t x = box_x1; x < box_x2; x += 2 * dash_length) {
        uint16_t end_x = x + dash_length;
        if (end_x > box_x2) end_x = box_x2;
        lcd_draw_line(x, box_y1, end_x, box_y1, color);
        lcd_draw_line(x, box_y2, end_x, box_y2, color);
    }
    // Left and right
    for (uint16_t y = box_y1; y < box_y2; y += 2 * dash_length) {
        uint16_t end_y = y + dash_length;
        if (end_y > box_y2) end_y = box_y2;
        lcd_draw_line(box_x1, y, box_x1, end_y, color);
        lcd_draw_line(box_x2, y, box_x2, end_y, color);
    }
    // Show the text lines
    lcd_show_string_center(0, 164 - 8, "ROTATE POTENTIOMETER", WHITE, 0, 16, 1);
    lcd_show_string_center(0, 164 + 8, "TO PLAY", WHITE, 0, 16, 1);

}


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
    state->paddle.speed = 6; // default paddle movement speed (pixels per update)

    // 2. Initialize Balls (multi-ball support)
    state->ball_count = 1;
    for (int i = 0; i < MAX_BALLS; i++) {
        state->balls[i].radius = 7;
        state->balls[i].color = WHITE;
    }
    // Initialize first ball
    state->balls[0].x = state->paddle.x + state->paddle.width / 2.0f;
    state->balls[0].prev_x = state->balls[0].x;
    state->balls[0].y = state->paddle.y - state->balls[0].radius - 1;
    state->balls[0].prev_y = state->balls[0].y;
    state->balls[0].dx = 0;  // Initial velocity
    state->balls[0].dy = -60;
    state->balls[0].color = WHITE;

    // 3. Initialize Score and Lives
    state->score = 0;
    state->lives = MAX_LIVES;
    state->level = 1;
    state->status = GAME_PLAYING;
    state->show_potentiometer_prompt = 0;

    // Initialize bricks for the starting level without drop animation (animate=0)
    state->level = 1;
    init_bricks_for_level(state, state->level, 0);
}

/**
 * @brief Draws the entire game screen for the first time.
 * This function clears the screen and draws all static and dynamic elements.
 */
void game_draw_initial_scene(const GameState *state) {
    lcd_clear(BLACK);
    draw_ui_bar(state->lives, state->score, state->level);
    draw_game_border();
    draw_bricks(state);
    draw_paddle(&state->paddle);
    // Draw all active balls
    for (int i = 0; i < state->ball_count; i++) {
        draw_ball(&state->balls[i]);
    }
    if (state->show_potentiometer_prompt) {
    	draw_potentiometer_prompt();
    }
}

/**
 * @brief Updates moving objects on the screen efficiently without flickering.
 * Erases objects at their previous positions and redraws them at new positions.
 */
void game_update_screen(GameState *state) {
	// update components
    // handle paddle movement from buttons before drawing/updating
    game_handle_paddle_buttons(state);
	// Update all active balls
	for (int i = 0; i < state->ball_count; i++) {
		game_update_ball(&state->balls[i]);
	}
    game_update_paddle(&state->paddle);
    game_update_ui_bar(state->score, state->lives, state->level);
    
    // Erase INCOMING bricks before updating their position (to avoid visual artifacts)
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            Brick *brick = &state->bricks[row][col];
            if (brick->state == BRICK_STATE_INCOMING) {
                // Erase old brick position
                int16_t erase_y1 = brick->y;
                int16_t erase_y2 = brick->y + brick->height;
                if (erase_y2 > UI_BAR_HEIGHT && erase_y1 < SCREEN_HEIGHT) {
                    if (erase_y1 < UI_BAR_HEIGHT) erase_y1 = UI_BAR_HEIGHT;
                    if (erase_y2 > SCREEN_HEIGHT) erase_y2 = SCREEN_HEIGHT;
                    lcd_fill(brick->x, erase_y1, brick->x + brick->width, erase_y2, BLACK);
                }
            }
        }
    }
    
    // Update brick drop animation: move bricks from INCOMING state to ACTIVE
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            Brick *brick = &state->bricks[row][col];
            if (brick->state == BRICK_STATE_INCOMING) {
                brick->y += brick->drop_speed; // Move downward
                if (brick->y >= brick->final_y) {
                    brick->y = brick->final_y; // Snap to final position
                    brick->state = BRICK_STATE_ACTIVE; // Now ready for collision
                }
            }
        }
    }
    
    draw_bricks(state);
    draw_game_border();
    // Update previous positions for the next frame
    state->paddle.prev_x = state->paddle.x;
    for (int i = 0; i < state->ball_count; i++) {
        state->balls[i].prev_x = state->balls[i].x;
        state->balls[i].prev_y = state->balls[i].y;
    }
}

/**
 * @brief Update paddle position from two buttons: button_count[5] (left) and button_count[6] (right).
 *        Holding a button moves the paddle continuously; the movement is clamped to screen bounds.
 */
void game_handle_paddle_buttons(GameState *state) {
    // button_count is updated by button_scan(); >0 means button is pressed
    extern uint16_t button_count[16];

    if (button_count[8] > 0) {
        // move left
        if (state->paddle.x > state->paddle.speed)
            state->paddle.x -= state->paddle.speed;
        else
            state->paddle.x = 0;
    }

    if (button_count[9] > 0) {
        // move right
        uint16_t max_x = SCREEN_WIDTH - state->paddle.width;
        if (state->paddle.x + state->paddle.speed < max_x)
            state->paddle.x += state->paddle.speed;
        else
            state->paddle.x = max_x;
    }
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
    lcd_show_string_center(-10, 120, "PAUSED", WHITE, DARKGRAY, 24, 1);

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

/**
 * @brief Spawns an extra ball when BRICK_SPECIAL_BALL is destroyed.
 */
void spawn_extra_ball(GameState *state, const Ball *template_ball) {
    if (state->ball_count >= MAX_BALLS) return;
    // Copy template ball to new slot
    Ball *new_ball = &state->balls[state->ball_count++];
    *new_ball = *template_ball;
    // Offset new ball slightly to avoid immediate overlap
    new_ball->x += 8;
    new_ball->y -= 5;
    // Give slightly different velocity to spread apart
    new_ball->dx = template_ball->dx * 0.9f - (template_ball->dx > 0 ? 15.0f : -15.0f);
    new_ball->dy = template_ball->dy * 0.95f;
    new_ball->prev_x = new_ball->x;
    new_ball->prev_y = new_ball->y;
}

/**
 * @brief Applies the BRICK_SPECIAL_PLUS power-up effect.
 */
void apply_plus_powerup(GameState *state) {
    // Increase lives up to MAX_LIVES, otherwise increase score
    if (state->lives < MAX_LIVES) {
        state->lives++;
    } else {
        state->score += 50;
    }
}

/**
 * @brief Initialize bricks for the given level.
 *        If animate=1, bricks start above screen (INCOMING) and drop down.
 *        If animate=0, bricks appear directly at final_y (ACTIVE) without animation.
 */
void init_bricks_for_level(GameState *state, uint8_t level, uint8_t animate) {
    uint16_t brick_colors[BRICK_ROWS] = {BLUE, RED, YELLOW, GREEN, MAGENTA};
    uint16_t total_height = BRICK_ROWS * (BRICK_HEIGHT + BRICK_GAP);
    uint8_t drop_speed = 4 + (level - 1); // 4 px/frame base, +1 per level
    
    srand((unsigned int)HAL_GetTick() + level);
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            Brick *brick = &state->bricks[row][col];
            brick->width = BRICK_WIDTH;
            brick->height = BRICK_HEIGHT;
            brick->x = GRID_PADDING_X + col * (BRICK_WIDTH + BRICK_GAP);
            
            // Calculate final position
            brick->final_y = GRID_START_Y + row * (BRICK_HEIGHT + BRICK_GAP);
            
            if (animate) {
                // Start above screen and drop down (INCOMING state)
                brick->y = (int16_t)(brick->final_y - total_height);
                brick->state = BRICK_STATE_INCOMING;
            } else {
                // Place directly at final position (ACTIVE state, no animation)
                brick->y = brick->final_y;
                brick->state = BRICK_STATE_ACTIVE;
            }
            
            brick->color = brick_colors[row];
            brick->drop_speed = drop_speed;
            
            // Random assignment same probabilities
            uint8_t rand_val = (uint8_t)(rand() % 100);
            if (rand_val < 5) {
                brick->special = BRICK_SPECIAL_BALL;
            } else if (rand_val < 15) {
                brick->special = BRICK_SPECIAL_PLUS;
            } else {
                brick->special = BRICK_SPECIAL_NONE;
            }
        }
    }
}

/**
 * @brief Advance to next level: increase difficulty and reinit bricks.
 */
void advance_level(GameState *state) {
    state->level++;
    // increase ball speed by 15% per level (cap can be added)
    const float SPEED_MULT = 1.15f;
    for (int i = 0; i < state->ball_count; i++) {
        float dx = (float)state->balls[i].dx;
        float dy = (float)state->balls[i].dy;
        dx *= SPEED_MULT;
        dy *= SPEED_MULT;
        state->balls[i].dx = (int16_t)dx;
        state->balls[i].dy = (int16_t)dy;
    }
    // reinitialize bricks for new level with drop animation (animate=1)
    init_bricks_for_level(state, state->level, 1);
}

/**
 * @brief Displays the game over screen.
 */
void game_draw_game_over_screen(const GameState *state) {
    lcd_fill(40, 100, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 100, BLACK);
    lcd_draw_rectangle(40, 100, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 100, RED);

    lcd_show_string_center(-20, 120, "GAME OVER", WHITE, BLACK, 24, 1);

    char score_str[20];
    sprintf(score_str, "Final Score: %05lu", state->score);
    lcd_show_string_center(0, 160, score_str, WHITE, BLACK, 16, 0);
}


// --- Partial Update Functions ---
void game_update_paddle(Paddle *paddle) {
    // Erase old paddle
    lcd_fill(paddle->prev_x, paddle->y, paddle->prev_x + paddle->width, paddle->y + paddle->height, BLACK);
    // Draw new paddle
    draw_paddle(paddle);
    // Update previous position
    paddle->prev_x = paddle->x;
}

void game_update_ball(Ball *ball) {
    // Erase old ball
    lcd_draw_circle(ball->prev_x, ball->prev_y, BLACK, ball->radius, 1);
    // Draw new ball
    draw_ball(ball);
}

/**
 * @brief Erases a single brick from the screen.
 */
void game_erase_brick(const Brick* brick) {
    lcd_fill(brick->x, brick->y, brick->x + brick->width, brick->y + brick->height, BLACK);
}

void game_update_ui_bar(uint32_t score, uint8_t lives, uint8_t level) {
    draw_ui_bar(lives, score, level);
}

// --- Private Drawing Functions ---

static void draw_ui_bar(uint8_t lives, uint32_t score, uint8_t level) {
    // Draw lives as filled circles
	for (int i = 0; i < MAX_LIVES; i++)
		lcd_draw_circle(15 + i * 20, 10, BLACK, 5, 1);
    for (int i = 0; i < MAX_LIVES; i++) {
        // Draw filled circle if lives > i, otherwise hollow
        lcd_draw_circle(15 + i * 20, 10, WHITE, 5, (i < lives));
    }

    // Draw score (example format)
    char score_str[10];
    sprintf(score_str, "%05lu", score);
    lcd_show_string_center(0, 2, score_str, WHITE, BLACK, 16, 0);
    // Draw level on right side
    char level_str[12];
    sprintf(level_str, "LVL:%d", level);
    int len = strlen(level_str);
    int x = SCREEN_WIDTH - (len * 8) - 4;
    lcd_show_string(x, 2, level_str, WHITE, BLACK, 16, 0);
}

void draw_game_border(void) {
    lcd_draw_rectangle(0, UI_BAR_HEIGHT, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, RED);
}

static void draw_bricks(const GameState *state) {
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            const Brick *brick = &state->bricks[row][col];
            // Only draw if brick is in drawable state
            if (brick->state == BRICK_STATE_ACTIVE || brick->state == BRICK_STATE_INCOMING) {
                // Clamp brick position to visible screen area
                int16_t draw_y1 = brick->y;
                int16_t draw_y2 = brick->y + brick->height;
                
                // Skip if completely outside screen
                if (draw_y2 <= UI_BAR_HEIGHT || draw_y1 >= SCREEN_HEIGHT) {
                    continue;
                }
                
                // Clamp to screen bounds
                if (draw_y1 < UI_BAR_HEIGHT) draw_y1 = UI_BAR_HEIGHT;
                if (draw_y2 > SCREEN_HEIGHT) draw_y2 = SCREEN_HEIGHT;
                
                // Draw the visible portion of the brick
                lcd_fill(brick->x, draw_y1, brick->x + brick->width, draw_y2, brick->color);

                // Draw special features if any (only if brick is mostly visible)
                if (brick->y >= UI_BAR_HEIGHT && brick->y + brick->height <= SCREEN_HEIGHT) {
                    switch (brick->special) {
                        case BRICK_SPECIAL_BALL:
                            // Draw a white circle inside the brick
                            lcd_draw_circle(brick->x + brick->width / 2, brick->y + brick->height / 2, WHITE, 6, 0);
                            break;
                        case BRICK_SPECIAL_PLUS:
                            // Draw a black plus sign inside the brick, bigger and balanced
                            {
                                int cross_half_len = (brick->height / 2) - 2;
                                int center_x = brick->x + brick->width / 2;
                                int center_y = brick->y + brick->height / 2;
                                // Vertical line
                                lcd_draw_line(center_x, center_y - cross_half_len, center_x, center_y + cross_half_len, BLACK);
                                // Horizontal line
                                lcd_draw_line(center_x - cross_half_len, center_y, center_x + cross_half_len, center_y, BLACK);
                            }
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
}

static void draw_paddle(const Paddle *paddle) {
    lcd_fill(paddle->x, paddle->y, paddle->x + paddle->width, paddle->y + paddle->height, paddle->color);
}

static void draw_ball(const Ball *ball) {
    lcd_draw_circle(ball->x, ball->y, ball->color, ball->radius, 1); // Filled circle
}


void game_erase_ball(const Ball *ball) {
    lcd_draw_circle(ball->x, ball->y, BLACK, ball->radius, 1);
}
