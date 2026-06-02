package com.universalac.host.serial

import org.junit.Assert.assertTrue
import org.junit.Test

class UsbSerialIdsTest {
    @Test
    fun ch34xProduct7522IsExplicitlySupported() {
        assertTrue(UsbSerialIds.supportsCh34x(0x1A86, 0x7522))
    }
}
