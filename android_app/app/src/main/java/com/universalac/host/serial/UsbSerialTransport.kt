package com.universalac.host.serial

import android.content.Context
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbManager
import com.hoho.android.usbserial.driver.Ch34xSerialDriver
import com.hoho.android.usbserial.driver.UsbSerialPort
import com.hoho.android.usbserial.driver.UsbSerialProber
import com.hoho.android.usbserial.util.SerialInputOutputManager
import java.io.IOException

class UsbSerialTransport(
    context: Context,
    private val listener: Listener
) {
    interface Listener {
        fun onLineReceived(line: String)
        fun onStatusChanged(status: String)
        fun onConnectionLost(reason: String)
    }

    private val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
    private val serialProber = UsbSerialProber(customProbeTable())
    private var connection: UsbDeviceConnection? = null
    private var port: UsbSerialPort? = null
    private var ioManager: SerialInputOutputManager? = null
    private var receiveBuffer = StringBuilder()

    fun findDrivers() = serialProber.findAllDrivers(usbManager)

    fun firstDevice(): UsbDevice? = findDrivers().firstOrNull()?.device

    fun deviceSummaries(): List<UsbDeviceSummary> {
        val driversByDeviceName = findDrivers().associateBy { it.device.deviceName }
        return usbManager.deviceList.values.map { device ->
            val driver = driversByDeviceName[device.deviceName]
            UsbDeviceSummary(
                name = device.deviceName,
                vendorId = device.vendorId,
                productId = device.productId,
                interfaceCount = device.interfaceCount,
                recognizedSerialDriver = driver != null,
                portCount = driver?.ports?.size ?: 0
            )
        }.sortedWith(compareBy<UsbDeviceSummary> { it.vendorId }.thenBy { it.productId })
    }

    fun hasPermission(device: UsbDevice): Boolean = usbManager.hasPermission(device)

    fun openFirst(): Boolean {
        val driver = findDrivers().firstOrNull()
        if (driver == null) {
            listener.onStatusChanged("未找到 USB 串口")
            return false
        }
        val selectedPort = driver.ports.firstOrNull()
        if (selectedPort == null) {
            listener.onStatusChanged("未找到串口端口")
            return false
        }
        val selectedConnection = usbManager.openDevice(driver.device)
        if (selectedConnection == null) {
            listener.onStatusChanged("USB 权限未授予")
            return false
        }

        return try {
            close()
            connection = selectedConnection
            port = selectedPort
            selectedPort.open(selectedConnection)
            selectedPort.setParameters(
                115200,
                8,
                UsbSerialPort.STOPBITS_1,
                UsbSerialPort.PARITY_NONE
            )
            // Match the desktop Qt host: 115200 8N1, no hardware flow control.
            // Some ESP32 auto-program circuits wire DTR/RTS to EN/BOOT, so do
            // not actively toggle them during normal command transport.
            startReader(selectedPort)
            listener.onStatusChanged("已连接 ${driver.device.deviceName}")
            true
        } catch (error: Exception) {
            runCatching { selectedPort.close() }
            selectedConnection.close()
            connection = null
            port = null
            listener.onConnectionLost(error.message ?: "打开串口失败")
            false
        }
    }

    fun isOpen(): Boolean = port != null

    fun sendLine(line: String): Boolean {
        val activePort = port ?: return false
        return try {
            activePort.write((line + "\n").toByteArray(Charsets.UTF_8), WRITE_TIMEOUT_MS)
            true
        } catch (error: IOException) {
            close()
            listener.onConnectionLost(error.message ?: "串口发送失败")
            false
        }
    }

    fun close() {
        ioManager?.stop()
        ioManager = null
        runCatching { port?.close() }
        port = null
        connection?.close()
        connection = null
        receiveBuffer.clear()
        listener.onStatusChanged("未连接")
    }

    private fun startReader(activePort: UsbSerialPort) {
        val manager = SerialInputOutputManager(
            activePort,
            object : SerialInputOutputManager.Listener {
                override fun onNewData(data: ByteArray) {
                    appendData(data)
                }

                override fun onRunError(e: Exception) {
                    close()
                    listener.onConnectionLost(e.message ?: "串口读取失败")
                }
            }
        )
        ioManager = manager
        manager.start()
    }

    private fun appendData(data: ByteArray) {
        receiveBuffer.append(String(data, Charsets.UTF_8))
        while (true) {
            val newline = receiveBuffer.indexOf("\n")
            if (newline < 0) {
                break
            }
            val line = receiveBuffer.substring(0, newline).trim()
            receiveBuffer.delete(0, newline + 1)
            if (line.isNotEmpty()) {
                listener.onLineReceived(line)
            }
        }
    }

    private companion object {
        const val WRITE_TIMEOUT_MS = 1000

        fun customProbeTable() = UsbSerialProber.getDefaultProbeTable()
            .addProduct(
                UsbSerialIds.QINHENG_VENDOR_ID,
                UsbSerialIds.CH34X_PRODUCT_7522,
                Ch34xSerialDriver::class.java
            )
    }
}
