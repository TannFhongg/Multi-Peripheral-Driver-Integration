# Drivers and Supporting Modules

## Driver Matrix

| Module | Files | Purpose |
| --- | --- | --- |
| UART | `Drivers/uart/uart_driver.c`, `Drivers/uart/uart_driver.h` | `USART1` initialization, blocking transmit, RX interrupt, ring-buffer-backed receive |
| I2C | `Drivers/i2c/i2c_driver.c`, `Drivers/i2c/i2c_driver.h` | Polling-based `I2C1` master transactions |
| SPI | `Drivers/spi/spi_driver.c`, `Drivers/spi/spi_driver.h` | `SPI1` TX-only transfers using DMA |
| DMA | `Drivers/dma/dma_driver.c`, `Drivers/dma/dma_driver.h` | `DMA2 Stream3 Channel3` support for `SPI1_TX` |
| Ring buffer utility | `Utils/ring_buffer.c`, `Utils/ring_buffer.h` | Byte FIFO used by UART RX |
| BSP | `BSP/stm32f4_discovery.c`, `BSP/stm32f4_discovery.h`, `BSP/board.h` | Board-level clock, GPIO, NVIC, LED, and button support |
| System facade | `System/system_init.c`, `System/system_init.h` | Thin abstraction layer over BSP initialization |

## UART Driver

### Purpose

Provides a simple console transport on `USART1`:

- Blocking transmit for text output.
- Interrupt-driven receive.
- Ring-buffer decoupling between the ISR and the application command parser.

### Public interface

| API | Notes |
| --- | --- |
| `uart_init(uint32_t baudrate)` | Configures `USART1` and the RX ring buffer |
| `uart_send(const uint8_t *data, uint16_t len)` | Sends bytes with polling on `TXE` |
| `uart_read_byte(uint8_t *byte)` | Reads one buffered byte if available |
| `uart_available(void)` | Returns buffered RX byte count |
| `uart_clear_rx(void)` | Flushes the ring buffer |

### Dependencies

- BSP must enable `USART1` clock, `PA9/PA10`, and `USART1_IRQn`.
- Uses `Utils/ring_buffer.*`.

### Usage notes

- `uart_send()` is blocking and has no timeout.
- RX error counters exist internally in the driver but are not exposed through the public API.
- RX overflow can occur if the ring buffer fills faster than the application drains it.

## I2C Driver

### Purpose

Implements a minimal `I2C1` master driver for polling-based read and write transactions.

### Public interface

| API | Notes |
| --- | --- |
| `i2c_init(void)` | Programs `CCR`, `TRISE`, and enables `I2C1` |
| `i2c_write(uint8_t slave_addr, uint8_t *data, uint8_t len)` | Sends a 7-bit address plus payload |
| `i2c_read(uint8_t slave_addr, uint8_t *buf, uint8_t len)` | Reads one or more bytes from a 7-bit address |

### Dependencies

- BSP must enable `I2C1` and configure `PB6/PB7`.
- Assumes the APB1 clock is consistent with the hardcoded timing values.

### Usage notes

- This is a blocking implementation with loop-count timeouts.
- The address parameter is a 7-bit device address, not a pre-shifted wire value.
- `I2C_CR2` frequency setup is currently missing.
- NACK and recovery handling are incomplete for robust field use.

## SPI Driver

### Purpose

Implements the `SPI1` transmit path and delegates the actual data movement to DMA.

### Public interface

| API | Notes |
| --- | --- |
| `spi_init(spi_callback_t callback)` | Configures `SPI1`, initializes DMA, and stores a completion callback |
| `spi_transmit_dma(uint8_t *data, uint16_t len)` | Starts a DMA-backed transmit operation |
| `spi_is_busy(void)` | Returns nonzero while DMA or the SPI bus is still active |

### Dependencies

- Depends on the DMA driver.
- Requires BSP configuration for `SPI1`, `DMA2`, and `DMA2_Stream3_IRQn`.

