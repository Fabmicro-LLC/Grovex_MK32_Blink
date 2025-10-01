
/******************** Описание аппаратуры для платы Grovex_MK32 ********************************/
	
#define HARDWARE_NAME	"Grovex_MK32"
#define	HARDWARE_VER	"V1.0"

// LED_RED - P1.6. Светодиод индикации состояние АВАРИЯ 
#define	LED_RED_PORT		1
#define	LED_RED_PIN		6

// LED_GRN - P1.5. Светодиод индикации состояния РАБОТА.
#define	LED_GRN_PORT		1
#define	LED_GRN_PIN		5


// SW1_ADDR[0-3] - P1.12 - P1.15.
// Входной дискретный GPIO с внутренней подтяжкой вверх.
#define	SW1_ADDRx_PORT		1
#define SW1_ADDR0_PIN		12
#define SW1_ADDR1_PIN		13
#define SW1_ADDR2_PIN		14
#define SW1_ADDR3_PIN		15


