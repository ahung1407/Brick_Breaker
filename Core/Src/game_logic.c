#include "game_logic.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
uint8_t circle_aabb_overlap(int16_t cx, int16_t cy, uint16_t radius,
                          uint16_t rx, uint16_t ry,
                          uint16_t rw, uint16_t rh) {
    // Find the closest point to the circle within the rectangle
    int16_t closest_x = CLAMP(cx, rx, rx + rw);
    int16_t closest_y = CLAMP(cy, ry, ry + rh);

    // Calculate the distance between the circle's center and this closest point
    int16_t distance_x = cx - closest_x;
    int16_t distance_y = cy - closest_y;

    // If the distance is less than the circle's radius, an intersection occurs
    uint32_t distance_squared = (distance_x * distance_x) + (distance_y * distance_y);
    return distance_squared < (radius * radius);
}

uint8_t resolve_ball_brick(Ball *ball, Brick *brick) {
    if (brick->state == BRICK_STATE_DESTROYED) {
        return 0; // No collision if brick is already destroyed
    }
    
    // Only allow collision if brick is visible in game area (top edge reached the game area)
    if (brick->y + brick->height <= UI_BAR_HEIGHT) {
        return 0; // Brick is still above game area
    }

    if (!circle_aabb_overlap(ball->x, ball->y, ball->radius,
                             brick->x, brick->y,
                             brick->width, brick->height)) {
        return 0; // No collision
    }
    /*
    * Check the side of collision and adjust ball velocity accordingly: vertical or horizontal or corner
    */
    uint8_t overlapL = ball->x + ball->radius - brick->x;
    uint8_t overlapR = (brick->x + brick->width) - (ball->x - ball->radius);
    uint8_t overlapT = ball->y + ball->radius - brick->y;
    uint8_t overlapB = (brick->y + brick->height) - (ball->y - ball->radius);
    uint8_t minOverlapX = (overlapL < overlapR) ? overlapL : overlapR;
    uint8_t minOverlapY = (overlapT < overlapB) ? overlapT : overlapB;

    float crit45 = fmaxf(CRIT45_FLOOR, ball->radius * CRIT45_SCALE);
    if (fabsf(minOverlapX - minOverlapY) <= crit45) {
        // Corner collision
        ball->dx = -ball->dx;
        ball->dy = -ball->dy;
        // Nudge ball out of collision
        ball->x += (overlapL < overlapR) ? -1 : 1;
        ball->y += (overlapT < overlapB) ? -1 : 1;
    } else if (minOverlapX < minOverlapY) {
        // Vertical collision
        ball->dx = -ball->dx;
        ball->x = (overlapL < overlapR) ? (brick->x - ball->radius) : (brick->x + brick->width + ball->radius);
    } else {
        // Horizontal collision
        ball->dy = -ball->dy;
        ball->y = (overlapT < overlapB) ? (brick->y - ball->radius) : (brick->y + brick->height + ball->radius);
    }
    brick->state = BRICK_STATE_DESTROYED;
    return 1; // Collision occurred
}

uint8_t resolve_ball_paddle(Ball *ball, const Paddle *paddle) {
    if (!circle_aabb_overlap(ball->x, ball->y, ball->radius,
                             paddle->x, paddle->y,
                             paddle->width, paddle->height)) {
        return 0; // No collision
    }

    // Simple reflection logic
    ball->dy = -fabsf(ball->dy); // Always reflect upwards

    // Adjust horizontal velocity based on where it hit the paddle
    float hitPos = (float)(ball->x - paddle->x) / (float)paddle->width; // 0.0 (left) to 1.0 (right)
    ball->dx = (hitPos - 0.5f) * 2.0f * V_X_MAX; // Scale to max horizontal speed

    // Clamp ball position to be just above the paddle
    ball->y = paddle->y - ball->radius - 1;

    return 1; // Collision occurred
}

