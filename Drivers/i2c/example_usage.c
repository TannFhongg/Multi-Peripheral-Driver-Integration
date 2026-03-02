/* example_usage.c - I2C Driver Usage Examples */

#include "i2c_driver.h"

/*=============================================================================
 * EXAMPLE 1: Write data to I2C slave
 *===========================================================================*/
void example_write_to_sensor(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03};
    uint8_t status;
    
    status = i2c_write(0x68, data, 3);  // Write 3 bytes to slave 0x68
    
    if (status == I2C_OK) {
        // Success
    } else {
        // Error handling
    }
}

/*=============================================================================
 * EXAMPLE 2: Read data from I2C slave
 *===========================================================================*/
void example_read_from_sensor(void)
{
    uint8_t buffer[6];
    uint8_t status;
    
    status = i2c_read(0x68, buffer, 6);  // Read 6 bytes from slave 0x68
    
    if (status == I2C_OK) {
        // Process received data in buffer[]
    }
}

/*=============================================================================
 * EXAMPLE 3: Write to specific register (common pattern)
 *===========================================================================*/
void example_write_register(void)
{
    uint8_t data[2];
    
    // Write value 0x80 to register 0x1A
    data[0] = 0x1A;  // Register address
    data[1] = 0x80;  // Register value
    
    i2c_write(0x68, data, 2);
}

/*=============================================================================
 * EXAMPLE 4: Read from specific register (common pattern)
 *===========================================================================*/
void example_read_register(void)
{
    uint8_t reg_addr = 0x3B;
    uint8_t buffer[2];
    
    // Step 1: Write register address
    i2c_write(0x68, &reg_addr, 1);
    
    // Step 2: Read data from that register
    i2c_read(0x68, buffer, 2);
    
    // Now buffer[] contains data from register 0x3B
}

/*=============================================================================
 * EXAMPLE 5: Complete initialization and usage
 *===========================================================================*/
void example_complete_flow(void)
{
    uint8_t sensor_id;
    uint8_t config_data[2];
    
    // Initialize I2C peripheral
    i2c_init();
    
    // Read WHO_AM_I register (0x75) from MPU6050
    uint8_t who_am_i_reg = 0x75;
    i2c_write(0x68, &who_am_i_reg, 1);
    i2c_read(0x68, &sensor_id, 1);
    
    // Configure sensor: write to PWR_MGMT_1 register (0x6B)
    config_data[0] = 0x6B;  // Register address
    config_data[1] = 0x00;  // Wake up sensor
    i2c_write(0x68, config_data, 2);
    
    // Read accelerometer data from register 0x3B (6 bytes)
    uint8_t accel_reg = 0x3B;
    uint8_t accel_data[6];
    i2c_write(0x68, &accel_reg, 1);
    i2c_read(0x68, accel_data, 6);
    
    // Process accelerometer data
    int16_t accel_x = (accel_data[0] << 8) | accel_data[1];
    int16_t accel_y = (accel_data[2] << 8) | accel_data[3];
    int16_t accel_z = (accel_data[4] << 8) | accel_data[5];
}
