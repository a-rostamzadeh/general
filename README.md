# STM32H7 General Utilities Library

![STM32](https://img.shields.io/badge/STM32-H7-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![Platform](https://img.shields.io/badge/Platform-STM32CubeIDE-orange.svg)

A comprehensive general-purpose utility library for STM32H7 microcontrollers, providing hardware abstraction, RGY 3-pin LED control, rotary encoder handling, key debouncing, timing utilities, and debugging support.

## 📋 Table of Contents

- [Features](#features)
- [Hardware Support](#hardware-support)
- [Getting Started](#getting-started)
- [Encoder Configuration](#encoder-configuration)
- [API Documentation](#api-documentation)
- [Examples](#examples)
- [Configuration](#configuration)
- [Debugging](#debugging)
- [Contributing](#contributing)
- [License](#license)

## ✨ Features

### Core Utilities
- **Microsecond Delay** - Busy-wait delay with configurable timing
- **Time Elapsed Checker** - Non-blocking timeout detection
- **Timer Frequency Configuration** - Automatic prescaler calculation for any frequency
- **String Utilities** - Whitespace checking
- **Hardware ID Reading** - Access to STM32H7 unique device ID (96-bit)

### Input Handling
- **Rotary Encoder Support** - Read position with noise filtering
  - Supports up to 4 encoders (FIRST_ENC, SECOND_ENC, THIRD_ENC, FOURTH_ENC)
- **Encoder Direction Detection** - Clockwise/counter-clockwise detection
- **Key Debouncing** - Edge detection with debouncing
- **Shift Key Support** - Handle secondary button functions

### Output Control
- **LED Management** - Single and bi-color LED control
- **Color Support** - Red, Green, Yellow, and off states
- **Stage LED Arrays** - Manage multiple LEDs efficiently

### Debugging
- **Debug Messaging** - Formatted output with timestamp support
- **Binary Conversion** - Display numbers in binary format
- **Configurable Output** - Conditional compilation for release builds

## 🎯 Hardware Support

### Supported Microcontrollers
- STM32H7 series (tested on STM32H743)
- Compatible with STM32H7A3, STM32H7B3, STM32H7B0

### Encoder to Timer Mapping
| Encoder Type | Timer | Typical Use |
|--------------|-------|-------------|
| `FIRST_ENC`  | TIM4  | R control   |
| `SECOND_ENC` | TIM3  | S control   |
| `THIRD_ENC`  | TIM23 | T control   |


### Peripheral Requirements
- **Timers**: TIM2, TIM3, TIM4, TIM23 (configurable)
- **GPIO**: Any GPIO pins with LED/button connections
- **USART**: For debug output (when enabled)
- **Flash**: For UDID access

### Dependencies
- STM32CubeH7 HAL Library
- CMSIS Core
- Custom `serial.h` for UART communication
- Custom `main.h` for project definitions

## 🚀 Getting Started

### Prerequisites
```bash
- STM32CubeIDE 1.8.0 or later
- STM32CubeH7 Firmware Package v1.10.0 or later
- ARM GCC Toolchain
