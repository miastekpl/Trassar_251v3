package com.trassar.v3.ui.navigation

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.BarChart
import androidx.compose.material.icons.filled.Dashboard
import androidx.compose.material.icons.filled.Description
import androidx.compose.material.icons.filled.Map
import androidx.compose.material.icons.filled.Tune
import androidx.compose.ui.graphics.vector.ImageVector

enum class TrassarScreen(
    val route: String,
    val title: String,
    val icon: ImageVector,
) {
    Dashboard("dashboard", "Dashboard", Icons.Default.Dashboard),
    Controls("controls", "Sterowanie", Icons.Default.Tune),
    Map("map", "Mapa GPS", Icons.Default.Map),
    Stats("stats", "Statystyki", Icons.Default.BarChart),
    Reports("reports", "Raporty", Icons.Default.Description),
}
