#include "app_config.h"
#include "app.h"

#include "../BSP/board.h"
#include "../Drivers/dma/dma_driver.h"
#include "../Drivers/i2c/i2c_driver.h"
#include "../Drivers/spi/spi_driver.h"
#include "../Drivers/uart/uart_driver.h"
#include "../System/system_init.h"

#define APP_ASCII_CR        13U
#define APP_ASCII_LF        10U

static app_status_t app_status = {
    .state = APP_STATE_BOOT
};

static uint8_t app_spi_tx_buffer[APP_SPI_TX_LENGTH];
static volatile uint8_t app_spi_callback_pending = 0U;
static volatile uint32_t app_spi_callback_dma_errors = 0U;
static uint8_t app_spi_busy = 0U;
static uint8_t app_error_latched = 0U;
static uint32_t app_last_dma_error_count = 0U;
static uint32_t app_heartbeat_counter = 0U;
static uint32_t app_button_debounce = 0U;
static uint8_t app_last_button_state = 0U;

static void app_spi_callback(void);
static void app_service_uart(void);
static void app_service_button(void);
static void app_service_spi(void);
static void app_service_heartbeat(void);
static void app_handle_command(uint8_t command);
static void app_start_spi_transfer(void);
static void app_run_i2c_sample(void);
static void app_clear_error(void);
static void app_print_help(void);
static void app_print_status(void);
static void app_uart_send_text(const char *text);
static void app_uart_send_u32(uint32_t value);
static void app_uart_send_hex8(uint8_t value);
static void app_uart_send_newline(void);

void app_init(void)
{
    system_init_all();

    bsp_led_off(BSP_LED_GREEN);
    bsp_led_off(BSP_LED_ORANGE);
    bsp_led_off(BSP_LED_RED);
    bsp_led_off(BSP_LED_BLUE);

    uart_init(APP_UART_BAUDRATE);
    i2c_init();
    spi_init(app_spi_callback);

    app_last_dma_error_count = dma_get_error_count();
    app_status.dma_error_count = app_last_dma_error_count;
    app_status.state = APP_STATE_READY;

    bsp_led_on(BSP_LED_GREEN);

    app_uart_send_text("\r\nSTM32F4 Discovery application ready\r\n");
    app_print_help();
}

void app_run(void)
{
    while (1) {
        app_process();
    }
}

void app_process(void)
{
    app_status.loop_count++;

    app_service_uart();
    app_service_button();
    app_service_spi();
    app_service_heartbeat();
}

const app_status_t *app_get_status(void)
{
    return &app_status;
}

static void app_spi_callback(void)
{
    app_spi_callback_dma_errors = dma_get_error_count();
    app_spi_callback_pending = 1U;
}

static void app_service_uart(void)
{
    uint8_t byte;

    while (uart_read_byte(&byte) != 0U) {
        app_handle_command(byte);
    }
}

static void app_service_button(void)
{
    uint8_t button_state = bsp_button_read(BSP_BUTTON_USER);

    if (app_button_debounce > 0U) {
        app_button_debounce--;
    }

    if ((button_state != 0U) &&
        (app_last_button_state == 0U) &&
        (app_button_debounce == 0U)) {
        app_button_debounce = APP_BUTTON_DEBOUNCE_LOOPS;
        app_start_spi_transfer();
    }

    app_last_button_state = button_state;
}

static void app_service_spi(void)
{
    if (app_spi_callback_pending == 0U) {
        return;
    }

    if (app_spi_callback_dma_errors != app_last_dma_error_count) {
        app_last_dma_error_count = app_spi_callback_dma_errors;
        app_status.dma_error_count = app_last_dma_error_count;
        app_status.spi_error_count++;
        app_status.state = APP_STATE_ERROR;
        app_error_latched = 1U;
        app_spi_busy = 0U;
        app_spi_callback_pending = 0U;

        bsp_led_off(BSP_LED_ORANGE);
        bsp_led_on(BSP_LED_RED);
        app_uart_send_text("SPI DMA error\r\n");
        return;
    }

    if (spi_is_busy() == 0U) {
        app_status.spi_transfer_count++;
        app_status.state = (app_error_latched == 0U) ? APP_STATE_READY : APP_STATE_ERROR;
        app_spi_busy = 0U;
        app_spi_callback_pending = 0U;

        bsp_led_off(BSP_LED_ORANGE);
        bsp_led_toggle(BSP_LED_BLUE);
        app_uart_send_text("SPI transfer complete\r\n");
    }
}

static void app_service_heartbeat(void)
{
    app_heartbeat_counter++;

    if (app_heartbeat_counter >= APP_HEARTBEAT_DIVIDER) {
        app_heartbeat_counter = 0U;
        bsp_led_toggle(BSP_LED_GREEN);
    }
}

static void app_handle_command(uint8_t command)
{
    if ((command == APP_ASCII_CR) || (command == APP_ASCII_LF)) {
        return;
    }

    switch (command) {
    case '?':
    case 'h':
    case 'H':
        app_print_help();
        break;

    case 'p':
    case 'P':
        app_print_status();
        break;

    case 's':
    case 'S':
        app_start_spi_transfer();
        break;

    case 'i':
    case 'I':
        app_run_i2c_sample();
        break;

    case 'l':
    case 'L':
        bsp_led_toggle(BSP_LED_BLUE);
        app_uart_send_text("Blue LED toggled\r\n");
        break;

    case 'c':
    case 'C':
        app_clear_error();
        break;

    default:
        app_uart_send_text("Unknown command. Press ? for help\r\n");
        break;
    }
}