### Usage notes

- TX only; the driver does not configure `MISO`.
- There is no chip-select API for an external slave.
- The caller must keep the TX buffer valid until DMA and SPI are both finished.
- Completion is reported through a callback chain triggered from the DMA interrupt path.

## DMA Driver

### Purpose

Provides the memory-to-peripheral transfer used by the current SPI TX implementation.

### Public interface

| API | Notes |
| --- | --- |
| `dma_init(uint32_t peripheral_addr, dma_callback_t callback)` | Configures `DMA2 Stream3 Channel3` |
| `dma_set_error_callback(dma_callback_t callback)` | Registers an error callback |
| `dma_start_tx(uint8_t *data, uint16_t len)` | Starts a normal-mode transfer |
| `dma_is_busy(void)` | Returns whether the stream is enabled |
| `dma_get_error_count(void)` | Returns the accumulated DMA error count |

### Dependencies

- Board initialization must already enable `DMA2`.
- Assumes the target mapping `DMA2 Stream3 Channel3 -> SPI1_TX`.

### Usage notes

- This is not a general-purpose DMA framework.
- Argument validation is minimal.
- Transfer-complete and error callbacks execute in interrupt context.
- FIFO error flags are defined but not currently handled in the IRQ path.

## Ring Buffer Utility

### Purpose

Implements a compact single-producer/single-consumer circular buffer for UART receive bytes.

### Public interface

| API | Notes |
| --- | --- |
| `ring_buffer_init(ring_buffer_t *rb, uint8_t *buffer, uint16_t size)` | Initializes storage and indices |
| `ring_buffer_put(ring_buffer_t *rb, uint8_t data)` | Returns `1` on success, `0` if full |
| `ring_buffer_get(ring_buffer_t *rb, uint8_t *data)` | Returns `1` on success, `0` if empty |
| `ring_buffer_available(ring_buffer_t *rb)` | Returns the buffered byte count |
| `ring_buffer_clear(ring_buffer_t *rb)` | Discards unread data |

### Usage notes

- Effective capacity is `size - 1`.
- Safe for the current pattern where the ISR only advances `head` and the foreground only advances `tail`.
- No argument validation is built in.

## BSP

### Purpose

Hides board-specific details behind a small API so higher layers do not manipulate STM32F4-Discovery registers directly.

### Public interface

| API | Notes |
| --- | --- |
| `bsp_clock_init(void)` | Configures the clock tree and enables peripheral clocks |
| `bsp_gpio_init(void)` | Programs GPIO modes, alternate functions, LEDs, and button |
| `bsp_nvic_init(void)` | Sets priorities and enables interrupt lines |
| `bsp_board_init(void)` | Runs clock, GPIO, and NVIC setup |
| `bsp_led_on/off/toggle()` | Controls onboard LEDs |
| `bsp_button_read()` | Reads the user button |

### Usage notes

- Tied to the STM32F4-Discovery family.
- Holds the hardware contract that the other drivers rely on.

## System Facade

### Purpose

Preserves a clean initialization API above the BSP. The current implementation is intentionally thin.

### Public interface

| API | Notes |
| --- | --- |
| `system_clock_init(void)` | Delegates to `bsp_clock_init()` |
| `system_gpio_init(void)` | Delegates to `bsp_gpio_init()` |
| `system_nvic_init(void)` | Delegates to `bsp_nvic_init()` |
| `system_init_all(void)` | Delegates to `bsp_board_init()` |

### Usage notes

- Useful if additional boards are introduced later while preserving the application-facing API.
- Keeps the application layer free from direct board selection logic.

## Integration Observations

- The driver set is coherent and appropriate for a prototype bring-up platform.
- The strongest reuse boundary is between `Application` and the lower-level hardware layers.
- The least reusable module is the DMA driver because it is intentionally fixed to one stream/channel mapping.
