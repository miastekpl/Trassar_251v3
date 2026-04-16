package com.trassar.v3.ui.dashboard

import androidx.lifecycle.ViewModel
import com.trassar.v3.TrassarApp
import com.trassar.v3.data.model.MachineStatus
import kotlinx.coroutines.flow.StateFlow

class DashboardViewModel : ViewModel() {

    private val repo = TrassarApp.instance.repository

    val status: StateFlow<MachineStatus> = repo.status
    val connected: StateFlow<Boolean> = repo.connected
}
