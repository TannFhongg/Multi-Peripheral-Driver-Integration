/* i2c_driver.c - Simple I2C Master Driver (Register-Level, Polling Mode) */

#include "i2c_driver.h"

/*=============================================================================
 * HARDWARE REGISTER DEFINITIONS (STM32-like I2C peripheral)
 *===========================================================================*/
#define I2C1_BASE           0x40005400UL

#define I2C_CR1             (*(volatile uint32_t *)(I2C1_BASE + 0x00))
#define I2C_CR2             (*(volatile uint32_t *)(I2C1_BASE + 0x04))
#define I2C_DR              (*(volatile uint32_t *)(I2C1_BASE + 0x10))
#define I2C_SR1             (*(volatile uint32_t *)(I2C1_BASE + 0x14))
#define I2C_SR2             (*(volatile uint32_t *)(I2C1_BASE + 0x18))
#define I2C_CCR             (*(volatile uint32_t *)(I2C1_BASE + 0x1C))
#define I2C_TRISE           (*(volatile uint32_t *)(I2C1_BASE + 0x20))

/* CR1 bits */
#define CR1_PE              (1 << 0)    /* Peripheral Enable */
#define CR1_START           (1 << 8)    /* Start Generation */
#define CR1_STOP            (1 << 9)    /* Stop Generation */
#define CR1_ACK             (1 << 10)   /* Acknowledge Enable */

/* SR1 bits */
#define SR1_SB              (1 << 0)    /* Start Bit (Master mode) */
#define SR1_ADDR            (1 << 1)    /* Address sent/matched */
#define SR1_BTF             (1 << 2)    /* Byte Transfer Finished */
#define SR1_RXNE            (1 << 6)    /* RX Not Empty */
#define SR1_TXE             (1 << 7)    /* TX Empty */
#define SR1_AF              (1 << 10)   /* Acknowledge Failure */

/* Simple timeout counter */
#define TIMEOUT             50000

/*=============================================================================
 * PRIVATE HELPER FUNCTIONS
 *===========================================================================*/

/* Wait for flag with timeout */
static uint8_t wait_flag(volatile uint32_t *reg, uint32_t flag)
{
    uint32_t timeout = TIMEOUT;
    while (!(*reg & flag)) {
        if (--timeout == 0) return I2C_ERROR;
    }
    return I2C_OK;
}

/* Clear ADDR flag by reading SR1 then SR2 */
static void clear_addr_flag(void)
{
    (void)I2C_SR1;
    (void)I2C_SR2;
}

/*=============================================================================
 * PUBLIC API IMPLEMENTATION
 *===========================================================================*/

/**
 * @brief Initialize I2C peripheral
 * 
 * Configures I2C1 for 100kHz standard mode
 * Assumes APB1 clock = 42MHz
 */
void i2c_init(void)
{
    /* Disable I2C to configure */
    I2C_CR1 &= ~CR1_PE;
    
    /* Configure clock: CCR = FPCLK / (2 * FSCL) */
    /* For 100kHz: CCR = 42MHz / (2 * 100kHz) = 210 */
    I2C_CCR = 210;
    
    /* Configure rise time: TRISE = (Trise / TPCLK) + 1 */
    /* For standard mode (1000ns): (1000ns / 23.8ns) + 1 = 43 */
    I2C_TRISE = 43;
    
    /* Enable peripheral */
    I2C_CR1 |= CR1_PE;
}

/**
 * @brief I2C Master Write (blocking)
 * 
 * Protocol sequence:
 * START → ADDR(W) → DATA[0] → DATA[1] → ... → DATA[n-1] → STOP
 * 
 * @param slave_addr: 7-bit slave address (not shifted)
 * @param data: pointer to data buffer
 * @param len: number of bytes to send
 * @return I2C_OK or I2C_ERROR
 */
uint8_t i2c_write(uint8_t slave_addr, uint8_t *data, uint8_t len)
{
    uint8_t i;
    
    /* 1. Generate START condition */
    I2C_CR1 |= CR1_START;
    if (wait_flag(&I2C_SR1, SR1_SB) != I2C_OK) return I2C_ERROR;
    
    /* 2. Send slave address + Write bit (0) */
    I2C_DR = (slave_addr << 1) | 0;
    if (wait_flag(&I2C_SR1, SR1_ADDR) != I2C_OK) return I2C_ERROR;
    clear_addr_flag();
    
    /* 3. Send data bytes */
    for (i = 0; i < len; i++) {
        /* Wait until TX register empty */
        if (wait_flag(&I2C_SR1, SR1_TXE) != I2C_OK) return I2C_ERROR;
        I2C_DR = data[i];
    }
    
    /* 4. Wait for byte transfer finished */
    if (wait_flag(&I2C_SR1, SR1_BTF) != I2C_OK) return I2C_ERROR;
    
    /* 5. Generate STOP condition */
    I2C_CR1 |= CR1_STOP;
    
    return I2C_OK;
}

/**
 * @brief I2C Master Read (blocking)
 * 
 * Protocol sequence:
 * START → ADDR(R) → DATA[0] ← DATA[1] ← ... ← DATA[n-1] → STOP
 * 
 * Note: Master sends NACK before last byte to signal end of transfer
 * 
 * @param slave_addr: 7-bit slave address (not shifted)
 * @param buf: pointer to receive buffer
 * @param len: number of bytes to receive
 * @return I2C_OK or I2C_ERROR
 */
uint8_t i2c_read(uint8_t slave_addr, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    
    /* 1. Generate START condition */
    I2C_CR1 |= CR1_START;
    if (wait_flag(&I2C_SR1, SR1_SB) != I2C_OK) return I2C_ERROR;
    
    /* 2. Send slave address + Read bit (1) */
    I2C_DR = (slave_addr << 1) | 1;
    if (wait_flag(&I2C_SR1, SR1_ADDR) != I2C_OK) return I2C_ERROR;
    
    /* Special case: single byte read */
    if (len == 1) {
        /* Disable ACK before clearing ADDR flag */
        I2C_CR1 &= ~CR1_ACK;
        clear_addr_flag();
        /* Generate STOP after clearing ADDR */
        I2C_CR1 |= CR1_STOP;
        /* Wait and read data */
        if (wait_flag(&I2C_SR1, SR1_RXNE) != I2C_OK) return I2C_ERROR;
        buf[0] = I2C_DR;
        return I2C_OK;
    }
    
    /* Multi-byte read */
    I2C_CR1 |= CR1_ACK;  /* Enable ACK */
    clear_addr_flag();
    
    /* 3. Receive data bytes */
    for (i = 0; i < len; i++) {
        /* Before last byte: disable ACK and generate STOP */
        if (i == len - 1) {
            I2C_CR1 &= ~CR1_ACK;
            I2C_CR1 |= CR1_STOP;
        }
        
        /* Wait for data received */
        if (wait_flag(&I2C_SR1, SR1_RXNE) != I2C_OK) return I2C_ERROR;
        buf[i] = I2C_DR;
    }
    
    return I2C_OK;
}
