package com.universalac.host.protocol

data class AcState(
    val power: Boolean = true,
    val mode: String = "cool",
    val temp: Int = 26,
    val fan: String = "auto",
    val swingv: String = "off",
    val swingh: String = "off"
)

data class AcRemote(
    val id: String,
    val brandId: String,
    val name: String,
    val minTemp: Int = 17,
    val maxTemp: Int = 30,
    val supportsFan: Boolean = true,
    val supportsSwingV: Boolean = false,
    val supportsSwingH: Boolean = false
)

data class AcBrand(
    val id: String,
    val name: String,
    val remotes: MutableList<AcRemote> = mutableListOf()
)

data class KnownAcDevice(
    val id: String,
    val name: String,
    val brandId: String,
    val remoteId: String,
    val state: AcState = AcState()
)

enum class ResponseKind {
    Ok,
    Error,
    Catalog,
    Other
}
