package com.universalac.host.ui

import com.universalac.host.protocol.AcProtocol
import com.universalac.host.protocol.AcRemote
import com.universalac.host.protocol.AcState
import com.universalac.host.protocol.KnownAcDevice

enum class SortMode(val label: String) {
    NAME("名称"),
    BRAND("品牌"),
    POWER("状态"),
    TEMP("温度");

    fun next(): SortMode = entries[(ordinal + 1) % entries.size]
}

data class ActionStep(
    val action: String,
    val state: AcState
)

object HostUiLogic {
    fun discoveryStates(remote: AcRemote): List<AcState> {
        val onState = AcState(
            power = true,
            mode = "cool",
            temp = 26,
            fan = "auto",
            swingv = "off",
            swingh = if (remote.supportsSwingH) "off" else "off"
        )
        val tempState = onState.copy(temp = minOf(27, remote.maxTemp))
        return listOf(onState, tempState)
    }

    fun discoveryCommands(remote: AcRemote): List<String> {
        return discoveryStates(remote).map { AcProtocol.buildAcCommand(remote.id, it) }
    }

    fun detailActionPlan(current: AcState, next: AcState): List<ActionStep> {
        val steps = mutableListOf<ActionStep>()
        var working = current
        fun addIfChanged(action: String, changed: Boolean, update: (AcState) -> AcState) {
            if (!changed) {
                return
            }
            working = update(working)
            steps += ActionStep(action, working)
        }
        addIfChanged("power", next.power != working.power) { it.copy(power = next.power) }
        addIfChanged("mode", next.mode != working.mode) { it.copy(mode = next.mode) }
        addIfChanged("temp", next.temp != working.temp) { it.copy(temp = next.temp) }
        addIfChanged("fan", next.fan != working.fan) { it.copy(fan = next.fan) }
        addIfChanged("swingv", next.swingv != working.swingv) { it.copy(swingv = next.swingv) }
        addIfChanged("swingh", next.swingh != working.swingh) { it.copy(swingh = next.swingh) }
        return steps
    }

    fun batchTargets(devices: List<KnownAcDevice>, selectedIds: Set<String>): List<KnownAcDevice> {
        if (selectedIds.isEmpty()) {
            return devices
        }
        return devices.filter { it.id in selectedIds }
    }

    fun duplicateDevice(device: KnownAcDevice, nowMillis: Long): KnownAcDevice {
        return device.copy(
            id = "${device.id}_copy_$nowMillis",
            name = if (device.name.isBlank()) "空调 副本" else "${device.name} 副本"
        )
    }

    fun sortDevices(devices: List<KnownAcDevice>, sortMode: SortMode): List<KnownAcDevice> {
        return when (sortMode) {
            SortMode.NAME -> devices.sortedWith(compareBy<KnownAcDevice> { it.name }.thenBy { it.id })
            SortMode.BRAND -> devices.sortedWith(compareBy<KnownAcDevice> { it.brandId }.thenBy { it.name })
            SortMode.POWER -> devices.sortedWith(compareByDescending<KnownAcDevice> { it.state.power }.thenBy { it.name })
            SortMode.TEMP -> devices.sortedWith(compareBy<KnownAcDevice> { it.state.temp }.thenBy { it.name })
        }
    }

    fun filterDevices(devices: List<KnownAcDevice>, searchText: String): List<KnownAcDevice> {
        val needle = searchText.trim()
        if (needle.isEmpty()) {
            return devices
        }
        return devices.filter { device ->
            device.name.contains(needle, ignoreCase = true) ||
                device.brandId.contains(needle, ignoreCase = true) ||
                device.remoteId.contains(needle, ignoreCase = true)
        }
    }
}

class SerialLogBuffer(
    private val maxLines: Int = 400,
    private val clock: () -> String
) {
    private val lines = ArrayDeque<String>()

    fun append(direction: String, message: String) {
        lines.addLast("${clock()}  $direction  $message")
        while (lines.size > maxLines) {
            lines.removeFirst()
        }
    }

    fun clear() {
        lines.clear()
    }

    fun text(): String = lines.joinToString(separator = "\n")

    fun copyText(): String = text()
}