uint8_t resolve_ball_wall(Ball *ball) {
    uint8_t collided = 0; 
    // collided = 0: no collision
    // collided = 1: wall collision
    // collided = 2: out of bounds (bottom)

    // Left wall
    if (ball->x - ball->radius <= 0) {
        ball->dx = fabsf(ball->dx);
        ball->x = ball->radius + 1;
        collided = 1;
    }
    // Right wall
    else if (ball->x + ball->radius >= SCREEN_WIDTH - 1) {
        ball->dx = -fabsf(ball->dx);
        ball->x = SCREEN_WIDTH - ball->radius - 1;
        collided = 1;
    }
    // Top wall
    if (ball->y - ball->radius <= UI_BAR_HEIGHT) {
        ball->dy = fabsf(ball->dy);
        ball->y = UI_BAR_HEIGHT + ball->radius + 1;
        collided = 1;
    }
    // Bottom wall (missed paddle)
    else if (ball->y + ball->radius >= SCREEN_HEIGHT - 1) {
        // Ball is out of bounds, typically handled as a life lost
        collided = 2; // Indicate out of bounds
    }

    return collided;
}

void initialize_ball_velocity(Ball *ball) {
    // Start with a fixed angle upwards
    ball->dx = 0.0f;
    ball->dy = -V_MIN;
}

void step_world(GameState *state, float dt) {
    // update all balls; be careful khi xóa ball trong vòng lặp
    for (int i = 0; i < state->ball_count; ) {
        Ball *b = &state->balls[i];
        b->x += b->dx * dt;
        b->y += b->dy * dt;

        uint8_t wc = resolve_ball_wall(b);
        if (wc == 2) { // out of bounds
            // Remove this ball from array (swap-with-last)
        	game_update_ball(b);
            game_erase_ball(b);
            state->balls[i] = state->balls[state->ball_count - 1];
            state->ball_count--;
            continue; // do not increment i, process new occupant
        } else {
            if (wc == 1) {
                // optionally play sound
            }
            // paddle collision
            resolve_ball_paddle(b, &state->paddle);

            // brick collisions: iterate bricks and call resolve_ball_brick(b, brick)
            for (int row = 0; row < BRICK_ROWS; row++) {
                for (int col = 0; col < BRICK_COLS; col++) {
                    Brick *brick = &state->bricks[row][col];
                    if (resolve_ball_brick(b, brick)) {
                        game_erase_brick(brick);
                        state->score += 10;
                        // special handling:
                        if (brick->special == BRICK_SPECIAL_BALL) {
                            spawn_extra_ball(state, b);
                        } else if (brick->special == BRICK_SPECIAL_PLUS) {
                            apply_plus_powerup(state);
                        }
                    }
                }
            }
            i++; // only advance if this ball remains
        }
    }

    // After loop: check if all bricks destroyed -> advance level
    int active_bricks = 0;
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            if (state->bricks[row][col].state == BRICK_STATE_ACTIVE) active_bricks++;
        }
    }
    if (active_bricks == 0) {
        // advance to next level
        advance_level(state);
        // redraw initial scene to show new bricks
        game_draw_initial_scene(state);
        return; // skip life-check this frame
    }

    // If not advancing level, continue to handle balls out-of-bounds
    if (state->ball_count == 0) {
        state->lives--;
        if (state->lives == 0) {
            state->status = GAME_OVER;
            game_draw_game_over_screen(state);
            return;
        } else {
            // reset one ball above paddle and set ball_count = 1
            Ball *b = &state->balls[0];
            b->x = state->paddle.x + state->paddle.width / 2.0f;
            b->y = state->paddle.y - b->radius - 1;
            b->dx = 0; b->dy = -V_MIN;
            state->ball_count = 1;
            state->show_potentiometer_prompt = 1;
            game_draw_initial_scene(state);
        }
    }
}


// spawn_extra_ball and other special effects are handled in game_ui.c
