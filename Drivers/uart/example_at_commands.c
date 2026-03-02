/* example_at_commands.c - UART Usage Example for AT Command Modules */

#include "uart_driver.h"
#include <string.h>

/*=============================================================================
 * EXAMPLE: Communicating with GSM module using AT commands
 *===========================================================================*/

#define AT_RESPONSE_TIMEOUT     5000    /* 5 seconds */

/* Simple delay function (replace with proper timer in production) */
static void delay_ms(uint32_t ms)
{
    for (volatile uint32_t i = 0; i < ms * 1000; i++);
}

/**
 * @brief Send AT command and wait for response
 * 
 * Typical AT command flow:
 * 1. Send command: "AT+CPIN?\r\n"
 * 2. GSM module processes command
 * 3. Module sends response: "+CPIN: READY\r\n\r\nOK\r\n"
 * 4. Response arrives byte-by-byte via UART RX interrupt
 * 5. Each byte is stored in ring buffer by ISR
 * 6. Application reads bytes from buffer and checks for "OK"
 * 
 * @param command: AT command string (e.g., "AT\r\n")
 * @param response: Buffer to store response
 * @param max_len: Maximum response length
 * @return 1 if OK received, 0 if timeout
 */
uint8_t send_at_command(const char *command, char *response, uint16_t max_len)
{
    uint16_t idx = 0;
    uint32_t timeout = AT_RESPONSE_TIMEOUT;
    
    /* Clear any old data in RX buffer */
    uart_clear_rx();
    
    /* Send AT command */
    uart_send((uint8_t *)command, strlen(command));
    
    /* Wait for response */
    while (timeout > 0) {
        /* Check if data available */
        if (uart_available() > 0) {
            uint8_t byte;
            if (uart_read_byte(&byte)) {
                /* Store byte in response buffer */
                if (idx < max_len - 1) {
                    response[idx++] = byte;
                    response[idx] = '\0';  /* Null terminate */
                    
                    /* Check if "OK" received (end of response) */
                    if (strstr(response, "OK") != NULL) {
                        return 1;  /* Success */
                    }
                    
                    /* Check if "ERROR" received */
                    if (strstr(response, "ERROR") != NULL) {
                        return 0;  /* Command failed */
                    }
                }
            }
        }
        
        delay_ms(1);
        timeout--;
    }
    
    return 0;  /* Timeout */
}

/**
 * @brief Initialize GSM module
 */
void gsm_init(void)
{
    char response[128];
    
    /* Initialize UART at 115200 baud */
    uart_init(115200);
    
    delay_ms(1000);  /* Wait for module to boot */
    
    /* Test communication */
    if (send_at_command("AT\r\n", response, sizeof(response))) {
        /* Module responded OK */
    }
    
    /* Check SIM card status */
    send_at_command("AT+CPIN?\r\n", response, sizeof(response));
    
    /* Set SMS text mode */
    send_at_command("AT+CMGF=1\r\n", response, sizeof(response));
}

/**
 * @brief Send SMS
 */
void gsm_send_sms(const char *phone, const char *message)
{
    char cmd[64];
    char response[128];
    
    /* Set recipient phone number */
    sprintf(cmd, "AT+CMGS=\"%s\"\r\n", phone);
    uart_send((uint8_t *)cmd, strlen(cmd));
    
    delay_ms(500);  /* Wait for ">" prompt */
    
    /* Send message text */
    uart_send((uint8_t *)message, strlen(message));
    
    /* Send Ctrl+Z to finish (0x1A) */
    uint8_t ctrl_z = 0x1A;
    uart_send(&ctrl_z, 1);
    
    /* Wait for response */
    send_at_command("", response, sizeof(response));
}

/**
 * @brief Read GPS coordinates (example for GPS module)
 */
void gps_read_position(void)
{
    char response[256];
    
    /* Request GPS data */
    if (send_at_command("AT+CGPSINFO\r\n", response, sizeof(response))) {
        /* Parse response to extract latitude/longitude */
        /* Example response: +CGPSINFO: 3113.343286,N,12121.234064,E,... */
        
        /* Simple parsing (production code should be more robust) */
        char *lat_start = strstr(response, "+CGPSINFO: ");
        if (lat_start != NULL) {
            /* Extract and process GPS data */
        }
    }
}

/**
 * @brief Main application example
 */
void example_main(void)
{
    gsm_init();
    
    while (1) {
        /* Check for incoming SMS */
        char response[256];
        if (send_at_command("AT+CMGL=\"ALL\"\r\n", response, sizeof(response))) {
            /* Process received SMS */
        }
        
        delay_ms(5000);  /* Check every 5 seconds */
    }
}

/*=============================================================================
 * DATA FLOW VISUALIZATION
 *===========================================================================*/

/*
 * WITHOUT INTERRUPT (Polling - BAD for AT commands):
 * 
 * App: Send "AT+CPIN?\r\n"
 * App: while (!data_ready) { check_uart(); }  ← CPU stuck here!
 * 
 * Problem: If app is doing something else, response bytes are lost!
 */

/*
 * WITH INTERRUPT + RING BUFFER (GOOD):
 * 
 * App: Send "AT+CPIN?\r\n"
 * App: Do other work...
 * 
 * [Background] UART RX interrupt fires for each byte:
 *   ISR: byte '+' → ring_buffer_put()
 *   ISR: byte 'C' → ring_buffer_put()
 *   ISR: byte 'P' → ring_buffer_put()
 *   ... (all bytes safely stored)
 * 
 * App: When ready, read from buffer:
 *   while (uart_available()) {
 *       uart_read_byte(&byte);
 *       process(byte);
 *   }
 * 
 * Result: No data loss, CPU can multitask!
 */

/*=============================================================================
 * TYPICAL AT COMMAND RESPONSES
 *===========================================================================*/

/* Example 1: Simple command
 * Send: "AT\r\n"
 * Recv: "AT\r\n\r\nOK\r\n"
 */

/* Example 2: Query command
 * Send: "AT+CPIN?\r\n"
 * Recv: "AT+CPIN?\r\n\r\n+CPIN: READY\r\n\r\nOK\r\n"
 */

/* Example 3: Multi-line response
 * Send: "AT+CMGL=\"ALL\"\r\n"
 * Recv: "+CMGL: 1,"REC UNREAD","+1234567890",...\r\n"
 *       "Hello World\r\n"
 *       "+CMGL: 2,"REC READ","+9876543210",...\r\n"
 *       "Test message\r\n"
 *       "\r\nOK\r\n"
 */
