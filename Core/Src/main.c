#include "bsp.h"
#include "delay.h"
#include "lcd.h"
#include <stdint.h>
#include <string.h>
// #include "adc.h"
#include "adxl345.h"
#include "bme280_intf.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"

#define RX_BUF_LENGTH 32
#define TX_BUF_LENGTH 32

#define USART_MESSAGE_LENGTH 32

static program_state_t program_state;

typedef enum {
    COMMUNICATION_MODE_POLLING,
    COMMUNICATION_MODE_INTERRUPT,
    COMMUNICATION_MODE_DMA
} communication_mode_t;

/* LCD configuration */
lcd_config_t lcd_config = {
    /* User-defined blocking delay function */
    .delay_us = delay_us,
    /* GPIO pin configuration */
    .data_ports = {GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB},
    .rs_port    = GPIOB,
    .rw_port    = GPIOB,
    .e_port     = GPIOB,
    .data_pins  = {5, 6, 7, 11, 12, 13, 14, 15},
    .rs_pin     = 4,
    .rw_pin     = 3,
    .e_pin      = 2,
    /* LCD settings */
    .direction = 1, // configure LCD GPIO pins as output
    .id        = 1, // increment AC (and hence cursor) on write
    .s         = 0, // don't shift display on write
    .d         = 1, // display on
    .c         = 1, // cursor on
    .b         = 0, // blinking off
    .dl        = 1, // 8-bit interface
    .n         = 1, // 2-line display
    .f         = 0, // 5 x 8 font
};

/* USART configuration */
volatile uint16_t usart_rx_buf[BSP_MAX_COMMAND_LENGTH] = {0xFD, 0x1,
                                                          0xFE}; // initialize program in idle state
volatile uint16_t usart_tx_buf[BSP_MAX_COMMAND_LENGTH];
uint16_t          usart_operands[USART_MESSAGE_LENGTH];
usart_config_t    usart_config = {.word_length           = USART_WORD_LENGTH_8BIT,
                                  .baud_rate             = 0x68,
                                  .stop_bits             = USART_STOP_BITS_1BIT,
                                  .oversampling_mode     = USART_OVERSAMPLING_MODE_16X,
                                  .fifo_mode             = 0,
                                  .parity_control_enable = 0,
                                  .parity_selection      = USART_PARITY_SELECTION_EVEN,
                                  .msb_first             = 0,
                                  .clock_polarity        = 0,
                                  .clock_phase           = 0,
                                  .last_bit_clock_pulse  = 0,
                                  .txfifo_threshold      = USART_TXFIFO_THRESHOLD_2LEVEL,
                                  .rxfifo_threshold      = USART_RXFIFO_THRESHOLD_2LEVEL,
                                  .clock_prescaler       = USART_CLOCK_PRESCALER_NONE,
                                  .character_match       = BSP_STOP_COMMAND};

/* ADXL345 configuration */
volatile uint8_t i2c_rx_buf[RX_BUF_LENGTH];
volatile uint8_t i2c_tx_buf[TX_BUF_LENGTH];
volatile int16_t x = 0, y = 0, z = 0;
adxl345_config_t adxl345_config = {/* User-defined blocking delay function */
                                   .delay_us = delay_us,
                                   /* Communication in I2C mode */
                                   .communication_format = ADXL345_I2C_MODE,
                                   /* Data transfer settings */
                                   .rate       = 0xD,
                                   .resolution = ADXL345_FULL_RES,
                                   .justify    = ADXL345_RIGHT_ALIGNED,
                                   .range      = 0x3,
                                   /* Interrupt settings */
                                   .interrupt_polarity = ADXL345_INTERRUPT_POLARITY_ACTIVE_HIGH,
                                   .interrupt_enable   = 0x0,
                                   .interrupt_map      = 0x0,
                                   /* FIFO settings */
                                   .fifo_mode = ADXL345_STREAM_MODE,
                                   /* I2C variables */
                                   .rx_buf = i2c_rx_buf,
                                   .tx_buf = i2c_tx_buf};

