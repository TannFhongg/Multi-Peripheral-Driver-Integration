# STM32F4 Multi-Peripheral Firmware

This repository contains bare-metal STM32F4 firmware organized as a small embedded platform for integrating the peripherals. The current codebase focuses on board bring-up and peripheral coordination rather than autonomy algorithms: direct-register clock/GPIO/NVIC initialization, a UART command shell, I2C sampling, SPI transmit via DMA, and simple runtime diagnostics using onboard LEDs and a user button.

The implementation targets the `STM32F4DISCOVERY / STM32F407G-DISC1` with an `STM32F407VGT6`. It does not use STM32 HAL, LL, or an RTOS; the drivers access peripheral registers directly.

## Why This Project Exists

It is reliable low-level integration: clocks, GPIO alternate functions, interrupt setup, serial diagnostics, sensor bus access, and efficient peripheral data movement. This repository demonstrates that integration layer in a compact and reviewable form.

What is currently in scope:

- Board bring-up for STM32F4-Discovery.
- A cooperative application loop.
- UART-based command and status output.
- I2C master polling transactions.
- SPI transmit using DMA.
- Basic runtime visibility through counters, LEDs, and a push button.

What is not currently in scope:

- Motor control, PWM drive stages, encoder feedback, or actuator safety logic.
- Sensor fusion, localization, path planning, or closed-loop vehicle control.
- A complete standalone build system with startup files and linker script checked into this repository.

## Implemented Features

- Register-level BSP for clock tree, GPIO alternate functions, NVIC configuration, LEDs, and button input.
- Layered firmware organization across `Application`, `System`, `BSP`, `Drivers`, and `Utils`.
- UART command shell over `USART1` at `115200 8N1`.
- Interrupt-driven UART receive path backed by a ring buffer.
- Polling-based `I2C1` master read/write support.
- `SPI1` transmit-only data path using `DMA2 Stream3 Channel3`.
- Application status tracking for SPI, DMA, and I2C activity/error counts.
- LED-based status indication and push-button-triggered SPI transfer.

## Architecture At a Glance

```text
main.c
  -> Application
       -> System facade
            -> BSP (board bring-up)
       -> UART driver
       -> I2C driver
       -> SPI driver
            -> DMA driver
       -> ring_buffer utility
```

| Layer | Primary files | Responsibility |
| --- | --- | --- |
| Entry point | `main.c` | Calls `app_init()` and `app_run()` |
| Application | `Application/app.c`, `Application/app.h`, `Application/app_config.h` | Command handling, status, button polling, LED policy, SPI/I2C orchestration |
| System | `System/system_init.c`, `System/system_init.h` | Compatibility facade that delegates initialization to the selected BSP |
| BSP | `BSP/board.h`, `BSP/stm32f4_discovery.*` | Target-board clocks, pin muxing, peripheral clock enables, NVIC setup, LEDs, button |
| Drivers | `Drivers/*` | Register-level UART, I2C, SPI, and DMA access |
| Utils | `Utils/ring_buffer.*` | Single-producer/single-consumer circular buffer for UART RX |

More detail is available in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Supported Hardware and Peripherals

The following map is derived directly from the source code.

| Item | Current configuration |
| --- | --- |
| Board | `STM32F4DISCOVERY / STM32F407G-DISC1` |
| MCU | `STM32F407VGT6` |
| Clock tree | `HSE 8 MHz -> PLL -> SYSCLK 168 MHz` |
| AHB / APB1 / APB2 | `168 MHz / 42 MHz / 84 MHz` |
| UART | `USART1`, `PA9 TX`, `PA10 RX`, `AF7` |
| I2C | `I2C1`, `PB6 SCL`, `PB7 SDA`, `AF4`, open-drain with pull-up |
| SPI | `SPI1`, `PA5 SCK`, `PA7 MOSI`, `AF5`, transmit-only |
| DMA | `DMA2 Stream3 Channel3` for `SPI1_TX` |
| LEDs | `PD12 green`, `PD13 orange`, `PD14 red`, `PD15 blue` |
| User button | `PA0`, active high |
| Shared onboard SPI device handling | `PE3` held high to keep the onboard MEMS device deselected |

