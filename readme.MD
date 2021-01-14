# ASR6505 sdk
该SDK是针对翱捷公司ASR6505单片机；该版本主要是支持lorawan平台，支持AT指令；如需支持linkwan，请移步：https://github.com/zhangjun1992/asr6505_sdk_at_linkwan

其他：LoRa物联网群：753601669

## 注意
只支持class A、C模式。
支持AT指令
### 2、2020年3月20日
修复ABP模式下，devAddr设置错误和显示错误。错误原因主要是stm8是8位单片机，处理过程移位导致溢出，并提醒小伙伴注意不要使用%08X打印debug信息，stm8会溢出，只会显示前面两个字节，lorawan ABP收发数据正常


