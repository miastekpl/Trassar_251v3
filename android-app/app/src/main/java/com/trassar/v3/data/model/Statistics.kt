package com.trassar.v3.data.model

import kotlinx.serialization.Serializable

@Serializable
data class Statistics(
    // Lifetime
    val lifetimeDistanceM: String = "0.0",
    val lifetimeAreaM2: String = "0.00",
    val lifetimePaintTimeSec: Long = 0,

    // Sesja biezaca
    val sessionDistanceM: String = "0.0",
    val sessionAreaM2: String = "0.00",
    val sessionTimeSec: Long = 0,

    // Dystans per pistolet (sesja)
    val gunDistances: List<String> = List(6) { "0.0" },

    // Licznik strzalow pistoletow (lifetime)
    val gunShotCounts: List<Long> = List(6) { 0L },

    // SD
    val sdReady: Boolean = false,
    val reportCount: Int = 0,

    // Farba
    val paintUsedL: String = "0.0",
    val paintRemainingL: String = "0.0",
    val paintTankL: String = "0",
    val paintUsedPct: Int = 0,
    val paintCurrentLevelL: String = "0.0",
    val refuelCount: Int = 0,
    val totalRefueledL: String = "0.0",
) {
    val lifetimeDistanceKm: String get() {
        val m = lifetimeDistanceM.toFloatOrNull() ?: 0f
        return String.format("%.2f", m / 1000f)
    }

    val sessionDistanceKm: String get() {
        val m = sessionDistanceM.toFloatOrNull() ?: 0f
        return String.format("%.2f", m / 1000f)
    }

    val lifetimePaintTimeFormatted: String get() {
        val h = lifetimePaintTimeSec / 3600
        val m = (lifetimePaintTimeSec % 3600) / 60
        return "${h}h ${m}m"
    }

    val sessionTimeFormatted: String get() {
        val h = sessionTimeSec / 3600
        val m = (sessionTimeSec % 3600) / 60
        val s = sessionTimeSec % 60
        return if (h > 0) "${h}h ${m}m ${s}s" else "${m}m ${s}s"
    }
}
