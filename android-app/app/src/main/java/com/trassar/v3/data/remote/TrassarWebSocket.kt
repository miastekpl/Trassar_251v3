package com.trassar.v3.data.remote

import com.trassar.v3.data.model.MachineStatus
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.serialization.json.Json
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import okhttp3.WebSocket
import okhttp3.WebSocketListener
import java.util.concurrent.TimeUnit

class TrassarWebSocket(private val baseUrl: String) {

    private val json = Json { ignoreUnknownKeys = true; coerceInputValues = true }
    private val client = OkHttpClient.Builder()
        .readTimeout(0, TimeUnit.MILLISECONDS)  // No timeout for WS
        .pingInterval(15, TimeUnit.SECONDS)
        .build()

    private var webSocket: WebSocket? = null

    private val _status = MutableStateFlow(MachineStatus())
    val status: StateFlow<MachineStatus> = _status

    private val _connected = MutableStateFlow(false)
    val connected: StateFlow<Boolean> = _connected

    private val _errors = MutableSharedFlow<String>(
        extraBufferCapacity = 5,
        onBufferOverflow = BufferOverflow.DROP_OLDEST
    )
    val errors: SharedFlow<String> = _errors

    fun connect() {
        disconnect()
        val wsUrl = baseUrl.replace("http://", "ws://").replace("https://", "wss://")
        val request = Request.Builder().url("$wsUrl:81").build()

        webSocket = client.newWebSocket(request, object : WebSocketListener() {
            override fun onOpen(webSocket: WebSocket, response: Response) {
                _connected.value = true
            }

            override fun onMessage(webSocket: WebSocket, text: String) {
                if (text == "{}") return  // ESP32 sends empty JSON on mutex timeout
                try {
                    _status.value = json.decodeFromString<MachineStatus>(text)
                } catch (e: Exception) {
                    _errors.tryEmit("JSON parse: ${e.message}")
                }
            }

            override fun onClosing(webSocket: WebSocket, code: Int, reason: String) {
                webSocket.close(1000, null)
                _connected.value = false
            }

            override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
                _connected.value = false
                _errors.tryEmit("WebSocket: ${t.message}")
            }
        })
    }

    fun disconnect() {
        webSocket?.close(1000, "App closing")
        webSocket = null
        _connected.value = false
    }
}
