package com.trassar.v3.ui.reports

import android.os.Environment
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.trassar.v3.TrassarApp
import com.trassar.v3.data.model.Report
import com.trassar.v3.data.model.Track
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import java.io.File

class ReportsViewModel : ViewModel() {

    private val repo = TrassarApp.instance.repository

    val connected: StateFlow<Boolean> = repo.connected

    private val _reports = MutableStateFlow<List<Report>>(emptyList())
    val reports: StateFlow<List<Report>> = _reports

    private val _tracks = MutableStateFlow<List<Track>>(emptyList())
    val tracks: StateFlow<List<Track>> = _tracks

    private val _loading = MutableStateFlow(false)
    val loading: StateFlow<Boolean> = _loading

    private val _message = MutableStateFlow<String?>(null)
    val message: StateFlow<String?> = _message

    fun loadReports() {
        viewModelScope.launch {
            _loading.value = true
            repo.getReports().onSuccess { _reports.value = it }
            _loading.value = false
        }
    }

    fun loadTracks() {
        viewModelScope.launch {
            _loading.value = true
            repo.getTracks().onSuccess { _tracks.value = it }
            _loading.value = false
        }
    }

    fun downloadReport(filename: String) {
        viewModelScope.launch {
            _message.value = "Pobieranie $filename..."
            val dir = File(
                Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                "TrassarV3/reports"
            )
            dir.mkdirs()
            val dest = File(dir, filename)
            repo.downloadReport(filename, dest)
                .onSuccess { _message.value = "Zapisano: ${it.absolutePath}" }
                .onFailure { _message.value = "Blad: ${it.message}" }
        }
    }

    fun downloadTrack(filename: String) {
        viewModelScope.launch {
            _message.value = "Pobieranie $filename..."
            val dir = File(
                Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                "TrassarV3/tracks"
            )
            dir.mkdirs()
            val dest = File(dir, filename)
            repo.downloadTrack(filename, dest)
                .onSuccess { _message.value = "Zapisano: ${it.absolutePath}" }
                .onFailure { _message.value = "Blad: ${it.message}" }
        }
    }

    fun clearMessage() { _message.value = null }
}
