# Bring-Up Guide

## Current Repository State

This repository contains firmware source, but it does not yet include all files required to build a flashable image directly from the repo.

Missing items:

- Startup assembly file
- Linker script
- Complete STM32 project metadata or standalone build system
- CMSIS device package headers

As a result, the recommended workflow is to import these sources into an STM32 project that generates the startup and linker baseline for `STM32F407VGTx`.

## What Has Been Verified

The source files pass a syntax-only compile check with `arm-none-eabi-gcc`:

```powershell
arm-none-eabi-gcc -DBOARD_STM32F4_DISCOVERY `
  -mcpu=cortex-m4 -mthumb -std=c11 -Wall -Wextra `
  -I. -IApplication -IBSP -IDrivers -ISystem -IUtils -fsyntax-only `
  main.c Application\app.c BSP\stm32f4_discovery.c System\system_init.c `
  Drivers\i2c\i2c_driver.c Drivers\uart\uart_driver.c `
  Drivers\spi\spi_driver.c Drivers\dma\dma_driver.c Utils\ring_buffer.c
```

This confirms the current source set is internally consistent at the C level, but it does not prove end-to-end hardware validation by itself.

## Hardware Needed

| Purpose | Recommended hardware |
| --- | --- |
| MCU board | `STM32F4DISCOVERY / STM32F407G-DISC1` |
| Flash/debug | Onboard ST-LINK USB connection |
| UART console | External `3.3 V` USB-UART adapter |
| I2C test | An external I2C device, default address `0x68` unless `APP_I2C_SAMPLE_ADDR` is changed |
| SPI test | An SPI target that can accept TX-only traffic |
| Signal inspection | Optional oscilloscope or logic analyzer for UART/I2C/SPI verification |

## Software Needed

| Purpose | Example tool |
| --- | --- |
| Embedded toolchain | ARM GNU Toolchain / STM32CubeCLT |
| STM32 project generation | STM32CubeIDE or equivalent |
| Flash/debug | STM32CubeIDE or STM32CubeProgrammer |
| Serial terminal | PuTTY, Tera Term, RealTerm, or similar |

## Recommended Bring-Up Flow

### 1. Inspect application configuration

Review `Application/app_config.h` before flashing:

- `APP_UART_BAUDRATE`
- `APP_I2C_SAMPLE_ADDR`
- `APP_I2C_SAMPLE_LENGTH`
- `APP_SPI_TX_LENGTH`
- `APP_BUTTON_DEBOUNCE_LOOPS`
- `APP_HEARTBEAT_DIVIDER`

### 2. Create a flashable STM32 project

One practical route is STM32CubeIDE:

1. Create a new project for `STM32F407VGTx`.
2. Let the IDE generate the startup file and linker script.
3. Copy these repository folders/files into that project:
   `main.c`, `Application/`, `BSP/`, `Drivers/`, `System/`, `Utils/`.
4. Add include paths for `.`, `Application`, `BSP`, `Drivers`, `System`, and `Utils`.
5. Define `BOARD_STM32F4_DISCOVERY`.
6. Ensure the vector table points to:
   `USART1_IRQHandler` and `DMA2_Stream3_IRQHandler`.

No build or flash CLI command is stored in this repository, so any exact flashing command should be treated as project-specific TODO work.

### 3. Connect the UART console

Wiring:

| USB-UART adapter | STM32 board |
| --- | --- |
| `TXD` | `PA10 / USART1_RX` |
| `RXD` | `PA9 / USART1_TX` |
| `GND` | `GND` |

Serial settings:

```text
115200 baud, 8 data bits, no parity, 1 stop bit, no flow control
```

### 4. Power, flash, and reset

Use the onboard ST-LINK USB connection for power/debug. After flashing, reset the board and open the UART terminal.

Expected startup text:

```text
STM32F4 Discovery application ready
```

The firmware also prints a short help menu after startup.

### 5. Run a basic smoke test

Recommended sequence:

1. Type `?` to confirm the UART command parser is active.
2. Type `p` to print the current application counters and state.
3. Type `l` to toggle the blue LED.
4. Type `s` to start an SPI DMA transfer.
5. Press the user button on `PA0` to trigger another SPI transfer.
6. Type `i` to attempt an I2C sample read from the configured address.

## LED Behavior During Bring-Up

| LED | Meaning |
| --- | --- |
| Green | Main-loop heartbeat |
| Orange | SPI transfer currently active |
| Blue | Activity or successful SPI/I2C event |
| Red | Latched error state |

## Practical Notes for I2C Testing

- The default test address is `0x68`.
- If no device is connected at that address, the `i` command is expected to fail.
- External pull-up resistors on `SCL` and `SDA` are recommended.
- The current driver is suitable for basic demonstration, not for a fully hardened field bus implementation.

## Practical Notes for SPI Testing

- The current SPI path is TX only.
- The code configures `PA5` and `PA7`; `PA6` is unused.
- The repository does not define an external chip-select line.
- If the target device requires CS, add a GPIO-controlled CS path before treating the transaction as complete system integration.

## Troubleshooting

### No UART output

Check:

1. The firmware was actually flashed into a project with a valid startup file and vector table.
2. The terminal is connected to the correct COM port.
3. Serial settings are `115200 8N1`.
4. TX/RX are crossed correctly.
5. Adapter logic level is `3.3 V`.
6. `USART1_IRQHandler` is present in the vector table.

### SPI command shows no useful behavior

Check:

1. `DMA2_Stream3_IRQHandler` is present in the vector table.
2. `PA5` and `PA7` are wired to the target device.
3. The target device does not require an unmanaged chip-select line.
4. Orange LED activity appears when `s` is sent.

### I2C sample fails

Check:

1. The external device address matches `APP_I2C_SAMPLE_ADDR`.
2. `PB6/PB7` wiring is correct.
3. Grounds are shared.
4. Pull-ups are present.
5. The connected device is powered correctly.

## Known Bring-Up Gaps

- No checked-in build system for one-command rebuilds.
- No scripted flash command stored in the repository.
- No board-level electrical documentation beyond the code-inferred mapping.
- No defined external SPI slave or I2C sensor reference design.

These are appropriate follow-up improvements if the goal is to move from a portfolio prototype to a reproducible lab project.
