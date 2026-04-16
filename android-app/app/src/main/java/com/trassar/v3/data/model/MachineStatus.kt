package com.trassar.v3.data.model

import kotlinx.serialization.Serializable

@Serializable
data class MachineStatus(
    // Stan maszyny
    val state: String = "idle",          // idle / painting / paused / stopped
    val mode: String = "auto",           // auto / semi / manual / demo
    val screen: Int = 0,
    val menuIndex: Int = 0,

    // Wzorzec
    val pattern: String = "P-1a",
    val patternName: String = "",
    val patternIdx: Int = 0,
    val reversed: Boolean = false,
    val gapStart: Boolean = false,
    val customValid: Boolean = false,
    val activeSlot: Int = 0,
    val slotsValid: List<Boolean> = emptyList(),
    val semiLineComplete: Boolean = false,

    // Predkosc / dystans
    val speed: String = "0.0",
    val distance: String = "0.0",
    val area: String = "0.00",
    val elapsed: Long = 0,

    // System
    val firmware: String = "",
    val freeHeap: Long = 0,
    val minFreeHeap: Long = 0,
    val uptime: Long = 0,
    val clients: Int = 0,

    // Kalibracja
    val calibrated: Boolean = false,
    val ppm: String = "0.0",
    val calibrating: Boolean = false,
    val calPulses: String = "0",

    // Alarmy predkosci
    val maxSpeed: String = "15.0",
    val minSpeed: String = "3.0",
    val overspeed: Boolean = false,
    val lowSpeed: Boolean = false,

    // Pistolety
    val guns: List<Boolean> = List(6) { false },

    // Auto-pauza
    val autoPaused: Boolean = false,
    val autoResumeEnabled: Boolean = false,
    val semiSegment: Int = 0,

    // Smart switch
    val smartSwitch: Boolean = false,
    val patternPending: Boolean = false,
    val pendingPattern: String? = null,

    // GPS
    val gpsFix: Boolean = false,
    val gpsLat: String = "0.0",
    val gpsLng: String = "0.0",
    val gpsSat: Int = 0,
    val gpsSpeed: String = "0.0",
    val gpsHdop: String = "0.0",
    val gpxRec: Boolean = false,
    val gpxPts: Int = 0,

    // Farba
    val paintLevelL: String = "0.0",
    val paintLevelPct: Int = 0,

    // Anomalie pistoletow
    val gunAnomalyDetected: Boolean = false,
    val gunAnomaly: List<Boolean> = List(6) { false },
) {
    val speedFloat: Float get() = speed.toFloatOrNull() ?: 0f
    val distanceFloat: Float get() = distance.toFloatOrNull() ?: 0f
    val areaFloat: Float get() = area.toFloatOrNull() ?: 0f
    val gpsLatDouble: Double get() = gpsLat.toDoubleOrNull() ?: 0.0
    val gpsLngDouble: Double get() = gpsLng.toDoubleOrNull() ?: 0.0

    val uptimeFormatted: String get() {
        val h = uptime / 3600
        val m = (uptime % 3600) / 60
        val s = uptime % 60
        return if (h > 0) "${h}h ${m}m ${s}s" else "${m}m ${s}s"
    }

    val elapsedFormatted: String get() {
        val h = elapsed / 3600
        val m = (elapsed % 3600) / 60
        val s = elapsed % 60
        return if (h > 0) "${h}h ${m}m ${s}s" else "${m}m ${s}s"
    }

    val stateDisplayName: String get() = when (state) {
        "idle" -> "GOTOWY"
        "painting" -> "MALOWANIE"
        "paused" -> "PAUZA"
        "stopped" -> "STOP"
        else -> state.uppercase()
    }

    val modeDisplayName: String get() = when (mode) {
        "auto" -> "AUTO"
        "semi" -> "SEMI-AUTO"
        "manual" -> "MANUAL"
        "demo" -> "DEMO"
        else -> mode.uppercase()
    }

    val isPainting: Boolean get() = state == "painting"
    val isIdle: Boolean get() = state == "idle"
    val isPaused: Boolean get() = state == "paused"
}
