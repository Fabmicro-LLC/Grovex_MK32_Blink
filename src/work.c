#include <mik32_memory_map.h>
#include <pad_config.h>
#include <gpio.h>

#include "xprintf.h"
#include "scr1_timer.h"
#include "hardware.h"

// Явно казать на размещение переменной (указателя) и её данных в NOR Flash (секция .spidi.rodata)
__attribute__((section(".spifi.rodata")))
	const char test_str_in_flash[]
		__attribute__((section(".spifi.rodata"))) = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n";


void ledBlink(void)
{
	GPIO_1->OUTPUT ^= 1 << PIN_LED_GRN; // Установка сигнала вывода 5 порта 1 в противоположный уровень
}

void ledButton(void)
{
	// Проверка состояния состояния входа 15 порта 1
	if (GPIO_1->STATE & (1 << PIN_MCU_ADDR0)) {
		GPIO_1->OUTPUT |= (1 << PIN_LED_RED); // Установка сигнала вывода 6 порта 1 в высокий уровень
	} else {
		GPIO_1->OUTPUT &= ~(1 << PIN_LED_RED); // Установка сигнала вывода 6 порта 1 в низкий уровень
	}
}

__attribute__((used, section(".spifi.text")))
void work(void) {
	static int counter = 0;

	ledBlink(); /* Инвертируем светодиод LED_GRN */

	ledButton(); /* Зажигаем светодиод LED_RED при нажатой кнопке */

	xprintf("Hello, world! Counter = %d, GPIO_1->STATE = 0x%08X\r\n", counter++, GPIO_1->STATE);
	
	xprintf(test_str_in_flash);
	
	for (volatile int i = 0; i < 100000; i++);
}

