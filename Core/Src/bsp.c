#include "bsp.h"

void SysTick_Handler(void) {
    debug_write(2, 1);

    OS_tick();

    __disable_irq();
    OS_sched();
    __enable_irq();

    debug_write(2, 0);
}

void BSP_init() {}

void BSP_getLCDLines(char *msg, char *first_line, char *second_line) {
    uint8_t first_idx            = 0;
    uint8_t second_idx           = 0;
    uint8_t second_line_detected = 0;

    while (*msg != '\0') {
        if (*msg == '\n') {
            second_line_detected = 1;
        } else if (second_line_detected) {
            if (second_idx < LCD_COLS) {
                second_line[second_idx++] = *msg;
            }
        } else {
            if (first_idx < LCD_COLS) {
                first_line[first_idx++] = *msg;
            }
        }
        msg++;
    }
    for (uint8_t i = first_idx; i < LCD_COLS; i++) {
        first_line[i] = ' ';
    }
    for (uint8_t i = second_idx; i < LCD_COLS; i++) {
        second_line[i] = ' ';
    }
    first_line[LCD_COLS]  = '\0';
    second_line[LCD_COLS] = '\0';
}

void BSP_convertWordToMsg(uint16_t *rx_buf, uint8_t length, char *msg) {
    for (uint8_t i = 0; i < length; i++) {
        msg[i] = (char)rx_buf[i];
    }
    msg[length] = '\0';
}

void BSP_formatMsg(uint16_t *rx_buf, char *first_line, char *second_line, uint8_t length) {
    char msg[LCD_COLS * 2 + 2]; // +2 for null and newline characters
    if (length <= (LCD_COLS * 2 + 2)) {
        BSP_convertWordToMsg(rx_buf, length, msg);
        BSP_getLCDLines(msg, first_line, second_line);
    }
}

void BSP_copyArray(uint16_t *copied_array, uint16_t *old_array, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        copied_array[i] = old_array[i];
    }
    for (uint8_t i = length; i < BSP_MAX_COMMAND_LENGTH; i++) {
        copied_array[i] = 0;
    }
}

uint8_t BSP_calculateCommandLength(volatile uint16_t *usart_command) {
    uint8_t length = 1;
    if (usart_command[0] == BSP_START_COMMAND) {
        while (*usart_command != BSP_STOP_COMMAND && length <= BSP_MAX_COMMAND_LENGTH) {
            length++;
            usart_command++;
        }
        if (length > BSP_MAX_COMMAND_LENGTH) {
            length = 0;
        }
    }
    return length;
}

void BSP_restorePreviousProgramState(volatile uint16_t    *usart_command,
                                     program_state_t      *program_state,
                                     program_prev_state_t *program_prev_state) {
    *program_state = program_prev_state->prev_state_before_transition;
    BSP_copyArray(usart_command, program_prev_state->prev_command,
                  program_prev_state->prev_command_length);
}

void BSP_saveProgramState(volatile uint16_t    *usart_command,
                          program_state_t      *program_state,
                          program_prev_state_t *program_prev_state) {
    program_prev_state->prev_state_before_transition = *program_state;
    program_prev_state->prev_command_length          = BSP_calculateCommandLength(usart_command);
    BSP_copyArray(program_prev_state->prev_command, usart_command,
                  program_prev_state->prev_command_length);
}

program_state_t BSP_getProgramState(volatile uint16_t *usart_command) {
    program_state_t program_state;
    switch (usart_command[1]) {
    case BSP_DISPLAY_ADXL345_MEASUREMENTS_COMMAND:
        program_state = PROGRAM_STATE_DISPLAY_ADXL345_MEASUREMENTS;
        break;
    case BSP_DISPLAY_BME280_MEASUREMENTS_COMMAND:
        program_state = PROGRAM_STATE_DISPLAY_BME280_MEASUREMENTS;
        break;
    case BSP_TRANSMIT_DATA_COMMAND:
        program_state = PROGRAM_STATE_TRANSMIT_DATA;
        break;
    case BSP_TYPE_TO_LCD_COMMAND:
        program_state = PROGRAM_STATE_TYPE_TO_LCD;
        break;
    default:
    case BSP_IDLE_COMMAND:
        program_state = PROGRAM_STATE_IDLE;
        break;
    }
    return program_state;
}

void BSP_updateProgramState(volatile uint16_t *usart_command, program_state_t *program_state) {
    *program_state = BSP_getProgramState(usart_command);
}

void BSP_delay(uint32_t num_cycles) {
    volatile uint32_t        i;
    static volatile uint32_t test[3];
    test[0] = 1;
    for (i = 0; i < num_cycles; i++) {
    }
}

void OS_onStartup(void) {
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);

    /* set the SysTick interrupt priority (highest) */
    NVIC_SetPriority(SysTick_IRQn, 0U);
}

void OS_onIdle(void) {
    // #ifdef NDBEBUG
    __WFI(); /* stop the CPU and Wait for Interrupt */
    // #endif
}

//............................................................................
void Q_onAssert(char const *module, int id) {
    /* TBD: damage control */
    (void)module; /* avoid the "unused parameter" compiler warning */
    (void)id;     /* avoid the "unused parameter" compiler warning */
    NVIC_SystemReset();
}

void assert_failed(char const *module, int id) {
    Q_onAssert(module, id);
}
