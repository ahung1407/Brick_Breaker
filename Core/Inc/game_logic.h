#include "game_ui.h"
#include "main.h"


// --- Parameter for logic ---
#define V_MIN 60
#define V_MAX 220
#define V_X_MAX 180
#define K_SPIN 0.18f
#define CRIT45_SCALE 0.7071f // cos(45°) or sin(45°)
#define CRIT45_FLOOR 2.0f


// --- Macro for clamping values ---
#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

// --- Function Logic Helper Prototypes ---
uint8_t circle_aabb_overlap(int16_t cx, int16_t cy, uint16_t radius,
                          uint16_t rx, uint16_t ry,
                          uint16_t rw, uint16_t rh);
uint8_t resolve_ball_brick(Ball *ball, Brick *brick);
uint8_t resolve_ball_paddle(Ball *ball, const Paddle *paddle);
uint8_t resolve_ball_wall(Ball *ball);
void initialize_ball_velocity(Ball *ball);
void step_world(GameState *state, float dt);

// for future paddle mechanics 
void apply_spin_to_ball(Ball *ball, int16_t paddle_dx);