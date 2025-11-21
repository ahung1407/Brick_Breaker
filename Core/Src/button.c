/*
 * button.c
 */

/* Includes */
#include "button.h"

#include "spi.h"
#include "gpio.h"

/* Variables */
uint16_t button_count[16] = { 0 };
static uint16_t button_spi_buffer = 0x0000;
uint8_t button_state[16] = {0};//

/* Functions */
/**
 * @brief  	Init matrix button
 * @param  	None
 * @retval 	None
 */
void button_init() {
	HAL_GPIO_WritePin(BTN_LOAD_GPIO_Port, BTN_LOAD_Pin, 1);
}

/**
 * @brief  	Scan matrix button
 * @param  	None
 * @note  	Call every 50ms
 * @retval 	None
 */
//void button_scan() {
//	HAL_GPIO_WritePin(BTN_LOAD_GPIO_Port, BTN_LOAD_Pin, 0);
//	HAL_GPIO_WritePin(BTN_LOAD_GPIO_Port, BTN_LOAD_Pin, 1);
//	HAL_SPI_Receive(&hspi1, (void*) &button_spi_buffer, 2, 10);
//
//	int button_index = 0;
//	uint16_t mask = 0x8000;
//	for (int i = 0; i < 16; i++) {
//		if (i >= 0 && i <= 3) {
//			button_index = i + 4;
//		} else if (i >= 4 && i <= 7) {
//			button_index = 7 - i;
//		} else if (i >= 8 && i <= 11) {
//			button_index = i + 4;
//		} else {
//			button_index = 23 - i;
//		}
//		if (button_spi_buffer & mask)
//			button_count[button_index] = 0;
//		else
//			button_count[button_index]++;
//		mask = mask >> 1;
//	}
//}

void button_scan() {
	// 1. Tải/Chốt trạng thái nút nhấn từ các đầu vào song song vào thanh ghi
	HAL_GPIO_WritePin(BTN_LOAD_GPIO_Port, BTN_LOAD_Pin, 0);
	HAL_GPIO_WritePin(BTN_LOAD_GPIO_Port, BTN_LOAD_Pin, 1);

	// 2. Đọc 2 byte (16 bit) dữ liệu từ Shift Register qua SPI
	// Timeout 10ms (tùy thuộc vào tốc độ SPI của bạn)
	HAL_SPI_Receive(&hspi1, (void*) &button_spi_buffer, 2, 10);

	int button_index = 0;
	uint16_t mask = 0x8000; // Bắt đầu từ Bit 15 (MSB)

	for (int i = 0; i < 16; i++) {
		// Ánh xạ tùy chỉnh từ chỉ số bit (i) sang chỉ số nút (button_index)
		if (i >= 0 && i <= 3) {
			button_index = i + 4;
		} else if (i >= 4 && i <= 7) {
			button_index = 7 - i;
		} else if (i >= 8 && i <= 11) {
			button_index = i + 4;
		} else { // i >= 12 && i <= 15
			button_index = 23 - i;
		}

        // --------------------------------------------------------
        // Logic Chống Rung (Debouncing Logic)
        // --------------------------------------------------------

		if (button_spi_buffer & mask) {
			// A. Nút đang THẢ (Bit = 1) - Do pull-up

            // Nếu trạng thái ổn định trước đó là nhấn (1), chuyển nó về thả (0)
            if (button_state[button_index] == 1) {
                button_state[button_index] = 0;
            }
            // Reset bộ đếm bất kể trạng thái
			button_count[button_index] = 0;

		} else {
			// B. Nút đang NHẤN (Bit = 0)

			// 1. Tăng bộ đếm chỉ khi chưa đạt ngưỡng
			if (button_count[button_index] < DEBOUNCE_THRESHOLD) {
                button_count[button_index]++;
            }

            // 2. Nếu bộ đếm đã đạt ngưỡng và trạng thái ổn định đang là thả (0),
            // xác nhận đây là sự kiện nhấn nút ổn định (Debounced Press)
            if (button_count[button_index] >= DEBOUNCE_THRESHOLD && button_state[button_index] == 0) {
                button_state[button_index] = 1; // Cập nhật trạng thái ổn định
            }
		}

		mask = mask >> 1;
	}
}

