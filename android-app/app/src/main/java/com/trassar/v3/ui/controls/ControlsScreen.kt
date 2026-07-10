package com.trassar.v3.ui.controls

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.SkipNext
import androidx.compose.material.icons.filled.Stop
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.FilterChip
import androidx.compose.material3.FilterChipDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.trassar.v3.data.model.PatternInfo
import com.trassar.v3.ui.components.ConnectionBanner
import com.trassar.v3.ui.components.StatusCard
import com.trassar.v3.ui.components.stateColor
import com.trassar.v3.ui.theme.StatusPainting
import com.trassar.v3.ui.theme.StatusPaused
import com.trassar.v3.ui.theme.StatusStopped
import com.trassar.v3.ui.theme.TrassarOrange

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun ControlsScreen(vm: ControlsViewModel = viewModel()) {
    val status by vm.status.collectAsState()
    val connected by vm.connected.collectAsState()
    val actionResult by vm.actionResult.collectAsState()

    val snackbar = remember { SnackbarHostState() }
    LaunchedEffect(actionResult) {
        actionResult?.let {
            snackbar.showSnackbar(it)
            vm.clearActionResult()
        }
    }

    Scaffold(
        snackbarHost = { SnackbarHost(snackbar) },
        containerColor = MaterialTheme.colorScheme.background,
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding),
        ) {
            ConnectionBanner(connected)

            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .verticalScroll(rememberScrollState())
                    .padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp),
            ) {
                // === Machine controls ===
                StatusCard(title = "STEROWANIE MASZYNA") {
                    Spacer(Modifier.height(8.dp))

                    Text(
                        text = status.stateDisplayName,
                        style = MaterialTheme.typography.titleLarge,
                        color = stateColor(status.state),
                        fontWeight = FontWeight.Bold,
                    )
                    Spacer(Modifier.height(12.dp))

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        // START
                        Button(
                            onClick = { vm.start() },
                            modifier = Modifier.weight(1f),
                            enabled = connected && (status.isIdle || status.isPaused),
                            colors = ButtonDefaults.buttonColors(
                                containerColor = StatusPainting,
                            ),
                            shape = RoundedCornerShape(8.dp),
                        ) {
                            Icon(Icons.Default.PlayArrow, contentDescription = null, Modifier.size(20.dp))
                            Spacer(Modifier.width(4.dp))
                            Text("START")
                        }

                        // PAUSE
                        Button(
                            onClick = { vm.pause() },
                            modifier = Modifier.weight(1f),
                            enabled = connected && status.isPainting,
                            colors = ButtonDefaults.buttonColors(
                                containerColor = StatusPaused,
                                contentColor = Color.Black,
                            ),
                            shape = RoundedCornerShape(8.dp),
                        ) {
                            Icon(Icons.Default.Pause, contentDescription = null, Modifier.size(20.dp))
                            Spacer(Modifier.width(4.dp))
                            Text("PAUZA")
                        }

                        // STOP
                        Button(
                            onClick = { vm.stop() },
                            modifier = Modifier.weight(1f),
                            enabled = connected && (status.isPainting || status.isPaused),
                            colors = ButtonDefaults.buttonColors(
                                containerColor = StatusStopped,
                            ),
                            shape = RoundedCornerShape(8.dp),
                        ) {
                            Icon(Icons.Default.Stop, contentDescription = null, Modifier.size(20.dp))
                            Spacer(Modifier.width(4.dp))
                            Text("STOP")
                        }
                    }

                    // Semi-auto: next line button
                    if (status.mode == "semi") {
                        Spacer(Modifier.height(8.dp))
                        OutlinedButton(
                            onClick = { vm.semiNextLine() },
                            modifier = Modifier.fillMaxWidth(),
                            enabled = connected && status.isPainting && status.semiLineComplete,
                            border = BorderStroke(1.dp, TrassarOrange),
                            shape = RoundedCornerShape(8.dp),
                        ) {
                            Icon(Icons.Default.SkipNext, contentDescription = null, Modifier.size(20.dp))
                            Spacer(Modifier.width(4.dp))
                            Text("Nastepna linia (SEMI)")
                        }
                    }

                    // Start from gap
                    if (status.isPaused && status.gapStart) {
                        Spacer(Modifier.height(8.dp))
                        OutlinedButton(
                            onClick = { vm.startFromGap() },
                            modifier = Modifier.fillMaxWidth(),
                            enabled = connected,
                            border = BorderStroke(1.dp, TrassarOrange),
                            shape = RoundedCornerShape(8.dp),
                        ) {
                            Text("Wznow od przerwy")
                        }
                    }
                }

                // === Tryb pracy ===
                StatusCard(title = "TRYB PRACY") {
                    Spacer(Modifier.height(8.dp))
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        val modes = listOf("AUTO" to 0, "SEMI" to 1, "MANUAL" to 2)
                        val currentModeIndex = when (status.mode) {
                            "auto" -> 0; "semi" -> 1; "manual" -> 2; else -> 0
                        }
                        modes.forEach { (label, idx) ->
                            FilterChip(
                                selected = idx == currentModeIndex,
                                onClick = { vm.setMode(idx) },
                                label = { Text(label) },
                                enabled = connected && !status.isPainting,
                                modifier = Modifier.weight(1f),
                                colors = FilterChipDefaults.filterChipColors(
                                    selectedContainerColor = TrassarOrange,
                                    selectedLabelColor = Color.Black,
                                ),
                            )
                        }
                    }
                }

                // === Wybor wzorca ===
                StatusCard(title = "WZORZEC: ${status.pattern}") {
                    Spacer(Modifier.height(8.dp))

                    val groups = PatternInfo.ALL.groupBy { it.group }
                    groups.forEach { (group, patterns) ->
                        Text(
                            text = group,
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                        Spacer(Modifier.height(4.dp))
                        FlowRow(
                            horizontalArrangement = Arrangement.spacedBy(6.dp),
                            verticalArrangement = Arrangement.spacedBy(6.dp),
                        ) {
                            patterns.forEach { pat ->
                                val isSelected = pat.index == status.patternIdx
                                FilterChip(
                                    selected = isSelected,
                                    onClick = { vm.setPattern(pat.index) },
                                    label = { Text(pat.code, style = MaterialTheme.typography.labelSmall) },
                                    enabled = connected,
                                    colors = FilterChipDefaults.filterChipColors(
                                        selectedContainerColor = TrassarOrange,
                                        selectedLabelColor = Color.Black,
                                    ),
                                )
                            }
                        }
                        Spacer(Modifier.height(8.dp))
                    }

                    // Reverse button (for P-3a/P-3b)
                    if (status.patternIdx == 7 || status.patternIdx == 8) {
                        OutlinedButton(
                            onClick = { vm.toggleReverse() },
                            enabled = connected,
                            border = BorderStroke(1.dp, TrassarOrange),
                            shape = RoundedCornerShape(8.dp),
                        ) {
                            Text(if (status.reversed) "Odwrocony" else "Odwroc kierunek")
                        }
                    }

                    // Pending pattern notification
                    if (status.patternPending) {
                        Spacer(Modifier.height(4.dp))
                        Text(
                            text = "Oczekuje: ${status.pendingPattern} (smart switch)",
                            style = MaterialTheme.typography.labelSmall,
                            color = TrassarOrange,
                        )
                    }
                }

                // === Opcje ===
                StatusCard(title = "OPCJE") {
                    Spacer(Modifier.height(8.dp))
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        Text("Auto-resume", style = MaterialTheme.typography.bodyMedium)
                        FilterChip(
                            selected = status.autoResumeEnabled,
                            onClick = { vm.setAutoResume(!status.autoResumeEnabled) },
                            label = { Text(if (status.autoResumeEnabled) "WL" else "WYL") },
                            enabled = connected,
                            colors = FilterChipDefaults.filterChipColors(
                                selectedContainerColor = StatusPainting,
                                selectedLabelColor = Color.White,
                            ),
                        )
                    }
                }

                Spacer(Modifier.height(80.dp))
            }
        }
    }
}
