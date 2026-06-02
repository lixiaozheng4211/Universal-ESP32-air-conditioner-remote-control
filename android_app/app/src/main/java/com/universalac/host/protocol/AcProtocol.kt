package com.universalac.host.protocol

object AcProtocol {
    private val fieldPatternCache = mutableMapOf<String, Regex>()

    private val localizedBrandNames = mapOf(
        "midea" to "美的",
        "gree" to "格力",
        "haier" to "海尔",
        "tcl" to "TCL",
        "kelon" to "科龙",
        "daikin" to "大金",
        "hitachi" to "日立",
        "panasonic" to "松下",
        "mitsubishi_electric" to "三菱电机",
        "mitsubishi_heavy" to "三菱重工",
        "toshiba" to "东芝",
        "sharp" to "夏普",
        "samsung" to "三星",
        "lg" to "LG",
        "carrier" to "开利",
        "aux" to "奥克斯",
        "bosch" to "博世",
        "delonghi" to "德龙",
        "fujitsu" to "富士通",
        "sanyo" to "三洋",
        "whirlpool" to "惠而浦"
    )

    fun parseCatalog(lines: List<String>): List<AcBrand> {
        val catalog = mutableListOf<AcBrand>()
        val brandIndexes = mutableMapOf<String, Int>()

        fun ensureBrand(id: String, rawName: String = id): AcBrand {
            val existing = brandIndexes[id]
            if (existing != null) {
                return catalog[existing]
            }
            val brand = AcBrand(id = id, name = localizedBrandName(id, rawName))
            brandIndexes[id] = catalog.size
            catalog += brand
            return brand
        }

        for (line in lines) {
            when {
                line.startsWith("CAT BRAND ") -> {
                    val id = catalogField(line, "id")
                    if (id.isNotEmpty()) {
                        ensureBrand(id, catalogField(line, "name"))
                    }
                }

                line.startsWith("CAT REMOTE ") -> {
                    val id = catalogField(line, "id")
                    val brandId = catalogField(line, "brand")
                    if (id.isNotEmpty() && brandId.isNotEmpty()) {
                        val (minTemp, maxTemp) = parseTempRange(line)
                        val remote = AcRemote(
                            id = id,
                            brandId = brandId,
                            name = catalogField(line, "name").ifEmpty { id },
                            minTemp = minTemp,
                            maxTemp = maxTemp,
                            supportsFan = catalogBool(line, "fan", defaultValue = true),
                            supportsSwingV = catalogBool(line, "swingv"),
                            supportsSwingH = catalogBool(line, "swingh")
                        )
                        ensureBrand(brandId).remotes += remote
                    }
                }
            }
        }

        return catalog
    }

    fun buildAcCommand(remoteId: String, state: AcState): String {
        return "AC remote=$remoteId power=${state.power.asInt()} mode=${state.mode} temp=${state.temp} " +
            "fan=${state.fan} swingv=${state.swingv} swingh=${state.swingh}"
    }

    fun buildAcActionCommand(remoteId: String, action: String, state: AcState): String {
        return "AC remote=$remoteId action=$action power=${state.power.asInt()} mode=${state.mode} " +
            "temp=${state.temp} fan=${state.fan} swingv=${state.swingv} swingh=${state.swingh}"
    }

    fun classifyResponse(line: String): ResponseKind {
        return when {
            line.startsWith("OK ") || line == "OK" -> ResponseKind.Ok
            line.startsWith("ERR ") -> ResponseKind.Error
            line.startsWith("CAT ") -> ResponseKind.Catalog
            else -> ResponseKind.Other
        }
    }

    fun findRemote(catalog: List<AcBrand>, remoteId: String): AcRemote? {
        return catalog.firstNotNullOfOrNull { brand ->
            brand.remotes.firstOrNull { it.id == remoteId }
        }
    }

    fun defaultDeviceName(remote: AcRemote): String = "${remote.name} 空调"

    private fun catalogField(line: String, key: String): String {
        val regex = fieldPatternCache.getOrPut(key) {
            Regex("(?:^|\\s)$key=(\"[^\"]*\"|\\S+)")
        }
        val raw = regex.find(line)?.groupValues?.getOrNull(1) ?: return ""
        return if (raw.length >= 2 && raw.startsWith('"') && raw.endsWith('"')) {
            raw.substring(1, raw.length - 1)
        } else {
            raw
        }
    }

    private fun catalogBool(line: String, key: String, defaultValue: Boolean = false): Boolean {
        val value = catalogField(line, key)
        if (value.isEmpty()) {
            return defaultValue
        }
        return value == "1" || value == "true" || value == "on"
    }

    private fun parseTempRange(line: String): Pair<Int, Int> {
        val parts = catalogField(line, "temp").split("-")
        if (parts.size != 2) {
            return 16 to 30
        }
        val min = parts[0].toIntOrNull()
        val max = parts[1].toIntOrNull()
        return if (min != null && max != null && min <= max) {
            min to max
        } else {
            16 to 30
        }
    }

    private fun localizedBrandName(id: String, fallback: String): String {
        return localizedBrandNames[id] ?: fallback.ifEmpty { id }
    }

    private fun Boolean.asInt(): Int = if (this) 1 else 0
}
