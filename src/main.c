/*
	Данный пример предназначен для платы Grovex_MK32 v1.x c NOR Flash памятью. 
	
	В примере настраивается выводы в режим GPIO к которому подключены светодиоды LED_RED и LED_GRN. 
	Доступна функция ledButton, которая считывает состояние переключателя SW1.6 и зажигает светодиод LED_RED.
	
	Код разбит на две части: 1) инициализация исполняемая из EEPROM, и 2) основной цикл work() расположенный
	в NOR Flash. Секция инициализации содержит код инициализации SPIFI интрефейса и перевод его в режим XiP с
	включением кэширования инструкций. 

	Секции кода .text.* и данных помещается в EEPROM.
	Секция кода .spifi помещается в NOR Flash.

	По умолчанию весь код и данные помещается в EEPROM. Для размещения в NOR Flash необходимо явно указывать секцию.
	Для этого, для функции необходимо указать атрибут:

	__attribute__((used, section(".spifi.text")))

	Для read-only данных необходимо указать атрибут:

	__attribute__((section(".spifi.rodata")))


	Иногда, если включена опция LTO (-flto), компилятор может заоптимизировать код нескольких функций в одно целое
	и игнорировать указание секции. В этом случае для функции можно отключить оптимизацию, указав атрибут:

	__attribute__((optimize("O0")))

	Не стоит размещать в NOR Flash обработчики прерываний и прочий код, скорость исполнения которого должна быть
	предсказуемой.
	
*/

#include <mik32_memory_map.h>
#include <pad_config.h>
#include <gpio.h>
#include <power_manager.h>
#include <wakeup.h>

#define	MIK32V2		// Используется в HAL библиотеках для выбора способов инициализации аппаратуры

#include "uart_lib.h"
#include "xprintf.h"
#include "scr1_timer.h"
#include "spifi.h"
#include "hardware.h"

extern void work(void) __attribute__ ((section (".text.spifi")));

void InitClock(void)
{
	// включение тактирования GPIO
	PM->CLK_APB_P_SET |= PM_CLOCK_APB_P_UART_0_M | PM_CLOCK_APB_P_GPIO_0_M | PM_CLOCK_APB_P_GPIO_1_M;

	// включение тактирования блока для смены режима выводов
	PM->CLK_APB_M_SET |= PM_CLOCK_APB_M_PAD_CONFIG_M | PM_CLOCK_APB_M_WU_M | PM_CLOCK_APB_M_PM_M;

	// Включение тактирования блока SPIFI (QSPI)
	PM->CLK_AHB_SET = PM_CLOCK_AHB_SPIFI_M;
}


int main(void)
{
	int ret;

	InitClock(); // Включение тактирования GPIO

	// Установка параметров UART0 - отладочный порт
	UART_Init(UART_0, OSC_SYSTEM_VALUE/115200, UART_CONTROL1_TE_M | UART_CONTROL1_RE_M |
						   UART_CONTROL1_M_8BIT_M, 0, 0);

	xprintf("Blinky started\n");

	xprintf("Init SPIFI\n");
	if(ret = spifi_init()) {
		xprintf("SPIFI init failed, ret = %d\n", ret);
		return ret;
	}

	xprintf("Enabling XiP on SPIFI\n");
	if(ret = spifi_enable_xip()) {
		xprintf("SPIFI XiP failed, ret = %d\n", ret);
		return ret;
	}

	xprintf("Init hardware\n");

	// Установка вывода 5 порта 1 в режим вывод GPIO (LED_GRN)
	PAD_CONFIG->PORT_1_CFG &= ~(0b11 << (2 * PIN_LED_GRN)); // Сборос режимов для вывода 5
	GPIO_1->DIRECTION_OUT = (1 << PIN_LED_GRN); // Установка направления вывода 5 порта 1 на выход

	// Установка вывода 6 порта 1 в режим вывод GPIO (LED_RED)
	PAD_CONFIG->PORT_1_CFG &= ~(0b11 << (2 * PIN_LED_RED)); // Сборос режимов для вывода 9
	GPIO_1->DIRECTION_OUT = (1 << PIN_LED_RED); // Установка направления вывода 6 порта 1 на выход

	// Установка вывода 12 порта 1 в режим вход GPIO (SW1.6 -> PIN_MCU_ADDR0)
	PAD_CONFIG->PORT_1_CFG &= ~(0b11 << (2 * PIN_MCU_ADDR0));
	PAD_CONFIG->PORT_1_PUPD |= (0b01 << (2 * PIN_MCU_ADDR0)); // PULLUP
	GPIO_1->DIRECTION_IN = (1 << PIN_MCU_ADDR0);

	xprintf("Running...\n");

	while (1) {
	
		work();

	}

	return 0;
}

