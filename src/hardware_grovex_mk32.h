
/******************** Описание аппаратуры для платы Grovex_MK32 ********************************/
	
#define HARDWARE_NAME	"Grovex_MK32"
#define	HARDWARE_VER	"V1.0"

// LED_RED - P1.6. Светодиод индикации состояние АВАРИЯ 
#define	LED_RED_PORT		1
#define	LED_RED_PIN		6

// LED_GRN - P1.5. Светодиод индикации состояния РАБОТА.
#define	LED_GRN_PORT		1
#define	LED_GRN_PIN		5


// SW1[3-6] = MCU_ADDR[3-0] - P1.15 - P1.12.
// Входной дискретный GPIO с внутренней подтяжкой вверх.
#define	MCU_ADDRx_PORT		1
#define MCU_ADDR0_PIN		12
#define MCU_ADDR1_PIN		13
#define MCU_ADDR2_PIN		14
#define MCU_ADDR3_PIN		15

// SW1[1-2] = MCU_BAUB[1-0] - P1.7 - P0.8.
// Входной дискретный GPIO с внутренней подтяжкой вверх.
#define	MCU_BAUD1_PORT		1
#define	MCU_BAUD0_PORT		0
#define MCU_BAUD1_PIN		7	
#define MCU_BAUD0_PIN		8	