/* BME280 configuration */
uint32_t             pressure           = 0;
int32_t              temperature        = 0;
uint32_t             humidity           = 0;
bme280_intf_config_t bme280_intf_config = {
    .settings =
        {
            .osr_p        = 0x01, // x1 oversampling for pressure
            .osr_t        = 0x01, // x1 oversampling for temperature
            .osr_h        = 0x01, // x1 oversampling for humidity
            .filter       = 0x00, // filter off
            .standby_time = 0x00  // 0.5ms standby time in normal mode
        },
    .intf = BME280_I2C_INTF // I2C communication
};

/* Program global variables */
program_prev_state_t program_prev_state = {.prev_state_before_transition = PROGRAM_STATE_IDLE,
                                           .prev_command                 = {0xFD, 0x1, 0xFE},
                                           .prev_command_length          = 3};
program_state_t      prev_state         = PROGRAM_STATE_IDLE;
uint8_t              adxl345_data_ready = 0;
uint8_t              bme280_data_ready  = 0;
uint8_t              tx_data_ready      = 0;
char                 usart_msg_first_line[LCD_COLS + 1];
char                 usart_msg_second_line[LCD_COLS + 1];

uint32_t USART_thread_stack[128];
OSThread USART_thread;
void     USART_threadHandler() {
    static uint8_t         measurement_idx                         = 0;
    static uint16_t        current_command[BSP_MAX_COMMAND_LENGTH] = {0xFD, 0x01, 0xFE};
    static program_state_t current_state                           = PROGRAM_STATE_IDLE;
    uint8_t                tx_data_sent                            = 0;
    while (1) {
        USART_read(BSP_MAX_COMMAND_LENGTH, usart_rx_buf, COMMUNICATION_MODE_INTERRUPT);
        uint8_t command_length = BSP_calculateCommandLength(usart_rx_buf);
        if (command_length >= BSP_MIN_COMMAND_LENGTH) {
            current_state = BSP_getProgramState(usart_rx_buf);
            if (current_state != program_state) {
                BSP_saveProgramState(current_command, &program_state, &program_prev_state);
                BSP_copyArray(current_command, usart_rx_buf, BSP_MAX_COMMAND_LENGTH);
                program_state = current_state;
            }
            if (program_state == PROGRAM_STATE_TRANSMIT_DATA && tx_data_ready) {
                uint8_t num_tx_data = current_command[2];
                usart_tx_buf[0]     = (uint16_t)(x & 0xFF);
                usart_tx_buf[1]     = (uint16_t)((x >> 8) & 0xFF);
                usart_tx_buf[2]     = (uint16_t)(y & 0xFF);
                usart_tx_buf[3]     = (uint16_t)((y >> 8) & 0xFF);
                usart_tx_buf[4]     = (uint16_t)(z & 0xFF);
                usart_tx_buf[5]     = (uint16_t)((z >> 8) & 0xFF);
                usart_tx_buf[6]     = (uint16_t)pressure;
                usart_tx_buf[7]     = (uint16_t)temperature;
                usart_tx_buf[8]     = (uint16_t)humidity;

                if (measurement_idx == num_tx_data - 1) {
                    usart_tx_buf[9] = '\n';
                }

                tx_data_sent = USART_write(9 + (measurement_idx == num_tx_data - 1 ? 1 : 0),
                                           usart_tx_buf, COMMUNICATION_MODE_INTERRUPT);

                if (tx_data_sent) {
                    if (measurement_idx < num_tx_data) {
                        measurement_idx++;
                    }
                    if (measurement_idx == num_tx_data) {
                        measurement_idx = 0;
                        BSP_restorePreviousProgramState(current_command, &program_state,
                                                        &program_prev_state);
                        BSP_copyArray(usart_rx_buf, current_command, BSP_MAX_COMMAND_LENGTH);
                    }
                    tx_data_ready = 0;
                    tx_data_sent  = 0;
                }
            } else if (program_state == PROGRAM_STATE_TYPE_TO_LCD) {
                uint8_t string_length = command_length - 3;
                BSP_formatMsg(&usart_rx_buf[2], usart_msg_first_line, usart_msg_second_line,
                              string_length);
            }
        }
        OS_delay(1U);
    }
}
// void USART_threadHandler() {
//	static uint8_t measurement_idx = 0;
//	static uint16_t usart_rx_buf_before_transition[BSP_MAX_COMMAND_LENGTH] = {0xFD, 0x1, 0xFE};
//	static program_state_t prev_state_before_transition = PROGRAM_STATE_IDLE;
//	static program_state_t current_state = PROGRAM_STATE_IDLE;
//	static uint8_t state_transition = 0;
//	uint8_t tx_data_sent = 0;
//     while (1) {
////    	debug_write(0, 1);
////    	rx_data_read = USART_start(BSP_MAX_COMMAND_LENGTH, 0, usart_rx_buf, (volatile
/// uint16_t*)0, COMMUNICATION_MODE_INTERRUPT);
//    	prev_state = program_state;
//    	BSP_copyArray(usart_rx_buf_before_transition, usart_rx_buf, BSP_MAX_COMMAND_LENGTH);
//    	USART_read(BSP_MAX_COMMAND_LENGTH, usart_rx_buf, COMMUNICATION_MODE_INTERRUPT);
//		uint8_t command_length = BSP_calculateCommandLength(usart_rx_buf);
//		if(command_length >= BSP_MIN_COMMAND_LENGTH){
//			current_state = BSP_getProgramState(usart_rx_buf);
//			if(current_state != prev_state){
//				prev_state_before_transition = program_state;
//				state_transition = 1;
//			}
//			program_state = current_state;
//			if(program_state == PROGRAM_STATE_TRANSMIT_DATA && tx_data_ready){
//				uint8_t num_tx_data = usart_rx_buf[2];
//				usart_tx_buf[0] = (uint16_t)(x & 0xFF);
//				usart_tx_buf[1] = (uint16_t)((x >> 8) & 0xFF);
//				usart_tx_buf[2] = (uint16_t)(y & 0xFF);
//				usart_tx_buf[3] = (uint16_t)((y >> 8) & 0xFF);
//				usart_tx_buf[4] = (uint16_t)(z & 0xFF);
//				usart_tx_buf[5] = (uint16_t)((z >> 8) & 0xFF);
//				usart_tx_buf[6] = (uint16_t)pressure;
//				usart_tx_buf[7] = (uint16_t)temperature;
//				usart_tx_buf[8] = (uint16_t)humidity;
//				if(measurement_idx == num_tx_data - 1){
//					usart_tx_buf[9] = '\n';
////					program_state =
/// program_prev_state.prev_state_before_transition;
//					BSP_restorePreviousProgramState(usart_rx_buf,
//&program_state, &program_prev_state);
//				}
//				tx_data_sent = USART_write(9 + (measurement_idx == num_tx_data - 1 ?
// 1 : 0), usart_tx_buf, COMMUNICATION_MODE_INTERRUPT);
//
//				if(tx_data_sent){
//					if(measurement_idx < num_tx_data){
//						measurement_idx++;
//					}
//					if(measurement_idx == num_tx_data){
//						measurement_idx = 0;
//					}
//					tx_data_ready = 0;
//					tx_data_sent = 0;
//				}
//			}
//			else if(program_state == PROGRAM_STATE_TYPE_TO_LCD){
//				uint8_t string_length = command_length - 3; // remove start, command
// and stop bytes 				BSP_formatMsg(&usart_rx_buf[2],
// usart_msg_first_line, usart_msg_second_line, string_length);
//			}
//		}
//    	if(state_transition){
////    		program_prev_state.prev_state_before_transition =
/// prev_state_before_transition;
//			BSP_saveProgramState(usart_rx_buf_before_transition,
//&prev_state_before_transition, &program_prev_state); 			state_transition = 0;
//		}
////    	debug_write(0, 0);
//    	OS_delay(1U);
//    }
//}

