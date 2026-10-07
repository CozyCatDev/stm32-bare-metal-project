#include "lcd.h"

static lcd_config_t *lcd_config;

// default direction of data ports is output
static uint8_t direction = 1;

static uint8_t LCD_getEndOfString(char *string) {
    uint8_t length = 0;
    while (*string++) {
        length++;
    }
    return length;
}

void LCD_formatString(char *string, uint8_t length) {
    uint8_t end_of_string = LCD_getEndOfString(string);
    if (end_of_string <= LCD_COLS) {
        for (uint8_t i = end_of_string; i < length; i++) {
            string[i] = ' ';
        }
        string[length] = '\0';
    }
}

static void LCD_configDataPortsAsOutput(void) {
    for (uint8_t i = 0; i < 8; i++) {
        lcd_config->data_ports[i]->MODER &= ~(0x3UL << (lcd_config->data_pins[i] * 2U));
        lcd_config->data_ports[i]->MODER |= (0x1UL << (lcd_config->data_pins[i] * 2U));
        lcd_config->data_ports[i]->OTYPER &= ~(0x1UL << lcd_config->data_pins[i]);
    }
}

static void LCD_configDataPortsAsInput(void) {
    for (uint8_t i = 0; i < 8; i++) {
        lcd_config->data_ports[i]->MODER &= ~(0x3UL << (lcd_config->data_pins[i] * 2U));
    }
}

static void LCD_configGPIO(GPIO_TypeDef *gpio_port, uint8_t gpio_pin, uint8_t direction) {
    // enable clock signals to LCD ports
    if (gpio_port == GPIOA) {
        RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    } else if (gpio_port == GPIOB) {
        RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
    } else if (gpio_port == GPIOC) {
        RCC->IOPENR |= RCC_IOPENR_GPIOCEN;
    } else if (gpio_port == GPIOD) {
        RCC->IOPENR |= RCC_IOPENR_GPIODEN;
    } else if (gpio_port == GPIOF) {
        RCC->IOPENR |= RCC_IOPENR_GPIOFEN;
    }

    // configure as input ('00')
    gpio_port->MODER &= ~(0x3UL << (gpio_pin * 2U));
    if (direction) {
        // configure as output ('01')
        gpio_port->MODER |= (0x1UL << (gpio_pin * 2U));
        // configure as push-pull ('0')
        gpio_port->OTYPER &= ~(0x1UL << gpio_pin);
    }
    // configure as very high speed ('11')
    gpio_port->OSPEEDR |= (0x3UL << (gpio_pin * 2U));

    // configure as no pull-up/pull-down resistors ('00')
    gpio_port->PUPDR &= ~(0x3UL << (gpio_pin * 2U));
}

static void LCD_config(void) {
    // configure data pins DB7-DB0
    for (uint8_t i = 0; i < 8; i++) {
        LCD_configGPIO(lcd_config->data_ports[i], lcd_config->data_pins[i], lcd_config->direction);
    }
    // configure RS, R/W, E pins
    LCD_configGPIO(lcd_config->rs_port, lcd_config->rs_pin, lcd_config->direction);
    LCD_configGPIO(lcd_config->rw_port, lcd_config->rw_pin, lcd_config->direction);
    LCD_configGPIO(lcd_config->e_port, lcd_config->e_pin, lcd_config->direction);
}

static void LCD_read(uint8_t *buffer) {
    uint8_t bit;
    if (direction != 0) {
        direction = 0;
        LCD_configDataPortsAsInput();
    }
    // RW = 1 (read)
    lcd_config->rw_port->BSRR = (0x1UL << lcd_config->rw_pin);
    lcd_config->delay_us(LCD_ADDRESS_SETUP_TIME);
    // E = 1 (enable)
    lcd_config->e_port->BSRR = (0x1UL << lcd_config->e_pin);
    // read byte into buffer
    for (uint8_t i = 0; i < 8; i++) {
        bit = lcd_config->data_ports[i]->IDR & (0x1UL << lcd_config->data_pins[i]);
        if (bit) {
            *buffer |= (0x1UL << i);
        } else {
            *buffer &= ~(0x1UL << i);
        }
    }
    lcd_config->delay_us(LCD_DATA_HOLD_TIME);
    // E = 0 (disable)
    lcd_config->e_port->BRR = (0x1UL << lcd_config->e_pin);
    lcd_config->delay_us(LCD_DATA_HOLD_TIME);
}

static uint8_t LCD_readBusyFlagAndAddressCounter(uint8_t *buffer) {
    // RS = 0 (instruction register)
    lcd_config->rs_port->BRR = (0x1UL << lcd_config->rs_pin);
    LCD_read(buffer);
    return ((*buffer & 0x80) ? 1 : 0); // return busy flag
}

static void LCD_readData(uint8_t *buffer) {
    // RS = 1 (data register)
    lcd_config->rs_port->BSRR = (0x1UL << lcd_config->rs_pin);
    LCD_read(buffer);
}

