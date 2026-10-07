#ifndef BSP_H
#define BSP_H

#include "debug.h"
#include "lcd.h"
#include "miros.h"
#include "stm32c031xx.h"
#include <stdint.h>

#define BSP_TICKS_PER_SEC (1000U)

#define BSP_MIN_COMMAND_LENGTH (3)
#define BSP_MAX_COMMAND_LENGTH (40)

#define BSP_START_COMMAND                        ((uint8_t)0xFD)
#define BSP_STOP_COMMAND                         ((uint8_t)0xFE)
#define BSP_IDLE_COMMAND                         ((uint8_t)0x01)
#define BSP_DISPLAY_ADXL345_MEASUREMENTS_COMMAND ((uint8_t)0x02)
#define BSP_DISPLAY_BME280_MEASUREMENTS_COMMAND  ((uint8_t)0x03)
#define BSP_TRANSMIT_DATA_COMMAND                ((uint8_t)0x04)
#define BSP_TYPE_TO_LCD_COMMAND                  ((uint8_t)0x05)

typedef enum {
    PROGRAM_STATE_IDLE,
    PROGRAM_STATE_DISPLAY_ADXL345_MEASUREMENTS,
    PROGRAM_STATE_DISPLAY_BME280_MEASUREMENTS,
    PROGRAM_STATE_TRANSMIT_DATA,
    PROGRAM_STATE_TYPE_TO_LCD
} program_state_t;

typedef struct {
    program_state_t prev_state_before_transition;
    uint16_t        prev_command[BSP_MAX_COMMAND_LENGTH];
    uint8_t         prev_command_length;
} program_prev_state_t;

void    BSP_getLCDLines(char *msg, char *first_line, char *second_line);
void    BSP_convertWordToMsg(uint16_t *rx_buf, uint8_t length, char *msg);
void    BSP_formatMsg(uint16_t *rx_buf, char *first_line, char *second_line, uint8_t length);
void    BSP_copyArray(uint16_t *copied_array, uint16_t *old_array, uint8_t length);
uint8_t BSP_calculateCommandLength(volatile uint16_t *usart_command);
void    BSP_restorePreviousProgramState(volatile uint16_t    *usart_command,
                                        program_state_t      *program_state,
                                        program_prev_state_t *program_prev_state);
void    BSP_saveProgramState(volatile uint16_t    *usart_command,
                             program_state_t      *program_state,
                             program_prev_state_t *program_prev_state);
program_state_t BSP_getProgramState(volatile uint16_t *usart_command);
void BSP_updateProgramState(volatile uint16_t *usart_command, program_state_t *program_state);
void BSP_init();
void BSP_delay();

#endif
