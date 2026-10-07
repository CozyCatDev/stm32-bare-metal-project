/**
 * @file       lcd.h
 * @brief      LCD driver for HD44780 display driver by Hitachi.
 */

/**
 * @defgroup LCD LCD
 * @brief LCD driver for HD44780 display driver by Hitachi.
 *
 *
 *
 */

#ifndef LCD_H
#define LCD_H

#include "stm32c031xx.h"
//#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @ingroup LCD
 * @defgroup LCD_Macros Macros
 * @{
 */
#define LCD_ROWS (2) /**< Number of LCD rows. */
#define LCD_COLS (16) /**< Number of LCD columns. */

#define LCD_ADDRESS_SETUP_TIME (500UL) /**< LCD address setup time in microseconds. */
#define LCD_DATA_HOLD_TIME     (500UL) /**< LCD data hold time in microseconds. */
/** @} */

/**
 * @ingroup LCD
 * @defgroup LCD_Init Initialization
 */

/**
 * @ingroup LCD_Init
 * @brief LCD configuration object.
 */
typedef struct {
    /**
     * @brief User-defined microsecond delay function.
     *
     * @param[in] us Delay duration in microseconds.
     */
    void (*delay_us)(uint32_t us);

    /* GPIO port mapping */
    GPIO_TypeDef *data_ports[8]; /**< Data ports. */
    GPIO_TypeDef *rs_port; /**< Register select port. */
    GPIO_TypeDef *rw_port; /**< Read/write port. */
    GPIO_TypeDef *e_port; /**< Enable signal port. */

    /* GPIO pin mapping */
    uint8_t data_pins[8]; /**< Data pins. */
    uint8_t rs_pin; /**< Register select pin. */
    uint8_t rw_pin; /**< Read/write pin. */
    uint8_t e_pin; /**< Enable signal pin. */

    /* LCD configuration */
    uint8_t direction; /**< '0' = Data pins configured as input, '1' = Data pins configured as output. */
    uint8_t id; /**< '0' = Increment Address Counter (AC) on data read/write, '1' = Decrement Address Counter (AC) on data read/write. */
    uint8_t s; /**< '0' = No display shift on data read/write, '1' = Display shift on data read/write. */
    uint8_t d; /**< '0' = Display off, '1' = Display on. */
    uint8_t c; /**< '0' = Cursor off, '1' = Cursor on. */
    uint8_t b; /**< '0' = Cursor character blinking off, '1' = Cursor character blinking on. */
    uint8_t dl; /**< '0' = 4-bit data bus (DB3-DB0 only), '1' = 8-bit data bus (DB7-DB0). */
    uint8_t n; /**< '0' = 1-line display, '1' = 2-line display. */
    uint8_t f; /**< '0' = 5 x 8 fonts, '1' = 5 x 10 fonts. */
} lcd_config_t;

/**
 * @ingroup LCD
 * @defgroup LCD_Format Formatting Functions
 * @brief Functions used for string formatting before printing to the LCD.
 */

/**
 * @ingroup LCD_Format
 * @brief Formats a given string by adding empty spaces up to `length` and a null character, '\0' at the end.
 *
 * This function is used instead of repeatedly clearing the LCD with LCD_clearDisplay() which causes printed letters to appear dim.
 *
 * @param[out] string String to format.
 * @param[in] length Length of string. Empty spaces are added until index, `length - 1`. '\0' added at index, `length`.
 *
 * @code
 * char test_string[LCD_COLS + 1] = "test";
 * LCD_formatString(test_string, LCD_COLS); // formats as "test" followed by 11 spaces and '\0' at test_string[LCD_COLS].
 * @endcode
 */
void LCD_formatString(char *string, uint8_t length);

/**
 * @ingroup LCD
 * @defgroup LCD_Control Control Functions
 * @brief Functions for controlling the LCD.
 */

/**
 * @ingroup LCD_Control
 * @brief Clears LCD display.
 *
 * This function erases DDRAM contents.
 *
 * @warning Use this function conservatively as clearing and printing to the LCD in quick successions can cause letters to appear dim. This is due to the "clear display" command requiring the longest execution time.
 */
void LCD_clearDisplay(void);

/**
 * @ingroup LCD_Control
 * @brief Returns cursor to DDRAM address 0 and returns display to original position. DDRAM contents remain unchanged.
 */
void LCD_returnHome(void);

/**
 * @ingroup LCD_Control
 * @brief Sets cursor increment/decrement and display shift with every data read/write.
 *
 * @param[in] id '0' = Increment Address Counter (AC) on data read/write, '1' = Decrement Address Counter (AC) on data read/write.
 * @param[in] id '0' = No display shift on data read/write, '1' = Display shift on data read/write.
 */
void LCD_entryModeSet(uint8_t id, uint8_t s);

/**
 * @ingroup LCD_Control
 * @brief Turns display on/off, cursor on/off, cursor character blinking on/off.
 *
 * @param[in] d '0' = Display off, '1' = Display on.
 * @param[in] c '0' = Cursor off, '1' = Cursor on.
 * @param[in] b '0' = Cursor character blinking off, '1' = Cursor character blinking on.
 */
void LCD_controlDisplay(uint8_t d, uint8_t c, uint8_t b);

/**
 * @ingroup LCD_Control
 * @brief Shifts either cursor/display left/right without changing DDRAM contents.
 *
 * @note This is different from LCD_entryModeSet() as cursor/display shifts are only performed once instead of performing on every data read/write.
 *
 * @param[in] sc '0' = Move cursor, '1' = Display shift.
 * @param[in] rl '0' = Shift cursor/display left, '1' = Shift cursor/display right.
 */
void LCD_shiftCursorOrDisplay(uint8_t sc, uint8_t rl);

/**
 * @ingroup LCD_Control
 * @brief Configures data bus length, number of LCD lines and type of LCD font.
 *
 * @param[in] dl '0' = 4-bit data bus (DB3-DB0 only), '1' = 8-bit data bus (DB7-DB0).
 * @param[in] n '0' = 1-line display, '1' = 2-line display.
 * @param[in] f '0' = 5 x 8 fonts, '1' = 5 x 10 fonts.
 */
void LCD_functionSet(uint8_t dl, uint8_t n, uint8_t f);

/**
 * @ingroup LCD_Control
 * @brief Sets CGRAM address to read/write from/to.
 *
 * CGRAM is character-generator RAM and its contents are user-defined so that custom characters can be saved and printed to the LCD.
 *
 * @param[in] address 8-bit CGRAM address to read/write from/to.
 */
void LCD_setCGRAMAddress(uint8_t address);

/**
 * @ingroup LCD_Control
 * @brief Sets DDRAM address to read/write from/to.
 *
 * DDRAM is display data RAM and its contents are the current characters on the LCD screen.
 *
 * @param[in] address DDRAM address to read/write from/to.
 */
void LCD_setDDRAMAddress(uint8_t address);

/**
 * @ingroup LCD
 * @defgroup LCD_Init Initialization
 * @brief Function for initializing LCD.
 */

/**
 * @ingroup LCD_Init
 * @brief Initializes LCD according to configuration object.
 *
 * Performs the following actions in this order:
 * 1. Configures GPIO ports and pins connected to LCD.
 * 2. Configure data bus length, number of LCD lines and type of LCD font using LCD_functionSet().
 * 3. Turn display on/off, cursor on/off, cursor character blinking on/off using LCD_controlDisplay().
 * 4. Sets cursor increment/decrement and display shift with every data read/write using LCD_entryModeSet().
 * 5. Clears LCD display using LCD_clearDisplay().
 *
 * @param[in] config Pointer to LCD configuration object.
 */
void LCD_init(lcd_config_t *config);

/**
 * @ingroup LCD_Control
 * @brief Sets cursor position of LCD.
 *
 * @param[in] x X position of character, must be between 0 to LCD_COLS - 1.
 * @param[in] y Y position of character, must be between 0 to LCD_ROWS - 1.
 */
void LCD_setCursor(uint8_t x, uint8_t y);

/**
 * @ingroup LCD_Control
 * @brief Prints a string to the LCD at a specified x and y position.
 *
 * @note This function calls LCD_formatString() before printing the string to the LCD.
 *
 * @param[in] text String to print to LCD.
 * @param[in] x X position of character, must be between 0 to LCD_COLS - 1.
 * @param[in] y Y position of character, must be between 0 to LCD_ROWS - 1.
 * @param[in] length Length of text.
 */
void LCD_print(char *text, uint8_t x, uint8_t y, uint8_t length);

/**
 * @ingroup LCD_Control
 * @brief Prints 2 lines to the LCD.
 *
 * @note This function calls LCD_formatString() on each line before printing them to the LCD. Empty spaces are added up to `LCD_COLS - 1`.
 *
 * @param[in] first_line First line to be printed to LCD.
 * @param[in] second_line Second line to be printed to LCD.
 */
void LCD_printLines(char *first_line, char *second_line);
// void LCD_printf(size_t length, const char* format, ...);

#endif
