/*
 * button.h
 */

#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#define DEBOUNCE_THRESHOLD 3

/* Includes */
#include <stdint.h>

/* Variables */
extern uint16_t button_count[16];

/* Functions */
extern void button_init();
extern void button_scan();

#endif /* INC_BUTTON_H_ */
