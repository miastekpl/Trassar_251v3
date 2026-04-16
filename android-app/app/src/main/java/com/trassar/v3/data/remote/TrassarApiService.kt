package com.trassar.v3.data.remote

import com.trassar.v3.data.model.Report
import com.trassar.v3.data.model.Statistics
import com.trassar.v3.data.model.Track
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json
import okhttp3.FormBody
import okhttp3.OkHttpClient
import okhttp3.Request
import java.io.File
import java.io.FileOutputStream
import java.util.concurrent.TimeUnit

class TrassarApiService(private val baseUrl: String) {

    private val json = Json { ignoreUnknownKeys = true; coerceInputValues = true }
    private val client = OkHttpClient.Builder()
        .connectTimeout(5, TimeUnit.SECONDS)
        .readTimeout(10, TimeUnit.SECONDS)
        .writeTimeout(5, TimeUnit.SECONDS)
        .build()

    // ========== Control API ==========

    suspend fun sendControl(action: String, params: Map<String, String> = emptyMap()): Result<String> =
        withContext(Dispatchers.IO) {
            try {
                val bodyBuilder = FormBody.Builder().add("action", action)
                params.forEach { (k, v) -> bodyBuilder.add(k, v) }

                val request = Request.Builder()
                    .url("$baseUrl/api/control")
                    .post(bodyBuilder.build())
                    .build()

                val response = client.newCall(request).execute()
                if (response.isSuccessful) {
                    Result.success(response.body?.string() ?: "OK")
                } else {
                    Result.failure(Exception("HTTP ${response.code}: ${response.body?.string()}"))
                }
            } catch (e: Exception) {
                Result.failure(e)
            }
        }

    suspend fun start() = sendControl("start")
    suspend fun pause() = sendControl("pause")
    suspend fun stop() = sendControl("stop")
    suspend fun startFromGap() = sendControl("start_from_gap")
    suspend fun semiNextLine() = sendControl("semi_next_line")

    suspend fun setPattern(index: Int) = sendControl("set_pattern", mapOf("value" to index.toString()))
    suspend fun toggleReverse() = sendControl("toggle_reverse")

    suspend fun setMode(mode: Int) = sendControl("set_mode", mapOf("value" to mode.toString()))
    suspend fun setMaxSpeed(kmh: Float) = sendControl("set_max_speed", mapOf("value" to kmh.toString()))
    suspend fun setMinSpeed(kmh: Float) = sendControl("set_min_speed", mapOf("value" to kmh.toString()))
    suspend fun setAutoResume(enabled: Boolean) = sendControl("set_auto_resume", mapOf("value" to if (enabled) "1" else "0"))
    suspend fun setSmartSwitch(mode: Int) = sendControl("set_switch_mode", mapOf("value" to mode.toString()))

    // ========== Stats API ==========

    suspend fun getStats(): Result<Statistics> = withContext(Dispatchers.IO) {
        try {
            val request = Request.Builder().url("$baseUrl/api/stats").build()
            val response = client.newCall(request).execute()
            if (response.isSuccessful) {
                val body = response.body?.string() ?: "{}"
                Result.success(json.decodeFromString<Statistics>(body))
            } else {
                Result.failure(Exception("HTTP ${response.code}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    // ========== Reports API ==========

    suspend fun getReports(): Result<List<Report>> = withContext(Dispatchers.IO) {
        try {
            val request = Request.Builder().url("$baseUrl/api/reports").build()
            val response = client.newCall(request).execute()
            if (response.isSuccessful) {
                val body = response.body?.string() ?: "[]"
                Result.success(json.decodeFromString<List<Report>>(body))
            } else {
                Result.failure(Exception("HTTP ${response.code}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun downloadReport(filename: String, destFile: File): Result<File> =
        withContext(Dispatchers.IO) {
            try {
                val request = Request.Builder()
                    .url("$baseUrl/api/reports/download?file=$filename")
                    .build()
                val response = client.newCall(request).execute()
                if (response.isSuccessful) {
                    response.body?.byteStream()?.use { input ->
                        FileOutputStream(destFile).use { output ->
                            input.copyTo(output)
                        }
                    }
                    Result.success(destFile)
                } else {
                    Result.failure(Exception("HTTP ${response.code}"))
                }
            } catch (e: Exception) {
                Result.failure(e)
            }
        }

    // ========== Tracks API ==========

    suspend fun getTracks(): Result<List<Track>> = withContext(Dispatchers.IO) {
        try {
            val request = Request.Builder().url("$baseUrl/api/tracks").build()
            val response = client.newCall(request).execute()
            if (response.isSuccessful) {
                val body = response.body?.string() ?: "[]"
                Result.success(json.decodeFromString<List<Track>>(body))
            } else {
                Result.failure(Exception("HTTP ${response.code}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun downloadTrack(filename: String, destFile: File): Result<File> =
        withContext(Dispatchers.IO) {
            try {
                val request = Request.Builder()
                    .url("$baseUrl/api/tracks/download?file=$filename")
                    .build()
                val response = client.newCall(request).execute()
                if (response.isSuccessful) {
                    response.body?.byteStream()?.use { input ->
                        FileOutputStream(destFile).use { output ->
                            input.copyTo(output)
                        }
                    }
                    Result.success(destFile)
                } else {
                    Result.failure(Exception("HTTP ${response.code}"))
                }
            } catch (e: Exception) {
                Result.failure(e)
            }
        }
}
