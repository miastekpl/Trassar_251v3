package com.trassar.v3.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.trassar.v3.ui.theme.DarkCard
import com.trassar.v3.ui.theme.GunAnomaly
import com.trassar.v3.ui.theme.GunOff
import com.trassar.v3.ui.theme.GunOn
import com.trassar.v3.ui.theme.StatusIdle
import com.trassar.v3.ui.theme.StatusPainting
import com.trassar.v3.ui.theme.StatusPaused
import com.trassar.v3.ui.theme.StatusStopped

@Composable
fun StatusCard(
    title: String,
    modifier: Modifier = Modifier,
    content: @Composable () -> Unit,
) {
    Card(
        modifier = modifier.fillMaxWidth(),
        shape = RoundedCornerShape(12.dp),
        colors = CardDefaults.cardColors(containerColor = DarkCard),
    ) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = title,
                style = MaterialTheme.typography.labelLarge,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
            content()
        }
    }
}

@Composable
fun MetricRow(label: String, value: String, unit: String = "", valueColor: Color = Color.White) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 2.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
        )
        Text(
            text = if (unit.isNotEmpty()) "$value $unit" else value,
            style = MaterialTheme.typography.bodyMedium,
            fontWeight = FontWeight.SemiBold,
            color = valueColor,
        )
    }
}

@Composable
fun GunIndicator(
    index: Int,
    isOn: Boolean,
    hasAnomaly: Boolean,
    modifier: Modifier = Modifier,
) {
    val color = when {
        hasAnomaly -> GunAnomaly
        isOn -> GunOn
        else -> GunOff
    }
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier = modifier,
    ) {
        Box(
            modifier = Modifier
                .size(28.dp)
                .clip(CircleShape)
                .background(color),
            contentAlignment = Alignment.Center,
        ) {
            Text(
                text = "${index + 1}",
                style = MaterialTheme.typography.labelSmall,
                fontWeight = FontWeight.Bold,
                color = Color.White,
            )
        }
        Text(
            text = "P${index + 1}",
            style = MaterialTheme.typography.labelSmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
        )
    }
}

fun stateColor(state: String): Color = when (state) {
    "painting" -> StatusPainting
    "paused" -> StatusPaused
    "stopped" -> StatusStopped
    else -> StatusIdle
}

@Composable
fun ConnectionBanner(connected: Boolean) {
    if (!connected) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .background(StatusStopped)
                .padding(8.dp),
            contentAlignment = Alignment.Center,
        ) {
            Text(
                text = "Brak polaczenia z maszyna",
                style = MaterialTheme.typography.labelLarge,
                color = Color.White,
            )
        }
    }
}
