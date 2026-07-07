# Hardware Notes

## Scope

This document describes the hardware assumptions that are visible in the source code. It is intentionally conservative: if the repository does not define a sensor, shield, or actuator explicitly, it is treated as unspecified.

## Target Board

| Item | Value from source |
| --- | --- |
| Board | `STM32F4DISCOVERY / STM32F407G-DISC1` |
| MCU | `STM32F407VGT6` |
| External high-speed clock | `8 MHz HSE` |
| SYSCLK | `168 MHz` |
| AHB clock | `168 MHz` |
| APB1 clock | `42 MHz` |
| APB2 clock | `84 MHz` |

These values are defined in `BSP/stm32f4_discovery.h` and configured in `BSP/stm32f4_discovery.c`.

## Pin and Peripheral Usage

The following mapping is inferred directly from the BSP and driver code.

| Function | Peripheral | Pin(s) | Notes |
| --- | --- | --- | --- |
| UART TX | `USART1` | `PA9` | Alternate function `AF7` |
| UART RX | `USART1` | `PA10` | Alternate function `AF7` |
| I2C SCL | `I2C1` | `PB6` | `AF4`, open-drain, pull-up |
| I2C SDA | `I2C1` | `PB7` | `AF4`, open-drain, pull-up |
| SPI SCK | `SPI1` | `PA5` | `AF5` |
| SPI MOSI | `SPI1` | `PA7` | `AF5` |
| SPI MISO | `SPI1` | not configured | Current firmware is TX-only |
| SPI shared-device deselect | GPIO | `PE3` | Driven high to keep the onboard MEMS device deselected |
| Green LED | GPIO | `PD12` | Heartbeat |
| Orange LED | GPIO | `PD13` | SPI busy |
| Red LED | GPIO | `PD14` | Latched error indicator |
| Blue LED | GPIO | `PD15` | Activity indicator |
| User button | GPIO | `PA0` | Polled, active high |

## External Hardware Expected for Testing

The source implies the following external equipment for practical bring-up:

| Purpose | Hardware needed |
| --- | --- |
| Programming/debug | Onboard ST-LINK over USB |
| UART console | External `3.3 V` USB-UART adapter |
| I2C test command | An external I2C device at a configurable 7-bit address, default `0x68` |
| SPI test command | An external SPI-compatible target that can accept TX-only traffic |

## UART Wiring

For the UART command shell:

| USB-UART adapter | STM32 board |
| --- | --- |
| `TXD` | `PA10 / USART1_RX` |
| `RXD` | `PA9 / USART1_TX` |
| `GND` | `GND` |

Notes:

- Use a `3.3 V` logic-level adapter.
- Connect TX/RX crossed.
- The board is already powered by ST-LINK USB during most debug sessions, so adapter `VCC` is typically not required.

## I2C Assumptions

The I2C driver initializes `I2C1` for a simple standard-mode configuration based on an assumed `42 MHz` APB1 clock.

Relevant values in source:

- `CCR = 210`
- `TRISE = 43`
- Sample address default: `0x68`
- Sample length default: `2` bytes

Observed limitations:

- The code does not identify a specific sensor model.
- `I2C_CR2` frequency configuration is not currently programmed in `i2c_init()`.
- Pull-ups are configured in GPIO, but external pull-up resistors are still recommended for real hardware.

## SPI Assumptions

The SPI path currently represents a minimal TX-only integration:

- `SPI1` master mode.
- `CPOL = 0`, `CPHA = 0`.
- Software-managed internal NSS (`SSM` + `SSI`).
- Prescaler `fPCLK / 16`.
- DMA-backed transmit using `DMA2 Stream3 Channel3`.

Observed limitations:

- No `MISO` pin setup.
- No external chip-select GPIO is implemented for an attached slave.
- The firmware does not define what the SPI target device is.

If the external device requires chip-select handling, that must be added in BSP and/or application logic.

## Onboard MEMS Handling

The board support code drives `PE3` high before configuring it as an output. This keeps the onboard MEMS device deselected while `SPI1` is reused for an external target. That is a helpful integration detail because the STM32F4-Discovery shares `SPI1` with onboard hardware.

## What the Repository Does Not Yet Specify

The following hardware details are not visible in the current source tree:

- Mini car chassis or drive train selection.
- Motor driver or H-bridge interface.
- Wheel encoder inputs.
- Battery or power-management design.
- Sensor part numbers beyond the generic I2C address.
- Any PWM, timer capture, ADC, CAN, USB, or wireless peripheral usage.

In other words, the repository currently documents a reusable embedded controller foundation, not the complete electrical design of a mini car.
