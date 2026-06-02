package com.universalac.host.ui

import com.universalac.host.protocol.AcRemote
import com.universalac.host.protocol.AcState
import com.universalac.host.protocol.KnownAcDevice
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class HostUiLogicTest {
    private val remote = AcRemote(
        id = "midea_rn02s13",
        brandId = "midea",
        name = "Midea RN02S13",
        minTemp = 17,
        maxTemp = 30,
        supportsFan = true,
        supportsSwingV = true,
        supportsSwingH = true
    )

    private val devices = listOf(
        KnownAcDevice("1", "B客厅", "midea", "midea_rn02s13", AcState(power = true, temp = 26)),
        KnownAcDevice("2", "C卧室", "gree", "gree_yaw1f", AcState(power = false, temp = 24)),
        KnownAcDevice("3", "A书房", "haier", "haier_ac", AcState(power = true, temp = 28))
    )

    @Test
    fun discoveryCommandsMatchQtWizardStates() {
        val commands = HostUiLogic.discoveryCommands(remote)

        assertEquals(
            listOf(
                "AC remote=midea_rn02s13 power=1 mode=cool temp=26 fan=auto swingv=off swingh=off",
                "AC remote=midea_rn02s13 power=1 mode=cool temp=27 fan=auto swingv=off swingh=off"
            ),
            commands
        )
    }

    @Test
    fun detailControlPlanOnlySendsChangedActionsInQtOrder() {
        val current = AcState(power = false, mode = "cool", temp = 26, fan = "auto", swingv = "off", swingh = "off")
        val next = AcState(power = true, mode = "heat", temp = 27, fan = "high", swingv = "auto", swingh = "auto")

        val actions = HostUiLogic.detailActionPlan(current, next).map { it.action }

        assertEquals(listOf("power", "mode", "temp", "fan", "swingv", "swingh"), actions)
    }

    @Test
    fun batchTargetsUseSelectionWhenPresentOtherwiseAllDevices() {
        assertEquals(listOf("2", "3"), HostUiLogic.batchTargets(devices, setOf("2", "3")).map { it.id })
        assertEquals(listOf("1", "2", "3"), HostUiLogic.batchTargets(devices, emptySet()).map { it.id })
    }

    @Test
    fun duplicateCreatesNewIdAndCopyNameWithoutChangingRemoteState() {
        val copy = HostUiLogic.duplicateDevice(devices[0], nowMillis = 12345)

        assertEquals("1_copy_12345", copy.id)
        assertEquals("B客厅 副本", copy.name)
        assertEquals(devices[0].remoteId, copy.remoteId)
        assertEquals(devices[0].state, copy.state)
    }

    @Test
    fun sortDevicesMatchesQtModes() {
        assertEquals(listOf("A书房", "B客厅", "C卧室"), HostUiLogic.sortDevices(devices, SortMode.NAME).map { it.name })
        assertEquals(listOf("C卧室", "A书房", "B客厅"), HostUiLogic.sortDevices(devices, SortMode.BRAND).map { it.name })
        assertEquals(listOf("A书房", "B客厅", "C卧室"), HostUiLogic.sortDevices(devices, SortMode.POWER).map { it.name })
        assertEquals(listOf("C卧室", "B客厅", "A书房"), HostUiLogic.sortDevices(devices, SortMode.TEMP).map { it.name })
    }

    @Test
    fun logBufferFormatsAndClearsLines() {
        val buffer = SerialLogBuffer(clock = { "12:34:56" })
        buffer.append("TX", "PING")
        buffer.append("RX", "OK PONG")

        assertTrue(buffer.text().contains("12:34:56  TX  PING"))
        assertTrue(buffer.text().contains("12:34:56  RX  OK PONG"))

        buffer.clear()
        assertTrue(buffer.text().isEmpty())
        assertFalse(buffer.copyText().contains("PING"))
    }
}
