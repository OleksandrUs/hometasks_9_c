/*
 * homework, main.c
 * Purpose: create FreeRTOS project and compile.
 * 
 * In the program the effect of 'rotating colored lights at variable speed' 
 * is created by means of using two threads (tasks) - one to change the LEDs state
 * and the other to change time delays between LEDs state changes.
 *
 * @author Oleksandr Ushkarenko
 * @version 1.0 19/09/2021
 */

#include "stm32f3xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

/*
 * These identifiers are used to determine the microcontroller pins
 * the eight color the LEDs connected to.
 */
#define BLUE_LED_1		GPIO_PIN_8
#define RED_LED_1 		GPIO_PIN_9
#define ORANGE_LED_1 	GPIO_PIN_10
#define GREEN_LED_1		GPIO_PIN_11
#define BLUE_LED_2 		GPIO_PIN_12
#define RED_LED_2 		GPIO_PIN_13
#define ORANGE_LED_2	GPIO_PIN_14
#define GREEN_LED_2 	GPIO_PIN_15

/*
 * These identifiers are used to determine the direction of variable changing
 * (i.e. increment or decrement) that is used to make time delays and to change the speed of
 * the LEDs switching.  
 */
#define UP 0U
#define DOWN 1U

/*
 * The size of the stack (in 4-byte words) for created tasks.
 */
#define STACK_SIZE 32U

/*
 * These identifiers are used to determine time delays - minimum and maximum
 * pauses between the LEDs state changes, default value of delay, the delay increment
 * and decrement step value and the pause between LEDs state changing. 
 * All these values were chosen experimentally to make the effect of LEDs 
 * state changing more appealing.
 */
#define MIN_DELAY 25U
#define MAX_DELAY 300U
#define INITIAL_DELAY 300U
#define DELAY_STEP 15
#define PAUSE 200

/*
 * The algorithm of the LEDs state changing is implemented as a digital automata
 * (Mealy state machine) and these identifiers are used to determile the states
 * of a state machine; S0 is the initial state.
 */
#define S0 0U
#define S1 1U
#define S2 2U
#define S3 3U

/*
 * Declaration of the function prototypes.
 */
void GPIO_Init(void);
void led_controller_task(void * params);
void speed_controller_task(void * params);
void change_state(uint8_t * pState);
void change_led_state(const uint8_t * pState);
void error_handler(void);

/*
 * This global variable is used to store the value of time delay
 * between the LEDs state changes. The value of this variable is changed in
 * speed_controller_task function and used in led_controller_task function.
 */
volatile uint32_t delay = INITIAL_DELAY;

/*
 * This variable is used to determine the direction of time delay changing
 * (i.e. increasing or decreasing); the initial value is UP means that
 * the delay will be increased until it reaches the maximum value.
 */
uint8_t direction = UP;


/*
 * The main function of the program (the entry point).
 * Demonstration of creating two tasks (changing the LEDs state and the time delay
 * between state shanges to create the effect of 'rotating colored lights at variable speed').
 */
int main()
{
	GPIO_Init();
	
	BaseType_t result;
	
	result = xTaskCreate(led_controller_task, "LED Controller Task", STACK_SIZE, NULL, 1, NULL);
	if(result != pdPASS){
		error_handler();
	}
	
	result = xTaskCreate(speed_controller_task, "Speed Controller Task", STACK_SIZE, NULL, 1, NULL);
	if(result != pdPASS){
		error_handler();
	}
	
	vTaskStartScheduler();
	while(1) {}
}

/*
 * The function is used to initialize I/O pins of port E (GPIOE). 
 * All microcontroller pins the LEDs connected to configured to output.
 * The push-pull mode is used, no pull-ups.
 */
void GPIO_Init()
{
	__HAL_RCC_GPIOE_CLK_ENABLE();

	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 
	| GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);

	GPIO_InitTypeDef gpio_init_struct;
	gpio_init_struct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
												 GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_init_struct.Pull = GPIO_NOPULL;
	gpio_init_struct.Speed = GPIO_SPEED_LOW;
	
	HAL_GPIO_Init(GPIOE, &gpio_init_struct);
}

/*
 * This is a task function (thread) in which the function of the LEDs state change
 * is periodically invoked.
 */
void led_controller_task(void * params)
{
	uint8_t state = S0;
	while(1) {
		change_state(&state);
		vTaskDelay(pdMS_TO_TICKS(delay));
	}
}

/*
 * This is a task function (thread) in which the value of time delay is changed.
 * The value of delay is increasing with specified step until it reaches the maximum 
 * value and then the value of delay is decreasing until it reaches the minimum value.
 * The direction variable is used as a flag (a kind of flip-flop) to determine 
 * the direction of delay changing (increment or decrement). 
 */
void speed_controller_task(void * params)
{
	while(1) {
		if(direction == UP) {
			while(delay < MAX_DELAY) {
				delay += DELAY_STEP;
				vTaskDelay(PAUSE);
			}
			direction = DOWN;
		} else {
				while(delay > MIN_DELAY) {
					delay -= DELAY_STEP;
					vTaskDelay(PAUSE);
			  }
				direction = UP;
		}
	}
}

/*
 * In this function the Mealy state machine to change the LEDs state is implemented.
 * The state changes periodically in order S0 -> S1 -> S2 -> S3 -> S0 etc. 
 */
void change_state(uint8_t *pState)
{
		switch(*pState) {
			case S0:
				*pState = S1;
				break;
			case S1:
				*pState = S2;
				break;
			case S2:
				*pState = S3;
				break;
			case S3:
				*pState = S0;
				break;
			default:
				*pState = S0;
				break;
		}
	
	change_led_state(pState);
}

/*
 * This function is used to control the LEDs state depending on the state of
 * the digital automata. A coulpe of switched-on LEDs indicates the current state.
 */
void change_led_state(const uint8_t *pState)
{
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 
	| GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);

		switch(*pState) {
			case S0:
				HAL_GPIO_WritePin(GPIOE, BLUE_LED_1 | BLUE_LED_2, GPIO_PIN_SET);
				break;
			case S1:
				HAL_GPIO_WritePin(GPIOE, RED_LED_1 | RED_LED_2, GPIO_PIN_SET);
				break;
			case S2:
				HAL_GPIO_WritePin(GPIOE, ORANGE_LED_1 | ORANGE_LED_2, GPIO_PIN_SET);
				break;
			case S3:
				HAL_GPIO_WritePin(GPIOE, GREEN_LED_1 | GREEN_LED_2, GPIO_PIN_SET);
				break;
			default:
				HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 
				| GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);
				break;
		}
}

/*
 * The function is used as an error handler: if an error occures, this function
 * is invoked and two red LEDs on board will be switched on.
 */
void error_handler(void)
{
	HAL_GPIO_WritePin(GPIOE, RED_LED_1 | RED_LED_2, GPIO_PIN_SET);
	while(1){	}
}