Important hardware note: STM32F4-Discovery does not provide an onboard USB virtual COM port for `USART1`, so a `3.3 V` USB-UART adapter is required for serial interaction.

See [docs/HARDWARE.md](docs/HARDWARE.md) for a fuller hardware-oriented reference.

## Runtime Behavior

`main.c` is intentionally minimal:

```c
int main(void)
{
    app_init();
    app_run();
}
```

During `app_init()` the firmware:

1. Calls `system_init_all()`, which delegates to the BSP.
2. Initializes the board clock tree, GPIO, and NVIC.
3. Turns off all four LEDs.
4. Initializes `USART1`, `I2C1`, and `SPI1`.
5. Captures the initial DMA error count.
6. Switches the application to `APP_STATE_READY`.
7. Prints a startup banner and command help over UART.

During `app_run()`, `app_process()` repeats forever and services:

- UART command parsing from the RX ring buffer.
- User-button polling with loop-based debounce.
- Deferred SPI DMA completion/error handling.
- A heartbeat LED toggle.

## Command Interface

Serial settings:

```text
115200 baud, 8 data bits, no parity, 1 stop bit, no flow control
```

USB-UART wiring:

| USB-UART adapter | STM32F4-Discovery |
| --- | --- |
| `TXD` | `PA10 / USART1_RX` |
| `RXD` | `PA9 / USART1_TX` |
| `GND` | `GND` |

Supported commands:

| Command | Behavior |
| --- | --- |
| `?`, `h`, `H` | Print help |
| `p`, `P` | Print application state and counters |
| `s`, `S` | Start an SPI DMA transfer |
| `i`, `I` | Read `APP_I2C_SAMPLE_LENGTH` bytes from `APP_I2C_SAMPLE_ADDR` |
| `l`, `L` | Toggle the blue LED |
| `c`, `C` | Clear the latched error state and turn off the red LED |

LED meaning:

| LED | Meaning |
| --- | --- |
| Green | Main-loop heartbeat |
| Orange | SPI/DMA transfer in progress |
| Blue | Activity or successful transaction indication |
| Red | Latched error condition |

The user button on `PA0` also triggers an SPI DMA transfer.

## Application Configuration

Key configuration values are defined in [Application/app_config.h](Application/app_config.h):

| Macro | Value | Meaning |
| --- | ---: | --- |
| `APP_UART_BAUDRATE` | `115200` | Serial command shell baud rate |
| `APP_HEARTBEAT_DIVIDER` | `200000` | Loop-count divider for the green LED heartbeat |
| `APP_BUTTON_DEBOUNCE_LOOPS` | `20000` | Loop-based debounce for the user button |
| `APP_I2C_SAMPLE_ADDR` | `0x68` | Default 7-bit I2C test address |
| `APP_I2C_SAMPLE_LENGTH` | `2` | Number of bytes read by the I2C sample command |
| `APP_SPI_TX_LENGTH` | `8` | Number of bytes sent in each SPI DMA transfer |

If the external I2C device uses a different address, update `APP_I2C_SAMPLE_ADDR` before testing the `i` command.

## Repository Structure

```text
.
|-- Application/
|-- BSP/
|-- Drivers/
|   |-- dma/
|   |-- i2c/
|   |-- spi/
|   `-- uart/
|-- System/
|-- Utils/
|-- docs/
|-- main.c
|-- README.md
`-- CLAUDE.md
```

Documentation map:

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md): software layering, control flow, and module interactions.
- [docs/HARDWARE.md](docs/HARDWARE.md): board assumptions, pin map, and external hardware notes.
- [docs/DRIVERS.md](docs/DRIVERS.md): driver-by-driver API and implementation notes.
- [docs/BRINGUP.md](docs/BRINGUP.md): practical bring-up workflow and troubleshooting.
- [docs/PORTFOLIO_NOTES.md](docs/PORTFOLIO_NOTES.md): CV/interview wording and repository metadata suggestions.

## Build and Bring-Up