static void app_start_spi_transfer(void)
{
    if ((app_spi_busy != 0U) || (spi_is_busy() != 0U)) {
        app_uart_send_text("SPI busy\r\n");
        return;
    }

    for (uint8_t i = 0U; i < APP_SPI_TX_LENGTH; i++) {
        app_spi_tx_buffer[i] = (uint8_t)(0xA0U + i + app_status.spi_transfer_count);
    }

    app_last_dma_error_count = dma_get_error_count();
    app_status.dma_error_count = app_last_dma_error_count;
    app_spi_callback_pending = 0U;
    app_spi_callback_dma_errors = app_last_dma_error_count;
    app_spi_busy = 1U;
    app_status.state = APP_STATE_SPI_BUSY;

    bsp_led_on(BSP_LED_ORANGE);
    spi_transmit_dma(app_spi_tx_buffer, APP_SPI_TX_LENGTH);
    app_uart_send_text("SPI transfer started\r\n");
}

static void app_run_i2c_sample(void)
{
    uint8_t sample[APP_I2C_SAMPLE_LENGTH];

    if (i2c_read(APP_I2C_SAMPLE_ADDR, sample, APP_I2C_SAMPLE_LENGTH) == I2C_OK) {
        app_status.i2c_success_count++;

        for (uint8_t i = 0U; i < APP_I2C_SAMPLE_LENGTH; i++) {
            app_status.last_i2c_sample[i] = sample[i];
        }

        bsp_led_toggle(BSP_LED_BLUE);
        app_uart_send_text("I2C sample: 0x");
        app_uart_send_hex8(sample[0]);
        app_uart_send_text(" 0x");
        app_uart_send_hex8(sample[1]);
        app_uart_send_newline();
    } else {
        app_status.i2c_error_count++;
        app_status.state = APP_STATE_ERROR;
        app_error_latched = 1U;
        bsp_led_on(BSP_LED_RED);
        app_uart_send_text("I2C sample failed\r\n");
    }
}

static void app_clear_error(void)
{
    bsp_led_off(BSP_LED_RED);
    app_error_latched = 0U;

    if (app_spi_busy == 0U) {
        app_status.state = APP_STATE_READY;
    }

    app_uart_send_text("Application error state cleared\r\n");
}

static void app_print_help(void)
{
    app_uart_send_text("Commands:\r\n");
    app_uart_send_text("  ?/h: help\r\n");
    app_uart_send_text("  p: print status\r\n");
    app_uart_send_text("  s: start SPI DMA transfer\r\n");
    app_uart_send_text("  i: read ");
    app_uart_send_u32(APP_I2C_SAMPLE_LENGTH);
    app_uart_send_text(" bytes from I2C addr 0x");
    app_uart_send_hex8(APP_I2C_SAMPLE_ADDR);
    app_uart_send_newline();
    app_uart_send_text("  l: toggle blue LED\r\n");
    app_uart_send_text("  c: clear error LED/state\r\n");
}

static void app_print_status(void)
{
    app_uart_send_text("state=");
    app_uart_send_u32((uint32_t)app_status.state);
    app_uart_send_text(" loops=");
    app_uart_send_u32(app_status.loop_count);
    app_uart_send_text(" spi_ok=");
    app_uart_send_u32(app_status.spi_transfer_count);
    app_uart_send_text(" spi_err=");
    app_uart_send_u32(app_status.spi_error_count);
    app_uart_send_text(" dma_err=");
    app_uart_send_u32(app_status.dma_error_count);
    app_uart_send_text(" i2c_ok=");
    app_uart_send_u32(app_status.i2c_success_count);
    app_uart_send_text(" i2c_err=");
    app_uart_send_u32(app_status.i2c_error_count);
    app_uart_send_text(" last_i2c=0x");
    app_uart_send_hex8(app_status.last_i2c_sample[0]);
    app_uart_send_text(" 0x");
    app_uart_send_hex8(app_status.last_i2c_sample[1]);
    app_uart_send_newline();
}

static void app_uart_send_text(const char *text)
{
    uint16_t len = 0U;

    while ((len < 0xFFFFU) && (text[len] != '\0')) {
        len++;
    }

    if (len > 0U) {
        uart_send((const uint8_t *)text, len);
    }
}

static void app_uart_send_u32(uint32_t value)
{
    char buffer[10];
    uint8_t index = 0U;

    if (value == 0U) {
        app_uart_send_text("0");
        return;
    }

    while ((value > 0U) && (index < (uint8_t)sizeof(buffer))) {
        buffer[index] = (char)('0' + (value % 10U));
        value /= 10U;
        index++;
    }

    while (index > 0U) {
        index--;
        uart_send((const uint8_t *)&buffer[index], 1U);
    }
}

static void app_uart_send_hex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    char text[2];

    text[0] = hex[(value >> 4) & 0x0FU];
    text[1] = hex[value & 0x0FU];

    uart_send((const uint8_t *)text, 2U);
}

static void app_uart_send_newline(void)
{
    app_uart_send_text("\r\n");
}
