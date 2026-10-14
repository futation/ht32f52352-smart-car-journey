# Third-Party Notices / 第三方组件声明

本目录（`project/HT32F52352_SmartCar_PID/`）包含以下第三方组件，**均为官方原样分发、未做任何修改**，仅出于让读者"clone 后即可编译"的目的随仓库提供：

## 1. Holtek HT32F5xxxx 标准外设库

- 位置：`Libraries/HT32F5xxxx_Driver/`、`Libraries/Device/Holtek/`、`Libraries/HT32_USBD_Library/`、`Libraries/CMSIS/Include`（部分）、`Utilities/`
- 版权：**Copyright (C) Holtek Semiconductor Inc. All rights reserved**
- 来源：Holtek 官网（[www.holtek.com](https://www.holtek.com)）HT32F52352 产品页公开提供的固件库包，免注册下载
- 许可：随库文件头部附带 Holtek 的 Firmware Disclaimer（见各 `.c/.h` 文件头 `@attention` 段落），一切权利归 Holtek 所有；本仓库中它们**不适用**仓库根目录的 MIT 许可
- 说明：若你是 Holtek 方权利人并希望移除这些文件，请提 issue，将立即处理

## 2. ARM CMSIS

- 位置：`Libraries/CMSIS/`
- 版权：Copyright (C) ARM Limited / ARM China
- 许可：随附原始许可文本 `Libraries/CMSIS/CMSIS_END_USER_LICENCE_AGREEMENT.pdf` 及 `Libraries/CMSIS/README.txt`，一切权利归 ARM 所有

## 本项目原创部分

以下内容为仓库作者原创，以仓库根目录的 **MIT** 许可发布：

- `Hardware/`（整车驱动模块）
- `User/`（`main.c`、中断服务）
- `drivers/`（仓库根目录的提炼版驱动画廊，与 `Hardware/` 同源）
- `docs/`（全部文档）

> 其中部分驱动的**思路**参考了公开的 STM32 教程例程（如江协科技 MPU-6050/串口例程）与 Holtek 官方例程框架，均按上述 API 重写或适配，第三方文件本身未混入这些目录。
