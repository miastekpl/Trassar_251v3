package com.trassar.v3.data.repository

import com.trassar.v3.data.model.MachineStatus
import com.trassar.v3.data.model.Report
import com.trassar.v3.data.model.Statistics
import com.trassar.v3.data.model.Track
import com.trassar.v3.data.remote.TrassarApiService
import com.trassar.v3.data.remote.TrassarWebSocket
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.StateFlow
import java.io.File

class TrassarRepository(
    private val api: TrassarApiService,
    private val ws: TrassarWebSocket,
) {
    // Live status via WebSocket
    val status: StateFlow<MachineStatus> = ws.status
    val connected: StateFlow<Boolean> = ws.connected
    val errors: SharedFlow<String> = ws.errors

    fun connectWebSocket() = ws.connect()
    fun disconnectWebSocket() = ws.disconnect()

    // Machine controls
    suspend fun start() = api.start()
    suspend fun pause() = api.pause()
    suspend fun stop() = api.stop()
    suspend fun startFromGap() = api.startFromGap()
    suspend fun semiNextLine() = api.semiNextLine()

    suspend fun setPattern(index: Int) = api.setPattern(index)
    suspend fun toggleReverse() = api.toggleReverse()

    suspend fun setMode(mode: Int) = api.setMode(mode)
    suspend fun setMaxSpeed(kmh: Float) = api.setMaxSpeed(kmh)
    suspend fun setMinSpeed(kmh: Float) = api.setMinSpeed(kmh)
    suspend fun setAutoResume(enabled: Boolean) = api.setAutoResume(enabled)

    // Stats
    suspend fun getStats(): Result<Statistics> = api.getStats()

    // Reports
    suspend fun getReports(): Result<List<Report>> = api.getReports()
    suspend fun downloadReport(filename: String, destFile: File) = api.downloadReport(filename, destFile)

    // Tracks
    suspend fun getTracks(): Result<List<Track>> = api.getTracks()
    suspend fun downloadTrack(filename: String, destFile: File) = api.downloadTrack(filename, destFile)
}
