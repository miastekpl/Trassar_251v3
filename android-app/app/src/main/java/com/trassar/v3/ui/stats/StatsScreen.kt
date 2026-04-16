package com.trassar.v3.ui.stats

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.trassar.v3.ui.components.ConnectionBanner
import com.trassar.v3.ui.components.MetricRow
import com.trassar.v3.ui.components.StatusCard
import com.trassar.v3.ui.theme.StatusStopped
import com.trassar.v3.ui.theme.TrassarOrange

@Composable
fun StatsScreen(vm: StatsViewModel = viewModel()) {
    val stats by vm.stats.collectAsState()
    val connected by vm.connected.collectAsState()
    val loading by vm.loading.collectAsState()

    LaunchedEffect(Unit) { vm.loadStats() }

    Column(modifier = Modifier.fillMaxSize()) {
        ConnectionBanner(connected)

        Column(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            // Refresh
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(
                    text = "Statystyki",
                    style = MaterialTheme.typography.headlineMedium,
                    color = Color.White,
                )
                if (loading) {
                    CircularProgressIndicator(
                        modifier = Modifier.padding(8.dp),
                        color = TrassarOrange,
                        strokeWidth = 2.dp,
                    )
                } else {
                    IconButton(onClick = { vm.loadStats() }) {
                        Icon(Icons.Default.Refresh, "Odswiez", tint = TrassarOrange)
                    }
                }
            }

            // === Sesja biezaca ===
            StatusCard(title = "SESJA BIEZACA") {
                Spacer(Modifier.height(8.dp))
                MetricRow("Dystans", stats.sessionDistanceKm, "km")
                MetricRow("Powierzchnia", stats.sessionAreaM2, "m\u00B2")
                MetricRow("Czas", stats.sessionTimeFormatted)
            }

            // === Lifetime ===
            StatusCard(title = "LIFETIME") {
                Spacer(Modifier.height(8.dp))
                MetricRow("Dystans", stats.lifetimeDistanceKm, "km")
                MetricRow("Powierzchnia", stats.lifetimeAreaM2, "m\u00B2")
                MetricRow("Czas malowania", stats.lifetimePaintTimeFormatted)
            }

            // === Farba ===
            StatusCard(title = "ZUZYCIE FARBY") {
                Spacer(Modifier.height(8.dp))
                MetricRow("Zuzyto", stats.paintUsedL, "L")
                MetricRow("Pozostalo", stats.paintRemainingL, "L")
                MetricRow("Pojemnosc zbiornika", stats.paintTankL, "L")
                MetricRow("Tankowania", stats.refuelCount.toString())
                MetricRow("Zatankowano lacznie", stats.totalRefueledL, "L")
                Spacer(Modifier.height(4.dp))
                LinearProgressIndicator(
                    progress = { (100 - stats.paintUsedPct) / 100f },
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(8.dp)
                        .clip(RoundedCornerShape(4.dp)),
                    color = if (stats.paintUsedPct > 85) StatusStopped else TrassarOrange,
                    trackColor = Color(0xFF2A2A3E),
                )
            }

            // === Dystans per pistolet (sesja) ===
            StatusCard(title = "DYSTANS PISTOLETOW (SESJA)") {
                Spacer(Modifier.height(8.dp))
                for (i in 0 until 6) {
                    MetricRow(
                        "P${i + 1}",
                        stats.gunDistances.getOrElse(i) { "0.0" },
                        "m",
                    )
                }
            }

            // === Strzaly pistoletow (lifetime) ===
            StatusCard(title = "STRZALY PISTOLETOW (LIFETIME)") {
                Spacer(Modifier.height(8.dp))
                for (i in 0 until 6) {
                    MetricRow(
                        "P${i + 1}",
                        stats.gunShotCounts.getOrElse(i) { 0L }.toString(),
                    )
                }
            }

            // === SD ===
            StatusCard(title = "KARTA SD") {
                Spacer(Modifier.height(8.dp))
                MetricRow(
                    "Status",
                    if (stats.sdReady) "OK" else "Brak",
                    valueColor = if (stats.sdReady) Color.Green else StatusStopped,
                )
                MetricRow("Raporty", stats.reportCount.toString())
            }

            Spacer(Modifier.height(80.dp))
        }
    }
}
