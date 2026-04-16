package com.trassar.v3.ui.stats

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.trassar.v3.TrassarApp
import com.trassar.v3.data.model.MachineStatus
import com.trassar.v3.data.model.Statistics
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch

class StatsViewModel : ViewModel() {

    private val repo = TrassarApp.instance.repository

    val status: StateFlow<MachineStatus> = repo.status
    val connected: StateFlow<Boolean> = repo.connected

    private val _stats = MutableStateFlow(Statistics())
    val stats: StateFlow<Statistics> = _stats

    private val _loading = MutableStateFlow(false)
    val loading: StateFlow<Boolean> = _loading

    fun loadStats() {
        viewModelScope.launch {
            _loading.value = true
            repo.getStats()
                .onSuccess { _stats.value = it }
            _loading.value = false
        }
    }
}
