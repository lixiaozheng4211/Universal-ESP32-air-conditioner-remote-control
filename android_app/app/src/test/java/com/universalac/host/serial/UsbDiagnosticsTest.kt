package com.universalac.host.serial

import org.junit.Assert.assertTrue
import org.junit.Test

class UsbDiagnosticsTest {
    @Test
    fun emptyDeviceListPointsToOtgPowerOrCableLayer() {
        val text = UsbDiagnostics.format(emptyList())

        assertTrue(text.contains("未发现任何 USB 设备"))
        assertTrue(text.contains("OTG"))
        assertTrue(text.contains("供电"))
    }

    @Test
    fun unrecognizedDeviceShowsVidPidForDriverInvestigation() {
        val text = UsbDiagnostics.format(
            listOf(
                UsbDeviceSummary(
                    name = "/dev/bus/usb/001/002",
                    vendorId = 0x1A86,
                    productId = 0x7523,
                    interfaceCount = 1,
                    recognizedSerialDriver = false,
                    portCount = 0
                )
            )
        )

        assertTrue(text.contains("1A86:7523"))
        assertTrue(text.contains("未识别为串口"))
    }
}
