# [platform-agm32](https://github.com/os-q/platform-agm32)

[![Build Status](https://github.com/os-q/platform-agm32/workflows/examples/badge.svg)](https://github.com/os-q/platform-agm32/actions/workflows/examples.yml)

基于国产异构双核（RISC-V+FPGA）MCU芯片 [AG32VF407](https://github.com/SoCXin/AG32VF407)，具有2K LES(CPLD)和丰富的外设。

# Usage

已安装情况下直接使用
```ini
[env:stable]
platform = agm32rv
board = ...
framework = agrv_sdk
...
```
使用在线仓库版本
```ini
[env:development]
platform = https://github.com/os-q/platform-agm32.git
board = ...
framework = agrv_sdk
...
```