static void LCD_write(uint8_t byte) {
    //	uint8_t buffer = 0;
    if (direction != 1) {
        direction = 1;
        LCD_configDataPortsAsOutput();
    }
    // RW = 0 (write)
    lcd_config->rw_port->BRR = (0x1UL << lcd_config->rw_pin);
    lcd_config->delay_us(LCD_ADDRESS_SETUP_TIME);
    // E = 1 (enable)
    lcd_config->e_port->BSRR = (0x1UL << lcd_config->e_pin);
    // send byte
    for (uint8_t i = 0; i < 8; i++) {
        // e.g. command = 0010 1110, data_pins[0] = 5, then bit 5 will be set (1 & 1 = 1)
        // e.g. command = 0010 1110, data_pins[1] = 4, then bit 5 will not be set (0 & 1 = 0)
        if (byte & (0x1UL << i)) {
            lcd_config->data_ports[i]->BSRR = (0x1UL << lcd_config->data_pins[i]);
        } else {
            lcd_config->data_ports[i]->BRR = (0x1UL << lcd_config->data_pins[i]);
        }
    }
    lcd_config->delay_us(LCD_DATA_HOLD_TIME);
    // E = 0 (disable)
    lcd_config->e_port->BRR = (0x1UL << lcd_config->e_pin);
    lcd_config->delay_us(LCD_DATA_HOLD_TIME);
    // reset DB7-DB0
    //	for(uint8_t i = 0; i < 8; i++){
    //		lcd_config->data_ports[i]->BRR = (0x1UL << lcd_config->data_pins[i]);
    //	}
    //	while(LCD_readBusyFlagAndAddressCounter(&buffer)){
    //
    //	}
}

static void LCD_sendCommand(uint8_t command) {
    // RS = 0 (instruction register)
    lcd_config->rs_port->BRR = (0x1UL << lcd_config->rs_pin);
    LCD_write(command);
}

static void LCD_sendData(uint8_t data) {
    // RS = 1 (data register)
    lcd_config->rs_port->BSRR = (0x1UL << lcd_config->rs_pin);
    LCD_write(data);
}

void LCD_clearDisplay(void) {
    LCD_sendCommand(0x01); // command = 0b0000 0001
}

void LCD_returnHome(void) {
    LCD_sendCommand(0x02); // command = 0b0000 0010
}

void LCD_entryModeSet(uint8_t id, uint8_t s) {
    LCD_sendCommand(0x04 | (id ? 0x02 : 0U) | (s ? 0x01 : 0U)); // command = 0b0000 01<I/D><S>
}

void LCD_controlDisplay(uint8_t d, uint8_t c, uint8_t b) {
    LCD_sendCommand(0x08 | (d ? 0x04 : 0U) | (c ? 0x02 : 0U) |
                    (b ? 0x01 : 0U)); // command = 0b0000 1<D><C><B>
}

void LCD_shiftCursorOrDisplay(uint8_t sc, uint8_t rl) {
    LCD_sendCommand(0x10 | (sc ? 0x08 : 0U) | (rl ? 0x04 : 0U)); // command = 0b0001 <S/C><R/L>xx
}

void LCD_functionSet(uint8_t dl, uint8_t n, uint8_t f) {
    LCD_sendCommand(0x20 | (dl ? 0x10 : 0U) | (n ? 0x08 : 0U) |
                    (f ? 0x04 : 0U)); // command = 0b001<DL> <N><F>xx
}

void LCD_setCGRAMAddress(uint8_t address) {
    LCD_sendCommand(0x40 | (address & 0x3F)); // command = 0b01<ACG[5:0]>
}

void LCD_setDDRAMAddress(uint8_t address) {
    LCD_sendCommand(0x80 | (address & 0x7F)); // command = 0b1<ADD[6:0]>
}

void LCD_init(lcd_config_t *config) {
    lcd_config = config;

    LCD_config();

    LCD_functionSet(lcd_config->dl, lcd_config->n, lcd_config->f);

    LCD_controlDisplay(lcd_config->d, lcd_config->c, lcd_config->b);

    LCD_entryModeSet(lcd_config->id, lcd_config->s);

    // clears DDRAM data and sets AC (and hence cursor) to 0
    LCD_clearDisplay();
}

void LCD_setCursor(uint8_t x, uint8_t y) {
    LCD_setDDRAMAddress(x + y * 0x40);
}

void LCD_print(char *text, uint8_t x, uint8_t y, uint8_t length) {
    LCD_formatString(text, length);
    LCD_setCursor(x, y);
    while (*text) {            // while current character is not NULL
        LCD_sendData(*text++); // send current character, then increment pointer
    }
}

void LCD_printLines(char *first_line, char *second_line) {
    LCD_print(first_line, 0, 0, LCD_COLS);
    LCD_print(second_line, 0, 1, LCD_COLS);
}

// void LCD_printf(size_t length, const char* format, ...){
//	char buffer[length];
//	va_list args;
//	va_start(args, format);
//	vsnprintf(buffer, length + 1, format, args);
//	va_end(args);
//	LCD_print(buffer);
// }