This repository does not currently include a startup file, linker script, or standalone build system. As a result, it is best treated as portable firmware source that can be imported into an STM32 project rather than flashed directly from this repository alone.

What is available today:

- The source passes a syntax-only check with `arm-none-eabi-gcc`.
- The required IRQ handlers are present in source:
  `USART1_IRQHandler` and `DMA2_Stream3_IRQHandler`.

Syntax check command:

```powershell
arm-none-eabi-gcc -DBOARD_STM32F4_DISCOVERY `
  -mcpu=cortex-m4 -mthumb -std=c11 -Wall -Wextra `
  -I. -IApplication -IBSP -IDrivers -ISystem -IUtils -fsyntax-only `
  main.c Application\app.c BSP\stm32f4_discovery.c System\system_init.c `
  Drivers\i2c\i2c_driver.c Drivers\uart\uart_driver.c `
  Drivers\spi\spi_driver.c Drivers\dma\dma_driver.c Utils\ring_buffer.c
```

Practical path to run on hardware:

1. Create a new STM32 project for `STM32F407VGTx` in STM32CubeIDE or another Cortex-M build environment.
2. Use the IDE-generated startup file and linker script for the STM32F407VG memory map.
3. Copy `main.c`, `Application/`, `BSP/`, `Drivers/`, `System/`, and `Utils/` into that project.
4. Add the include paths `.`, `Application`, `BSP`, `Drivers`, `System`, and `Utils`.
5. Define `BOARD_STM32F4_DISCOVERY`.
6. Ensure the vector table references `USART1_IRQHandler` and `DMA2_Stream3_IRQHandler`.
7. Build, flash, and debug through the onboard ST-LINK.

Detailed notes are in [docs/BRINGUP.md](docs/BRINGUP.md).

## Skills Demonstrated

This project is a good portfolio example for showing practical embedded firmware fundamentals:

| Skill area | Evidence in the repository |
| --- | --- |
| Bare-metal embedded C | Direct peripheral register access throughout BSP and drivers |
| Board bring-up | Clock tree, GPIO alternate functions, NVIC, LED/button helpers |
| Driver abstraction | Separate modules for UART, I2C, SPI, DMA, and ring buffer utility |
| Interrupt design | `USART1_IRQHandler` for RX buffering and `DMA2_Stream3_IRQHandler` for transfer completion/error |
| DMA integration | SPI transmit path offloaded to DMA2 Stream3 |
| Cooperative firmware structure | Main loop with explicit service functions instead of RTOS tasks |
| Diagnostics and observability | UART status output, counters, LED state mapping, error latching |
| Embedded code organization | Clear split between application logic, hardware abstraction, drivers, and utilities |

## Current Limitations

- `SPI1` is transmit-only; `MISO` is not configured and there is no external chip-select API.
- The DMA driver is hardcoded for `DMA2 Stream3 Channel3` and is not a general DMA abstraction.
- I2C is blocking/polling and still lacks stronger NACK handling, bus recovery, and `I2C_CR2` frequency setup.
- UART transmit is blocking, and RX overflow is not exposed through a public diagnostics API.
- The debounce and heartbeat mechanisms are loop-count-based rather than timer-based.
- No startup assembly, linker script, CMSIS device package, or flashable project files are stored in the repository.

## Future Improvements

- Extend SPI with chip-select handling and optional receive/full-duplex support.
- Harden the I2C driver for real hardware bring-up with explicit error-flag handling and recovery.
- Surface UART diagnostics such as overflow and framing-error counters through a public API.
- Add timer-driven timekeeping instead of loop-count heuristics.
- Expand the platform toward vehicle-specific modules such as motor control, sensing, and higher-level control logic.

## Portfolio Summary

This repository is best presented as a prototype-level STM32F4 firmware integration platform for an embedded . It shows credible low-level engineering work: board support, peripheral drivers, interrupts, DMA, and layered firmware structure. It should not be presented as a complete autonomous car stack.

For CV wording, interview talking points, GitHub description ideas, and topic suggestions, see [docs/PORTFOLIO_NOTES.md](docs/PORTFOLIO_NOTES.md).