uint32_t I2C_thread_stack[128];
OSThread I2C_thread;
void     I2C_threadHandler() {
    while (1) {
        //    	debug_write(1, 1);
        switch (program_state) {
        case PROGRAM_STATE_DISPLAY_ADXL345_MEASUREMENTS:
            adxl345_data_ready = ADXL345_readData(&x, &y, &z, COMMUNICATION_MODE_INTERRUPT);
            break;
        case PROGRAM_STATE_DISPLAY_BME280_MEASUREMENTS:
            bme280_data_ready = bme280_intf_getSensorData(BME280_ALL);
            pressure          = (uint32_t)bme280_intf_config.data->pressure / 1000;
            temperature       = (int32_t)bme280_intf_config.data->temperature / 100;
            humidity          = (uint32_t)bme280_intf_config.data->humidity / 1000;
            break;
        case PROGRAM_STATE_TRANSMIT_DATA:
            adxl345_data_ready = ADXL345_readData(&x, &y, &z, COMMUNICATION_MODE_INTERRUPT);
            bme280_data_ready  = bme280_intf_getSensorData(BME280_ALL);
            pressure           = (uint32_t)bme280_intf_config.data->pressure / 1000;
            temperature        = (int32_t)bme280_intf_config.data->temperature / 100;
            humidity           = (uint32_t)bme280_intf_config.data->humidity / 1000;
            tx_data_ready      = adxl345_data_ready && bme280_data_ready;
            break;
        default:
        case PROGRAM_STATE_IDLE:
            break;
        }
        //    	debug_write(1, 0);
        //        OS_delay(10U); /* block for 1 tick */
        OS_delay(1U); /* block for 1 tick */
    }
}

