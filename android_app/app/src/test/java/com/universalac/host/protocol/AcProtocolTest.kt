package com.universalac.host.protocol

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class AcProtocolTest {
    @Test
    fun parseCatalogLinesBuildsBrandAndRemoteTree() {
        val catalog = AcProtocol.parseCatalog(
            listOf(
                "OK CATALOG remotes=2",
                "CAT BRAND id=midea name=\"Midea\" first_child=16 next_sibling=1",
                "CAT REMOTE id=midea_rn02s13 brand=midea name=\"Midea RN02S13\" temp=17-30 fan=1 swingv=1 swingh=1 driver=\"Midea RN02S13\" next_sibling=-1",
                "OK CATALOG END"
            )
        )

        assertEquals(1, catalog.size)
        assertEquals("midea", catalog[0].id)
        assertEquals("美的", catalog[0].name)
        assertEquals(1, catalog[0].remotes.size)

        val remote = catalog[0].remotes[0]
        assertEquals("midea_rn02s13", remote.id)
        assertEquals("midea", remote.brandId)
        assertEquals("Midea RN02S13", remote.name)
        assertEquals(17, remote.minTemp)
        assertEquals(30, remote.maxTemp)
        assertTrue(remote.supportsFan)
        assertTrue(remote.supportsSwingV)
        assertTrue(remote.supportsSwingH)
    }

    @Test
    fun buildActionCommandKeepsFullStateFields() {
        val state = AcState(
            power = true,
            mode = "cool",
            temp = 27,
            fan = "high",
            swingv = "auto",
            swingh = "off"
        )

        val command = AcProtocol.buildAcActionCommand("midea_rn02s13", "temp", state)

        assertEquals(
            "AC remote=midea_rn02s13 action=temp power=1 mode=cool temp=27 fan=high swingv=auto swingh=off",
            command
        )
    }

    @Test
    fun classifyResponseSeparatesOkErrAndCatalogLines() {
        assertEquals(ResponseKind.Ok, AcProtocol.classifyResponse("OK PONG"))
        assertEquals(ResponseKind.Error, AcProtocol.classifyResponse("ERR BAD_TEMP temperature out of remote range"))
        assertEquals(ResponseKind.Catalog, AcProtocol.classifyResponse("CAT REMOTE id=x brand=b name=\"X\" temp=16-30"))
        assertEquals(ResponseKind.Other, AcProtocol.classifyResponse("debug text"))
    }

    @Test
    fun parseCatalogUsesDefaultsForMissingCapabilities() {
        val catalog = AcProtocol.parseCatalog(
            listOf("CAT REMOTE id=basic brand=demo name=\"Basic\" temp=18-28")
        )

        val remote = catalog.single().remotes.single()
        assertEquals("demo", catalog.single().id)
        assertEquals("demo", catalog.single().name)
        assertEquals(18, remote.minTemp)
        assertEquals(28, remote.maxTemp)
        assertTrue(remote.supportsFan)
        assertFalse(remote.supportsSwingV)
        assertFalse(remote.supportsSwingH)
    }
}
