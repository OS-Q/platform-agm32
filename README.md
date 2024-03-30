# [platform-agm32rv](https://github.com/os-q/platform-agm32)

[![Build Status](https://github.com/os-q/platform-agm32/workflows/examples/badge.svg)](https://github.com/os-q/platform-agm32/actions/workflows/examples.yml)

# Usage

```ini
[env:stable]
platform = agm32rv
board = ...
framework = agrv_sdk
...
```

```ini
[env:development]
platform = https://github.com/os-q/platform-agm32.git
board = ...
framework = agrv_sdk
...
```

```bash
debug_tool = cmsis-dap
upload_protocol = cmsis-dap
```