uint32_t LCD_thread_stack[256];
OSThread LCD_thread;
void     LCD_threadHandler() {
    static program_state_t previous_lcd_thread_state = PROGRAM_STATE_IDLE;
    while (1) {
        //    	USART_convertWordToMsg(usart_rx_buf, usart_msg, USART_MESSAGE_LENGTH);
        //    	LCD_clearDisplay();
        if (program_state != previous_lcd_thread_state) {
            LCD_clearDisplay();
            previous_lcd_thread_state = program_state;
        }
        switch (program_state) {
        case PROGRAM_STATE_DISPLAY_ADXL345_MEASUREMENTS:
            if (adxl345_data_ready) {
                char x_string[7 + 1];
                char y_string[7 + 1];
                char z_string[7 + 1];
                snprintf(x_string, 7 + 1, "x %d", x);
                snprintf(y_string, 7 + 1, "y %d", y);
                snprintf(z_string, 7 + 1, "z %d", z);
                LCD_print(x_string, 0, 0, 7);
                LCD_print(y_string, 8, 0, 7);
                LCD_print(z_string, 0, 1, 7);
            }
            break;
        case PROGRAM_STATE_DISPLAY_BME280_MEASUREMENTS:
            //    			LCD_print("P: T: H:");
            if (bme280_data_ready) {
                char pressure_string[8 + 1];
                char temperature_string[5 + 1];
                char humidity_string[7 + 1];
                snprintf(pressure_string, 8 + 1, "P %dkPa", pressure);
                snprintf(temperature_string, 5 + 1, "T %dC", temperature);
                snprintf(humidity_string, 7 + 1, "H %dRH", humidity);
                LCD_print(pressure_string, 0, 0, 8);
                LCD_print(temperature_string, 0, 1, 5);
                LCD_print(humidity_string, 6, 1, 7);
            }
            break;
        case PROGRAM_STATE_TYPE_TO_LCD:
            LCD_printLines(usart_msg_first_line, usart_msg_second_line);
            break;
        default:
        case PROGRAM_STATE_IDLE:
            LCD_printLines("---Idle State---", "Enter Command");
            break;
        }
        //    	snprintf(usart_msg, 16, "%d", program_state);
        //    	LCD_formatString(usart_msg, 1);
        //    	LCD_print(usart_msg);
        OS_delay(100U); /* block for 1 tick */
    }
}

uint32_t idle_thread_stack[40];

int main(void) {
	/* Initialize debug pins and delay peripheral */
    debug_init();
    delay_timer_init();

	/* Initialize peripherals */
    LCD_init(&lcd_config);
    USART_init(&usart_config);
    ADXL345_init(&adxl345_config);
    bme280_intf_init(&bme280_intf_config);

    /* Initialize RTOS */
    OS_init(idle_thread_stack, sizeof(idle_thread_stack));

    /* Start USART thread */
    OSThread_start(&USART_thread, 3U, /* priority */
                   &USART_threadHandler, USART_thread_stack, sizeof(USART_thread_stack));

    /* Start I2C thread */
    OSThread_start(&I2C_thread, 2U, /* priority */
                   &I2C_threadHandler, I2C_thread_stack, sizeof(I2C_thread_stack));

    /* Start LCD thread */
    OSThread_start(&LCD_thread, 1U, /* priority */
                   &LCD_threadHandler, LCD_thread_stack, sizeof(LCD_thread_stack));

    /* Transfer control to the RTOS to run the threads */
    OS_run();

    while (1);
}
