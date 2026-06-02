package com.universalac.host.serial

data class UsbDeviceSummary(
    val name: String,
    val vendorId: Int,
    val productId: Int,
    val interfaceCount: Int,
    val recognizedSerialDriver: Boolean,
    val portCount: Int
)

object UsbDiagnostics {
    fun format(devices: List<UsbDeviceSummary>): String {
        if (devices.isEmpty()) {
            return "未发现任何 USB 设备。\n\n" +
                "优先检查 OTG 转接头、USB 线是否支持数据、手机是否开启 OTG，以及手机是否给外设供电。"
        }

        return buildString {
            appendLine("发现 ${devices.size} 个 USB 设备:")
            devices.forEach { device ->
                appendLine()
                appendLine("${device.name}")
                appendLine("VID:PID ${device.vendorId.hex4()}:${device.productId.hex4()}")
                appendLine("接口数: ${device.interfaceCount}")
                if (device.recognizedSerialDriver) {
                    appendLine("串口驱动: 已识别, ports=${device.portCount}")
                } else {
                    appendLine("串口驱动: 未识别为串口")
                }
            }
        }
    }

    private fun Int.hex4(): String = toString(16).uppercase().padStart(4, '0')
}
