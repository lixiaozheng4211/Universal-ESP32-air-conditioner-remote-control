package com.universalac.host

import android.app.Activity
import android.app.AlertDialog
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.graphics.Typeface
import android.hardware.usb.UsbManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.Editable
import android.text.TextWatcher
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.CheckBox
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.Spinner
import android.widget.TextView
import android.widget.Toast
import com.universalac.host.protocol.AcBrand
import com.universalac.host.protocol.AcProtocol
import com.universalac.host.protocol.AcRemote
import com.universalac.host.protocol.AcState
import com.universalac.host.protocol.KnownAcDevice
import com.universalac.host.protocol.ResponseKind
import com.universalac.host.serial.UsbDiagnostics
import com.universalac.host.serial.UsbSerialTransport
import com.universalac.host.storage.AcRepository
import com.universalac.host.ui.HostUiLogic
import com.universalac.host.ui.SerialLogBuffer
import com.universalac.host.ui.SortMode
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class MainActivity : Activity(), UsbSerialTransport.Listener {
    private lateinit var repository: AcRepository
    private lateinit var transport: UsbSerialTransport

    private lateinit var statusView: TextView
    private lateinit var selectionView: TextView
    private lateinit var searchEdit: EditText
    private lateinit var sortButton: Button
    private lateinit var deviceList: LinearLayout

    private var logDialogText: TextView? = null
    private val mainHandler = Handler(Looper.getMainLooper())
    private val timestampFormat = SimpleDateFormat("HH:mm:ss", Locale.US)
    private val logBuffer = SerialLogBuffer { timestampFormat.format(Date()) }
    private val catalogLines = mutableListOf<String>()
    private val pendingUpdates = ArrayDeque<PendingUpdate>()

    private var receivingCatalog = false
    private var catalog: List<AcBrand> = emptyList()
    private var devices: MutableList<KnownAcDevice> = mutableListOf()
    private var selectedDeviceId: String? = null
    private val selectedDeviceIds = mutableSetOf<String>()
    private var sortMode = SortMode.NAME
    private var searchText = ""
    private var batchToken = 0
    private var lastTapDeviceId: String? = null
    private var lastTapMillis: Long = 0

    private val usbReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            try {
                when (intent.action) {
                    ACTION_USB_PERMISSION -> {
                        if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                            connectSerial()
                        } else {
                            updateStatus("USB 权限被拒绝")
                        }
                    }

                    UsbManager.ACTION_USB_DEVICE_ATTACHED -> requestUsbPermissionOrConnect()
                    UsbManager.ACTION_USB_DEVICE_DETACHED -> {
                        transport.close()
                        updateStatus("USB 已断开")
                    }
                }
            } catch (error: Exception) {
                updateStatus("USB 事件异常: ${error.javaClass.simpleName}")
                appendLog("ERR", error.message ?: "USB event failed")
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        repository = AcRepository(this)
        transport = UsbSerialTransport(this, this)
        devices = repository.load()
        registerUsbReceiver()
        buildUi()
        refreshDeviceList()
        requestUsbPermissionOrConnect()
    }

    override fun onDestroy() {
        unregisterReceiver(usbReceiver)
        transport.close()
        super.onDestroy()
    }

    override fun onLineReceived(line: String) {
        mainHandler.post {
            appendLog("RX", line)
            handleSerialLine(line)
        }
    }

    override fun onStatusChanged(status: String) {
        mainHandler.post { updateStatus(status) }
    }

    override fun onConnectionLost(reason: String) {
        mainHandler.post {
            updateStatus("连接丢失: $reason")
            Toast.makeText(this, reason, Toast.LENGTH_SHORT).show()
        }
    }

    private fun buildUi() {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(12), dp(12), dp(12), dp(12))
            setBackgroundColor(COLOR_BG)
        }

        statusView = label("未连接", 16, bold = true)
        selectionView = label("未选择", 13)
        root.addView(statusView)
        root.addView(selectionView)

        root.addView(row {
            addView(button("连接") { requestUsbPermissionOrConnect() }, weighted())
            addView(button("断开") { transport.close() }, weighted())
            addView(button("USB诊断") { showUsbDiagnostics() }, weighted())
        })
        root.addView(row {
            addView(button("可选空调") { showCatalogDialog() }, weighted())
            addView(button("添加空调") { showAddWizard() }, weighted())
            addView(button("红外测试") { showIrTestMenu() }, weighted())
            addView(button("串口日志") { showLogDialog() }, weighted())
        })
        root.addView(row {
            addView(button("开启空调") { batchPower(true) }, weighted())
            addView(button("关闭所有") { batchPower(false) }, weighted())
            addView(button("批量设置") { showBatchSetDialog() }, weighted())
            addView(button("停止任务") { stopBatchTask() }, weighted())
        })
        root.addView(row {
            addView(button("重命名") { renameSelectedDevice() }, weighted())
            addView(button("复制") { duplicateSelectedDevice() }, weighted())
            addView(button("删除") { deleteSelectedOrCheckedDevices() }, weighted())
        })

        searchEdit = EditText(this).apply {
            hint = "搜索名称、品牌、遥控器"
            setSingleLine(true)
            setTextColor(COLOR_TEXT)
            setHintTextColor(COLOR_MUTED)
            setBackgroundColor(COLOR_PANEL)
            setPadding(dp(10), dp(8), dp(10), dp(8))
            setOnEditorActionListener { _, _, _ ->
                searchText = text.toString()
                refreshDeviceList()
                false
            }
            setOnFocusChangeListener { _, hasFocus ->
                if (!hasFocus) {
                    searchText = text.toString()
                    refreshDeviceList()
                }
            }
            addTextChangedListener(object : TextWatcher {
                override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) = Unit
                override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {
                    searchText = s?.toString().orEmpty()
                    refreshDeviceList()
                }
                override fun afterTextChanged(s: Editable?) = Unit
            })
        }
        root.addView(searchEdit)
        root.addView(row {
            sortButton = button("排序: ${sortMode.label}") {
                sortMode = sortMode.next()
                sortButton.text = "排序: ${sortMode.label}"
                refreshDeviceList()
            }
            addView(sortButton, weighted())
            addView(button("应用搜索") {
                searchText = searchEdit.text.toString()
                refreshDeviceList()
            }, weighted())
            addView(button("全选") { selectAllVisibleDevices() }, weighted())
            addView(button("清空选择") { clearSelectedDevices() }, weighted())
        })

        root.addView(sectionTitle("已保存空调"))
        deviceList = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        val deviceScroll = ScrollView(this).apply {
            setBackgroundColor(COLOR_PANEL)
            addView(deviceList)
        }
        root.addView(deviceScroll, LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f))

        setContentView(root)
    }

    private fun registerUsbReceiver() {
        val filter = IntentFilter().apply {
            addAction(ACTION_USB_PERMISSION)
            addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
            addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(usbReceiver, filter, Context.RECEIVER_NOT_EXPORTED)
        } else {
            registerReceiver(usbReceiver, filter)
        }
    }

    private fun requestUsbPermissionOrConnect() {
        try {
            val device = transport.firstDevice()
            if (device == null) {
                val summaries = transport.deviceSummaries()
                updateStatus(if (summaries.isEmpty()) "未发现 USB 设备" else "发现 USB 设备但未识别为串口")
                appendLog("USB", UsbDiagnostics.format(summaries).replace("\n", " | "))
                return
            }
            if (transport.hasPermission(device)) {
                connectSerial()
                return
            }
            val flags = PendingIntent.FLAG_UPDATE_CURRENT or
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) PendingIntent.FLAG_MUTABLE else 0
            val pendingIntent = PendingIntent.getBroadcast(
                this,
                0,
                Intent(ACTION_USB_PERMISSION).setPackage(packageName),
                flags
            )
            (getSystemService(Context.USB_SERVICE) as UsbManager).requestPermission(device, pendingIntent)
            updateStatus("等待 USB 授权")
        } catch (error: Exception) {
            updateStatus("USB 授权异常: ${error.javaClass.simpleName}")
            appendLog("ERR", error.message ?: "USB permission request failed")
        }
    }

    private fun connectSerial() {
        if (transport.openFirst()) {
            sendCommand("PING")
            sendCommand("CATALOG")
        }
    }

    private fun handleSerialLine(line: String) {
        when {
            line == "OK CATALOG END" -> {
                receivingCatalog = false
                val parsed = AcProtocol.parseCatalog(catalogLines)
                if (parsed.isNotEmpty()) {
                    catalog = parsed
                    Toast.makeText(this, "目录已更新: ${catalog.sumOf { it.remotes.size }} 个候选", Toast.LENGTH_SHORT).show()
                }
                catalogLines.clear()
                return
            }

            line.startsWith("OK CATALOG ") -> {
                receivingCatalog = true
                catalogLines.clear()
                return
            }

            receivingCatalog && line.startsWith("CAT ") -> {
                catalogLines += line
                return
            }
        }

        when (AcProtocol.classifyResponse(line)) {
            ResponseKind.Ok -> applyPendingUpdate()
            ResponseKind.Error -> discardPendingUpdate(line)
            ResponseKind.Catalog, ResponseKind.Other -> Unit
        }
    }

    private fun sendCommand(command: String, pendingUpdate: PendingUpdate? = null): Boolean {
        if (!transport.isOpen()) {
            Toast.makeText(this, "请先连接 USB 串口", Toast.LENGTH_SHORT).show()
            return false
        }
        if (!transport.sendLine(command)) {
            Toast.makeText(this, "发送失败", Toast.LENGTH_SHORT).show()
            return false
        }
        if (pendingUpdate != null) {
            pendingUpdates.addLast(pendingUpdate)
        }
        appendLog("TX", command)
        return true
    }

    private fun showCatalogDialog() {
        val message = if (catalog.isEmpty()) {
            "目录为空。请先连接设备并发送 CATALOG。"
        } else {
            catalog.joinToString("\n\n") { brand ->
                val remotes = brand.remotes.joinToString("\n") { "  ${it.name} (${it.id})" }
                "${brand.name} (${brand.id}, ${brand.remotes.size}个候选)\n$remotes"
            }
        }
        showTextDialog("可选空调", message)
    }

    private fun showLogDialog() {
        val content = ScrollView(this).apply {
            setBackgroundColor(COLOR_LOG)
            logDialogText = label(logBuffer.text(), 12).apply {
                typeface = Typeface.MONOSPACE
                setPadding(dp(10), dp(10), dp(10), dp(10))
            }
            addView(logDialogText)
        }
        AlertDialog.Builder(this)
            .setTitle("串口日志")
            .setView(content)
            .setPositiveButton("复制") { _, _ ->
                (getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager)
                    .setPrimaryClip(ClipData.newPlainText("serial-log", logBuffer.copyText()))
            }
            .setNeutralButton("清空") { _, _ ->
                logBuffer.clear()
                logDialogText?.text = ""
            }
            .setNegativeButton("关闭", null)
            .show()
    }

    private fun showUsbDiagnostics() {
        showTextDialog("USB 诊断", UsbDiagnostics.format(transport.deviceSummaries()))
    }

    private fun showIrTestMenu() {
        AlertDialog.Builder(this)
            .setTitle("红外测试")
            .setItems(arrayOf("38K 测试", "40K 测试")) { _, which ->
                sendCommand("IRTEST freq=${if (which == 0) 38000 else 40000} mode=nec count=4 duty=33")
            }
            .show()
    }

    private fun showAddWizard() {
        if (!transport.isOpen()) {
            Toast.makeText(this, "请先连接 ESP32 串口", Toast.LENGTH_SHORT).show()
            return
        }
        if (catalog.isEmpty()) {
            sendCommand("CATALOG")
            Toast.makeText(this, "目录为空，已请求 CATALOG，请稍后再添加", Toast.LENGTH_SHORT).show()
            return
        }
        val labels = catalog.map { "${it.name} (${it.id}, ${it.remotes.size}个候选)" }.toTypedArray()
        AlertDialog.Builder(this)
            .setTitle("选择品牌")
            .setItems(labels) { _, which -> runDiscoveryCandidate(catalog[which], 0) }
            .show()
    }

    private fun runDiscoveryCandidate(brand: AcBrand, index: Int) {
        if (index >= brand.remotes.size) {
            Toast.makeText(this, "该品牌候选遥控器已尝试完毕", Toast.LENGTH_LONG).show()
            return
        }
        val remote = brand.remotes[index]
        val states = HostUiLogic.discoveryStates(remote)
        if (!sendCommand(AcProtocol.buildAcCommand(remote.id, states[0]))) {
            return
        }
        AlertDialog.Builder(this)
            .setTitle("开机响应")
            .setMessage("${remote.name} 是否响应开机？")
            .setPositiveButton("是") { _, _ ->
                if (sendCommand(AcProtocol.buildAcCommand(remote.id, states[1]))) {
                    confirmDiscoveryTemp(brand, remote, states[0], index)
                }
            }
            .setNegativeButton("否") { _, _ -> runDiscoveryCandidate(brand, index + 1) }
            .show()
    }

    private fun confirmDiscoveryTemp(brand: AcBrand, remote: AcRemote, savedState: AcState, index: Int) {
        AlertDialog.Builder(this)
            .setTitle("温度响应")
            .setMessage("${remote.name} 温度是否可调？")
            .setPositiveButton("是") { _, _ -> askDeviceNameAndSave(brand, remote, savedState) }
            .setNegativeButton("否") { _, _ -> runDiscoveryCandidate(brand, index + 1) }
            .show()
    }

    private fun askDeviceNameAndSave(brand: AcBrand, remote: AcRemote, state: AcState) {
        val input = EditText(this).apply {
            setText(AcProtocol.defaultDeviceName(remote))
            selectAll()
            setTextColor(0xFF111827.toInt())
        }
        AlertDialog.Builder(this)
            .setTitle("保存空调")
            .setView(input)
            .setPositiveButton("保存") { _, _ ->
                val device = KnownAcDevice(
                    id = "${remote.id}_${System.currentTimeMillis()}",
                    name = input.text.toString().trim().ifEmpty { AcProtocol.defaultDeviceName(remote) },
                    brandId = brand.id,
                    remoteId = remote.id,
                    state = state
                )
                devices += device
                selectedDeviceId = device.id
                repository.save(devices)
                refreshDeviceList()
            }
            .setNegativeButton("取消", null)
            .show()
    }

    private fun openDeviceControl(deviceId: String) {
        val index = devices.indexOfFirst { it.id == deviceId }
        if (index < 0) return
        val device = devices[index]
        var dialogState = device.state
        val remote = AcProtocol.findRemote(catalog, device.remoteId)
        lateinit var status: TextView

        val temp = Spinner(this).also { spinner ->
            val min = remote?.minTemp ?: 16
            val max = remote?.maxTemp ?: 30
            setSpinnerItems(spinner, (min..max).map { it.toString() })
            selectSpinnerValue(spinner, dialogState.temp.toString())
        }
        val mode = Spinner(this).also {
            setSpinnerItems(it, listOf("auto", "cool", "heat", "dry", "fan"))
            selectSpinnerValue(it, dialogState.mode)
        }
        val fan = Spinner(this).also {
            setSpinnerItems(it, listOf("auto", "low", "med", "high", "max"))
            it.isEnabled = remote?.supportsFan != false
            selectSpinnerValue(it, dialogState.fan)
        }
        val swingV = CheckBox(this).apply {
            text = "上下风"
            setTextColor(COLOR_TEXT)
            isEnabled = remote?.supportsSwingV != false
            isChecked = dialogState.swingv == "auto"
        }
        val swingH = CheckBox(this).apply {
            text = "左右风"
            setTextColor(COLOR_TEXT)
            isEnabled = remote?.supportsSwingH != false
            isChecked = dialogState.swingh == "auto"
        }

        fun stateFromDialog(power: Boolean = dialogState.power) = AcState(
            power = power,
            mode = mode.selectedItem.toString(),
            temp = temp.selectedItem.toString().toIntOrNull() ?: dialogState.temp,
            fan = if (remote?.supportsFan == false) "auto" else fan.selectedItem.toString(),
            swingv = if (swingV.isEnabled && swingV.isChecked) "auto" else "off",
            swingh = if (swingH.isEnabled && swingH.isChecked) "auto" else "off"
        )

        fun sendAction(action: String, state: AcState): Boolean {
            val ok = sendCommand(AcProtocol.buildAcActionCommand(device.remoteId, action, state), PendingUpdate(device.id, state))
            if (ok) {
                dialogState = state
            }
            return ok
        }

        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(12), dp(8), dp(12), dp(8))
            setBackgroundColor(COLOR_BG)
            addView(label(device.name, 18, bold = true))
            addView(label("${device.brandId} / ${remote?.name ?: device.remoteId}", 13))
            addView(row {
                addView(button("开机") {
                    if (sendAction("power", dialogState.copy(power = true))) status.text = "已开机"
                }, weighted())
                addView(button("关机") {
                    if (sendAction("power", dialogState.copy(power = false))) status.text = "已关机"
                }, weighted())
            })
            addView(labeledSpinner("温度", temp))
            addView(labeledSpinner("模式", mode))
            addView(labeledSpinner("风速", fan))
            addView(swingV)
            addView(swingH)
            addView(button("发送控制") {
                val steps = HostUiLogic.detailActionPlan(dialogState, stateFromDialog())
                if (steps.isEmpty()) {
                    status.text = "无变化"
                    return@button
                }
                var sent = 0
                for (step in steps) {
                    if (!sendAction(step.action, step.state)) {
                        status.text = "发送失败"
                        return@button
                    }
                    sent += 1
                }
                status.text = "已发送 $sent 项"
            })
            status = label("", 13)
            addView(status)
        }

        AlertDialog.Builder(this)
            .setTitle("空调控制")
            .setView(panel)
            .setPositiveButton("关闭", null)
            .show()
    }

    private fun showBatchSetDialog() {
        val targets = devices.filter { it.id in selectedDeviceIds }
        if (targets.isEmpty()) {
            Toast.makeText(this, "请先勾选要批量设置的空调", Toast.LENGTH_SHORT).show()
            return
        }
        val temp = Spinner(this).also {
            setSpinnerItems(it, (16..32).map { value -> value.toString() })
            selectSpinnerValue(it, "26")
        }
        val mode = Spinner(this).also { setSpinnerItems(it, listOf("auto", "cool", "heat", "dry", "fan")) }
        val fan = Spinner(this).also { setSpinnerItems(it, listOf("auto", "low", "med", "high", "max")) }
        val swingV = CheckBox(this).apply { text = "上下风"; setTextColor(COLOR_TEXT) }
        val swingH = CheckBox(this).apply { text = "左右风"; setTextColor(COLOR_TEXT) }
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(COLOR_BG)
            addView(labeledSpinner("温度", temp))
            addView(labeledSpinner("模式", mode))
            addView(labeledSpinner("风速", fan))
            addView(swingV)
            addView(swingH)
        }
        AlertDialog.Builder(this)
            .setTitle("批量设置")
            .setView(panel)
            .setPositiveButton("发送") { _, _ ->
                val state = AcState(
                    power = true,
                    mode = mode.selectedItem.toString(),
                    temp = temp.selectedItem.toString().toIntOrNull() ?: 26,
                    fan = fan.selectedItem.toString(),
                    swingv = if (swingV.isChecked) "auto" else "off",
                    swingh = if (swingH.isChecked) "auto" else "off"
                )
                sendBatch(targets.map { it to clampStateToRemote(it.remoteId, state) }, useActionPower = false)
            }
            .setNegativeButton("取消", null)
            .show()
    }

    private fun batchPower(power: Boolean) {
        val targets = HostUiLogic.batchTargets(devices, selectedDeviceIds)
        if (targets.isEmpty()) {
            Toast.makeText(this, "没有已保存空调", Toast.LENGTH_SHORT).show()
            return
        }
        sendBatch(targets.map { it to it.state.copy(power = power) }, useActionPower = true)
    }

    private fun sendBatch(targets: List<Pair<KnownAcDevice, AcState>>, useActionPower: Boolean, index: Int = 0, token: Int = ++batchToken) {
        if (token != batchToken || index >= targets.size) {
            return
        }
        val (device, state) = targets[index]
        val command = if (useActionPower) {
            AcProtocol.buildAcActionCommand(device.remoteId, "power", state)
        } else {
            AcProtocol.buildAcCommand(device.remoteId, state)
        }
        if (!sendCommand(command, PendingUpdate(device.id, state))) {
            return
        }
        mainHandler.postDelayed({ sendBatch(targets, useActionPower, index + 1, token) }, BATCH_DELAY_MS)
    }

    private fun stopBatchTask() {
        ++batchToken
        Toast.makeText(this, "已停止后续批量任务", Toast.LENGTH_SHORT).show()
    }

    private fun renameSelectedDevice() {
        val device = selectedDevice() ?: return toastNoSelected()
        val input = EditText(this).apply {
            setText(device.name)
            selectAll()
            setTextColor(0xFF111827.toInt())
        }
        AlertDialog.Builder(this)
            .setTitle("重命名空调")
            .setView(input)
            .setPositiveButton("保存") { _, _ ->
                val index = devices.indexOfFirst { it.id == device.id }
                if (index >= 0) {
                    devices[index] = devices[index].copy(name = input.text.toString().trim().ifEmpty { device.name })
                    repository.save(devices)
                    refreshDeviceList()
                }
            }
            .setNegativeButton("取消", null)
            .show()
    }

    private fun duplicateSelectedDevice() {
        val device = selectedDevice() ?: return toastNoSelected()
        devices += HostUiLogic.duplicateDevice(device, System.currentTimeMillis())
        repository.save(devices)
        refreshDeviceList()
    }

    private fun deleteSelectedOrCheckedDevices() {
        val ids = if (selectedDeviceIds.isNotEmpty()) selectedDeviceIds.toSet() else setOfNotNull(selectedDeviceId)
        if (ids.isEmpty()) return toastNoSelected()
        AlertDialog.Builder(this)
            .setTitle("删除空调")
            .setMessage("确定删除 ${ids.size} 台空调？")
            .setPositiveButton("删除") { _, _ ->
                devices.removeAll { it.id in ids }
                selectedDeviceIds.removeAll(ids)
                if (selectedDeviceId in ids) selectedDeviceId = devices.firstOrNull()?.id
                repository.save(devices)
                refreshDeviceList()
            }
            .setNegativeButton("取消", null)
            .show()
    }

    private fun applyPendingUpdate() {
        if (pendingUpdates.isEmpty()) return
        val update = pendingUpdates.removeFirst()
        val index = devices.indexOfFirst { it.id == update.deviceId }
        if (index >= 0) {
            devices[index] = devices[index].copy(state = update.state)
            repository.save(devices)
            refreshDeviceList()
        }
    }

    private fun discardPendingUpdate(reason: String) {
        if (pendingUpdates.isNotEmpty()) pendingUpdates.removeFirst()
        Toast.makeText(this, reason, Toast.LENGTH_SHORT).show()
    }

    private fun selectAllVisibleDevices() {
        selectedDeviceIds.clear()
        visibleDevices().forEach { selectedDeviceIds += it.id }
        refreshDeviceList()
    }

    private fun clearSelectedDevices() {
        selectedDeviceIds.clear()
        refreshDeviceList()
    }

    private fun refreshDeviceList() {
        deviceList.removeAllViews()
        if (selectedDeviceId == null && devices.isNotEmpty()) selectedDeviceId = devices.first().id
        val visible = visibleDevices()
        selectionView.text = if (selectedDeviceIds.isEmpty()) {
            selectedDevice()?.let { "当前: ${it.name}" } ?: "未选择"
        } else {
            "已选 ${selectedDeviceIds.size} 台"
        }
        if (visible.isEmpty()) {
            deviceList.addView(label("没有匹配的已保存空调。", 13).apply { setPadding(dp(10), dp(12), dp(10), dp(12)) })
            return
        }
        visible.forEach { device ->
            deviceList.addView(createDeviceRow(device))
        }
    }

    private fun createDeviceRow(device: KnownAcDevice): LinearLayout {
        val row = row {
            gravity = Gravity.CENTER_VERTICAL
            setBackgroundColor(if (device.id == selectedDeviceId) COLOR_SELECTED else COLOR_PANEL)
            setPadding(dp(6), dp(6), dp(6), dp(6))
            val check = CheckBox(this@MainActivity).apply {
                isChecked = device.id in selectedDeviceIds
                setTextColor(COLOR_TEXT)
                setOnCheckedChangeListener { _, checked ->
                    if (checked) selectedDeviceIds += device.id else selectedDeviceIds -= device.id
                    selectionView.text = if (selectedDeviceIds.isEmpty()) {
                        selectedDevice()?.let { "当前: ${it.name}" } ?: "未选择"
                    } else {
                        "已选 ${selectedDeviceIds.size} 台"
                    }
                }
            }
            addView(check)
            addView(label("${device.name}\n${device.brandId} / ${device.remoteId}\n${stateText(device.state)}", 13), weighted())
            addView(button("详情") { openDeviceControl(device.id) })
        }
        row.setOnClickListener { handleDeviceTap(device.id) }
        return row
    }

    private fun handleDeviceTap(deviceId: String) {
        val now = System.currentTimeMillis()
        if (lastTapDeviceId == deviceId && now - lastTapMillis < DOUBLE_TAP_MS) {
            openDeviceControl(deviceId)
        } else {
            selectedDeviceId = deviceId
            refreshDeviceList()
        }
        lastTapDeviceId = deviceId
        lastTapMillis = now
    }

    private fun visibleDevices(): List<KnownAcDevice> {
        return HostUiLogic.sortDevices(HostUiLogic.filterDevices(devices, searchText), sortMode)
    }

    private fun selectedDevice(): KnownAcDevice? = devices.firstOrNull { it.id == selectedDeviceId }

    private fun clampStateToRemote(remoteId: String, state: AcState): AcState {
        val remote = AcProtocol.findRemote(catalog, remoteId) ?: return state
        return state.copy(
            temp = state.temp.coerceIn(remote.minTemp, remote.maxTemp),
            fan = if (remote.supportsFan) state.fan else "auto",
            swingv = if (remote.supportsSwingV) state.swingv else "off",
            swingh = if (remote.supportsSwingH) state.swingh else "off"
        )
    }

    private fun appendLog(direction: String, message: String) {
        logBuffer.append(direction, message)
        logDialogText?.text = logBuffer.text()
    }

    private fun updateStatus(status: String) {
        statusView.text = status
    }

    private fun stateText(state: AcState): String {
        val power = if (state.power) "开" else "关"
        return "$power  ${state.mode}  ${state.temp}C  ${state.fan}  上下:${state.swingv}  左右:${state.swingh}"
    }

    private fun showTextDialog(title: String, message: String) {
        AlertDialog.Builder(this).setTitle(title).setMessage(message).setPositiveButton("确定", null).show()
    }

    private fun toastNoSelected() {
        Toast.makeText(this, "请先选择一个空调", Toast.LENGTH_SHORT).show()
    }

    private fun sectionTitle(text: String): TextView = label(text, 15, bold = true).apply {
        setPadding(0, dp(10), 0, dp(4))
    }

    private fun label(text: String, sizeSp: Int, bold: Boolean = false): TextView {
        return TextView(this).apply {
            this.text = text
            textSize = sizeSp.toFloat()
            setTextColor(COLOR_TEXT)
            if (bold) typeface = Typeface.DEFAULT_BOLD
        }
    }

    private fun button(text: String, action: () -> Unit): Button {
        return Button(this).apply {
            this.text = text
            setTextColor(COLOR_TEXT)
            setBackgroundColor(COLOR_BUTTON)
            setOnClickListener { action() }
        }
    }

    private fun row(builder: LinearLayout.() -> Unit): LinearLayout {
        return LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            setPadding(0, dp(4), 0, dp(4))
            builder()
        }
    }

    private fun labeledSpinner(label: String, spinner: Spinner): LinearLayout {
        return row {
            addView(label(label, 13), LinearLayout.LayoutParams(dp(74), LinearLayout.LayoutParams.WRAP_CONTENT))
            addView(spinner, weighted())
        }
    }

    private fun setSpinnerItems(spinner: Spinner, values: List<String>) {
        spinner.adapter = object : ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, values) {
            override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
                return (super.getView(position, convertView, parent) as TextView).apply {
                    setTextColor(COLOR_TEXT)
                    setBackgroundColor(COLOR_PANEL)
                    textSize = 14f
                    setPadding(dp(10), dp(8), dp(10), dp(8))
                }
            }

            override fun getDropDownView(position: Int, convertView: View?, parent: ViewGroup): View {
                return (super.getDropDownView(position, convertView, parent) as TextView).apply {
                    setTextColor(0xFF111827.toInt())
                    setBackgroundColor(0xFFFFFFFF.toInt())
                    textSize = 14f
                    setPadding(dp(12), dp(10), dp(12), dp(10))
                }
            }
        }
    }

    private fun selectSpinnerValue(spinner: Spinner, value: String) {
        val adapter = spinner.adapter ?: return
        for (i in 0 until adapter.count) {
            if (adapter.getItem(i).toString() == value) {
                spinner.setSelection(i)
                return
            }
        }
    }

    private fun weighted() = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()

    private data class PendingUpdate(val deviceId: String, val state: AcState)

    private companion object {
        const val ACTION_USB_PERMISSION = "com.universalac.host.USB_PERMISSION"
        const val BATCH_DELAY_MS = 1500L
        const val DOUBLE_TAP_MS = 450L
        val COLOR_BG = 0xFF17202A.toInt()
        val COLOR_PANEL = 0xFF0F172A.toInt()
        val COLOR_LOG = 0xFF0B1220.toInt()
        val COLOR_SELECTED = 0xFF1E3A5F.toInt()
        val COLOR_TEXT = 0xFFE5EDF6.toInt()
        val COLOR_MUTED = 0xFF94A3B8.toInt()
        val COLOR_BUTTON = 0xFF334155.toInt()
    }
}
