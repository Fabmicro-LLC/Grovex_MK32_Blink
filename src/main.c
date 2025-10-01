/*
	ПО для платы RobiReceiver - 6-ти канальный приемник Д/У.
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
#include "mik32_hal_adc.h"
#include "mik32_hal_pcc.h"
#include "mik32_hal_irq.h"
#include "mik32_hal_wdt.h"
#include "mik32_hal_i2c.h"
#include "spifi.h"
#include "common.h"
#include "hardware.h"

const char *WELCOME_TEXT = HARDWARE_NAME " " HARDWARE_VER " " __TIME__ " " __DATE__ "\r\n";
 
extern void work(void);
extern void modem_rx(uint8_t modem_rx_buf[], uint32_t modem_rx_buf_length);

uint8_t modem_rx_buf[MODEM_RX_BUF_SIZE];
volatile uint32_t modem_rx_buf_length = 0; 
volatile uint32_t modem_rx_buf_ready = 0;

void InitClock(void)
{
 	// включение тактирования периферии 
	PM->CLK_APB_P_SET |=	PM_CLOCK_APB_P_UART_0_M |
		   		PM_CLOCK_APB_P_UART_1_M |
				PM_CLOCK_APB_P_GPIO_0_M |
				PM_CLOCK_APB_P_GPIO_1_M |
				PM_CLOCK_APB_P_GPIO_2_M |
				PM_CLOCK_APB_P_TIMER16_1_M |
				PM_CLOCK_APB_P_WDT_M |
				PM_CLOCK_APB_P_I2C_0_M;

	// включение тактирования блока для смены режима выводов
	PM->CLK_APB_M_SET |=	PM_CLOCK_APB_M_PAD_CONFIG_M |
				PM_CLOCK_APB_M_WU_M |
				PM_CLOCK_APB_M_PM_M |
				PM_CLOCK_APB_M_EPIC_M;

	// Включение тактирования блока SPIFI (QSPI)
	PM->CLK_AHB_SET =	PM_CLOCK_AHB_SPIFI_M;

	// Установить тактирования Watchdog от внутреннего 32К
	__HAL_WDT_SET_CLOCK(HAL_WDT_LSI32K);

	// Инициализировать и запустить машинный (MTIME) таймер
	SCR1_TIMER->TIMER_CTRL &= ~SCR1_TIMER_CTRL_CLKSRC_M; // По умолчанию - внутр.
	//SCR1_TIMER->TIMER_CTRL |= SCR1_TIMER_CTRL_CLKSRC_INTERNAL_M;
	SCR1_TIMER->TIMER_DIV = 32;
	SCR1_TIMER->TIMER_CTRL |= SCR1_TIMER_CTRL_ENABLE_M;
}

static inline void WDT_SetCON(uint32_t conValue)
{
	intptr_t ptr = (intptr_t)WDT;
	asm volatile(
		"li a0,0x1E\n\t" //Store unlock byte somewhere
		"sb a0,0x9C(%0)\n\t" //Write UNLOCK byte into WDT KEY register
		"sw %1,0x84(%0)\n\t" //Write
		:
		: "r" (ptr), "r" (conValue)
		: "a0"
	);
}


static inline void WDT_SetKEY(uint8_t key)
{
	intptr_t ptr = (intptr_t)WDT;
	asm volatile(
		"li a0,0x1E\n\t" //Store unlock byte somewhere
		"sb a0,0x9C(%0)\n\t" //Write UNLOCK byte into WDT KEY register
		"sb %1,0x9C(%0)\n\t" //Write
		:
		: "r" (ptr), "r" (key)
		: "a0"
	);
}


__attribute__ ((used, section (".spifi.text")))
void InitHardware(void)
{
	// Остановить Watchdog, чтобы не мешал инициализации 
	WDT_SetKEY(WDT_KEY_STOP);

	// Настраиваем выход для светодиодов
	PAD_CFG_OUT(LED_GRN_PORT, LED_GRN_PIN);
	PAD_CFG_OUT(LED_RED_PORT, LED_RED_PIN);

	// Настраиваем вход для MCU_ADDR[0-4] (SW1.6 - SW1.3) 
	PAD_CFG_IN(SW1_ADDRx_PORT, SW1_ADDR0_PIN);
	PAD_CFG_IN(SW1_ADDRx_PORT, SW1_ADDR1_PIN);
	PAD_CFG_IN(SW1_ADDRx_PORT, SW1_ADDR2_PIN);
	PAD_CFG_IN(SW1_ADDRx_PORT, SW1_ADDR3_PIN);
	PAD_PUPD(SW1_ADDRx_PORT, SW1_ADDR0_PIN, HAL_GPIO_PULL_UP);
	PAD_PUPD(SW1_ADDRx_PORT, SW1_ADDR1_PIN, HAL_GPIO_PULL_UP);
	PAD_PUPD(SW1_ADDRx_PORT, SW1_ADDR2_PIN, HAL_GPIO_PULL_UP);
	PAD_PUPD(SW1_ADDRx_PORT, SW1_ADDR3_PIN, HAL_GPIO_PULL_UP);
	
	// Установка параметров UART1 - порт управления модемом (ESP32)
	UART_Init(UART_1, OSC_SYSTEM_VALUE/115200, UART_CONTROL1_TE_M | UART_CONTROL1_RE_M |
						   UART_CONTROL1_M_8BIT_M, 0, 0);
	// Включить Watchdog
	// период WDT = 2,56 сек 
	WDT_SetCON(WDT_CON_PRESCALE_4096_M | WDT_CON_PRELOAD(0xfff - 20)); 
	DelayMs(100);
	WDT_SetKEY(WDT_KEY_START);
}


int main(void)
{
	int ret;

	InitClock(); // Включение тактирования GPIO

	// Установка параметров UART0 - отладочный порт
	UART_Init(UART_0, OSC_SYSTEM_VALUE/115200, UART_CONTROL1_TE_M | UART_CONTROL1_RE_M |
						   UART_CONTROL1_M_8BIT_M, 0, 0);
	DelayMs(100);

	xprintf(WELCOME_TEXT);

	if(ret = spifi_init()) {
		xprintf("SPIFI init failed, ret = %d\n", ret);
		return ret;
	}

	if(ret = spifi_enable_xip()) {
		xprintf("SPIFI XiP failed, ret = %d\n", ret);
		return ret;
	}

	xprintf("Init hardware\n");

	InitHardware();

	// Пропустить инициализацию модема если сработал Watchdog
	if(WDT->STA & WDT_STA_RST_FLAG_M) {
		xprintf("WDT triggered, modem init skipped!\n");
	} else {
		xprintf("Init modem\n");
		//InitModem();
	}

	xprintf("Running...\n");

	while (1) {
	
		WDT_SetKEY(WDT_KEY_START); // Сбросить сторожевой таймер

		work();

		if(modem_rx_buf_ready) {
			modem_rx(modem_rx_buf, modem_rx_buf_length);
			modem_rx_buf_ready = 0;
		}

	}

	return 0;
}

void trap_handler()
{
	//xputs("TRAP!\r\n");

	if (EPIC_CHECK_UART_1()) {

		// RXNE - есть данные в приемном FIFO, читаем и складываем в свой буфер
		while(UART_1->FLAGS & UART_FLAGS_RXNE_M) {
			modem_rx_buf[modem_rx_buf_length++] = UART_1->RXDATA;
			modem_rx_buf_length &= (MODEM_RX_BUF_SIZE-1); // ограничить длину в 128 байт
			UART_1->FLAGS |= UART_FLAGS_RXNE_M; //прерывание обработано, ждать еще данных
		}

		// IDLE - значит больше данных не присылают. Обработаем то, что накопили
		if(UART_1->FLAGS & UART_FLAGS_IDLE_M) {
			// Сигнализировать главному циклу о готовности данных
			modem_rx_buf_ready = 1;

			// Прерывание обработано 
			UART_1->FLAGS |= UART_FLAGS_IDLE_M;
		}
	}

	/*
	if (EPIC_CHECK_TIMER16_1()) {

		uint32_t interrupt_status = HAL_Timer16_GetInterruptStatus(&htimer16_1);

		//xprintf("IRQ: interrupt_status = %08X\r\n", interrupt_status);

		if (interrupt_status & TIMER16_ISR_ARR_MATCH_M) {

			timer_counter++;
		}

		// Сброс нескольких флагов прерываний по маске
		HAL_Timer16_ClearInterruptMask(&htimer16_1, 0xFFFFFFFF); 
	}
	*/

	/* Сброс прерываний */
	HAL_EPIC_Clear(0xFFFFFFFF);
}

