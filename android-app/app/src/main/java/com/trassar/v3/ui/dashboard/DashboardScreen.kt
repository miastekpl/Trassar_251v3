package com.trassar.v3.ui.dashboard

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.trassar.v3.ui.components.ConnectionBanner
import com.trassar.v3.ui.components.GunIndicator
import com.trassar.v3.ui.components.MetricRow
import com.trassar.v3.ui.components.StatusCard
import com.trassar.v3.ui.components.stateColor
import com.trassar.v3.ui.theme.GpsFix
import com.trassar.v3.ui.theme.GpsNoFix
import com.trassar.v3.ui.theme.StatusPainting
import com.trassar.v3.ui.theme.StatusStopped
import com.trassar.v3.ui.theme.TrassarOrange

@Composable
fun DashboardScreen(vm: DashboardViewModel = viewModel()) {
    val status by vm.status.collectAsState()
    val connected by vm.connected.collectAsState()

    Column(modifier = Modifier.fillMaxSize()) {
        ConnectionBanner(connected)

        Column(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            // === Stan maszyny ===
            StatusCard(title = "STAN MASZYNY") {
                Spacer(Modifier.height(8.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    Column {
                        Text(
                            text = status.stateDisplayName,
                            style = MaterialTheme.typography.headlineLarge,
                            color = stateColor(status.state),
                        )
                        Text(
                            text = "Tryb: ${status.modeDisplayName}",
                            style = MaterialTheme.typography.bodyMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                    }
                    Column(horizontalAlignment = Alignment.End) {
                        Text(
                            text = status.pattern,
                            style = MaterialTheme.typography.headlineMedium,
                            color = TrassarOrange,
                        )
                        Text(
                            text = status.patternName,
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                    }
                }
            }

            // === Predkosc i dystans ===
            StatusCard(title = "PREDKOSC / DYSTANS") {
                Spacer(Modifier.height(8.dp))
                // Big speed display
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.Center,
                    verticalAlignment = Alignment.Bottom,
                ) {
                    Text(
                        text = status.speed,
                        fontSize = 48.sp,
                        fontWeight = FontWeight.Bold,
                        color = when {
                            status.overspeed -> StatusStopped
                            status.isPainting -> StatusPainting
                            else -> Color.White
                        },
                    )
                    Text(
                        text = " km/h",
                        style = MaterialTheme.typography.titleLarge,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                        modifier = Modifier.padding(bottom = 8.dp),
                    )
                }
                Spacer(Modifier.height(8.dp))
                MetricRow("Dystans", status.distance, "m")
                MetricRow("Powierzchnia", status.area, "m\u00B2")
                MetricRow("Czas sesji", status.elapsedFormatted)
                if (status.overspeed) {
                    Spacer(Modifier.height(4.dp))
                    Text(
                        text = "PRZEKROCZENIE PREDKOSCI!",
                        style = MaterialTheme.typography.labelLarge,
                        color = StatusStopped,
                    )
                }
                if (status.lowSpeed && status.isPainting) {
                    Text(
                        text = "Za wolna predkosc",
                        style = MaterialTheme.typography.labelSmall,
                        color = StatusStopped,
                    )
                }
            }

            // === Pistolety ===
            StatusCard(title = "PISTOLETY") {
                Spacer(Modifier.height(8.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceEvenly,
                ) {
                    for (i in 0 until 6) {
                        GunIndicator(
                            index = i,
                            isOn = status.guns.getOrElse(i) { false },
                            hasAnomaly = status.gunAnomaly.getOrElse(i) { false },
                        )
                    }
                }
                if (status.gunAnomalyDetected) {
                    Spacer(Modifier.height(4.dp))
                    Text(
                        text = "Wykryto anomalie pistoletow!",
                        style = MaterialTheme.typography.labelSmall,
                        color = StatusStopped,
                    )
                }
            }

            // === Farba ===
            StatusCard(title = "FARBA") {
                Spacer(Modifier.height(8.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    Text(
                        text = "${status.paintLevelL} L",
                        style = MaterialTheme.typography.headlineMedium,
                        fontWeight = FontWeight.Bold,
                        color = if (status.paintLevelPct < 15) StatusStopped else Color.White,
                    )
                    Text(
                        text = "${status.paintLevelPct}%",
                        style = MaterialTheme.typography.titleLarge,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
                Spacer(Modifier.height(4.dp))
                LinearProgressIndicator(
                    progress = { status.paintLevelPct / 100f },
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(8.dp)
                        .clip(RoundedCornerShape(4.dp)),
                    color = if (status.paintLevelPct < 15) StatusStopped else TrassarOrange,
                    trackColor = Color(0xFF2A2A3E),
                )
            }

            // === GPS ===
            StatusCard(title = "GPS") {
                Spacer(Modifier.height(8.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Box(
                            modifier = Modifier
                                .size(10.dp)
                                .clip(RoundedCornerShape(5.dp))
                                .background(if (status.gpsFix) GpsFix else GpsNoFix),
                        )
                        Text(
                            text = if (status.gpsFix) " FIX" else " Brak",
                            style = MaterialTheme.typography.bodyMedium,
                            color = if (status.gpsFix) GpsFix else GpsNoFix,
                        )
                    }
                    Text(
                        text = "Sat: ${status.gpsSat}",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
                if (status.gpsFix) {
                    MetricRow("Szer.", status.gpsLat, "\u00B0")
                    MetricRow("Dl.", status.gpsLng, "\u00B0")
                    MetricRow("HDOP", status.gpsHdop)
                }
                if (status.gpxRec) {
                    MetricRow("Nagrywanie trasy", "${status.gpxPts} pkt",
                        valueColor = StatusPainting)
                }
            }

            // === System ===
            StatusCard(title = "SYSTEM") {
                Spacer(Modifier.height(8.dp))
                MetricRow("Firmware", status.firmware)
                MetricRow("Uptime", status.uptimeFormatted)
                MetricRow("Free heap", "${status.freeHeap / 1024} KB")
                MetricRow("Klienci WiFi", status.clients.toString())
            }

            Spacer(Modifier.height(80.dp)) // Space for bottom nav
        }
    }
}
