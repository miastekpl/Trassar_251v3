package com.trassar.v3.ui.controls

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.trassar.v3.TrassarApp
import com.trassar.v3.data.model.MachineStatus
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch

class ControlsViewModel : ViewModel() {

    private val repo = TrassarApp.instance.repository

    val status: StateFlow<MachineStatus> = repo.status
    val connected: StateFlow<Boolean> = repo.connected

    private val _actionResult = MutableStateFlow<String?>(null)
    val actionResult: StateFlow<String?> = _actionResult

    fun start() = sendAction { repo.start() }
    fun pause() = sendAction { repo.pause() }
    fun stop() = sendAction { repo.stop() }
    fun startFromGap() = sendAction { repo.startFromGap() }
    fun semiNextLine() = sendAction { repo.semiNextLine() }

    fun setPattern(index: Int) = sendAction { repo.setPattern(index) }
    fun toggleReverse() = sendAction { repo.toggleReverse() }

    fun setMode(mode: Int) = sendAction { repo.setMode(mode) }
    fun setAutoResume(enabled: Boolean) = sendAction { repo.setAutoResume(enabled) }

    fun clearActionResult() { _actionResult.value = null }

    private fun sendAction(block: suspend () -> Result<String>) {
        viewModelScope.launch {
            val result = block()
            result.onFailure { _actionResult.value = "Blad: ${it.message}" }
        }
    }
}
