package com.universalac.host.storage

import android.content.Context
import com.universalac.host.protocol.AcState
import com.universalac.host.protocol.KnownAcDevice
import org.json.JSONArray
import org.json.JSONObject

class AcRepository(context: Context) {
    private val preferences = context.getSharedPreferences("known_ac_devices", Context.MODE_PRIVATE)

    fun load(): MutableList<KnownAcDevice> {
        val raw = preferences.getString(KEY_DEVICES, null) ?: return mutableListOf()
        val array = runCatching { JSONArray(raw) }.getOrNull() ?: return mutableListOf()
        val devices = mutableListOf<KnownAcDevice>()
        for (i in 0 until array.length()) {
            val obj = array.optJSONObject(i) ?: continue
            val id = obj.optString("id")
            val remoteId = obj.optString("remoteId")
            if (id.isEmpty() || remoteId.isEmpty()) {
                continue
            }
            devices += KnownAcDevice(
                id = id,
                name = obj.optString("name", remoteId),
                brandId = obj.optString("brandId"),
                remoteId = remoteId,
                state = stateFromJson(obj.optJSONObject("state"))
            )
        }
        return devices
    }

    fun save(devices: List<KnownAcDevice>) {
        val array = JSONArray()
        devices.forEach { device ->
            array.put(
                JSONObject()
                    .put("id", device.id)
                    .put("name", device.name)
                    .put("brandId", device.brandId)
                    .put("remoteId", device.remoteId)
                    .put("state", stateToJson(device.state))
            )
        }
        preferences.edit().putString(KEY_DEVICES, array.toString()).apply()
    }

    private fun stateToJson(state: AcState): JSONObject {
        return JSONObject()
            .put("power", state.power)
            .put("mode", state.mode)
            .put("temp", state.temp)
            .put("fan", state.fan)
            .put("swingv", state.swingv)
            .put("swingh", state.swingh)
    }

    private fun stateFromJson(obj: JSONObject?): AcState {
        if (obj == null) {
            return AcState()
        }
        return AcState(
            power = obj.optBoolean("power", true),
            mode = obj.optString("mode", "cool"),
            temp = obj.optInt("temp", 26),
            fan = obj.optString("fan", "auto"),
            swingv = obj.optString("swingv", "off"),
            swingh = obj.optString("swingh", "off")
        )
    }

    private companion object {
        const val KEY_DEVICES = "devices_json"
    }
}
