package com.trassar.v3

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.animation.AnimatedContentTransitionScope
import androidx.compose.animation.core.tween
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import com.trassar.v3.ui.controls.ControlsScreen
import com.trassar.v3.ui.dashboard.DashboardScreen
import com.trassar.v3.ui.map.MapScreen
import com.trassar.v3.ui.navigation.TrassarScreen
import com.trassar.v3.ui.reports.ReportsScreen
import com.trassar.v3.ui.stats.StatsScreen
import com.trassar.v3.ui.theme.DarkCard
import com.trassar.v3.ui.theme.TrassarOrange
import com.trassar.v3.ui.theme.TrassarTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            TrassarTheme {
                TrassarMainScreen()
            }
        }
    }
}

@Composable
fun TrassarMainScreen() {
    val navController = rememberNavController()
    val repo = TrassarApp.instance.repository

    // Connect/disconnect WebSocket with lifecycle
    DisposableEffect(Unit) {
        repo.connectWebSocket()
        onDispose { repo.disconnectWebSocket() }
    }

    Scaffold(
        bottomBar = { TrassarBottomBar(navController) },
    ) { padding ->
        NavHost(
            navController = navController,
            startDestination = TrassarScreen.Dashboard.route,
            modifier = Modifier.padding(padding),
            enterTransition = {
                slideIntoContainer(AnimatedContentTransitionScope.SlideDirection.Start, tween(200))
            },
            exitTransition = {
                slideOutOfContainer(AnimatedContentTransitionScope.SlideDirection.Start, tween(200))
            },
            popEnterTransition = {
                slideIntoContainer(AnimatedContentTransitionScope.SlideDirection.End, tween(200))
            },
            popExitTransition = {
                slideOutOfContainer(AnimatedContentTransitionScope.SlideDirection.End, tween(200))
            },
        ) {
            composable(TrassarScreen.Dashboard.route) { DashboardScreen() }
            composable(TrassarScreen.Controls.route) { ControlsScreen() }
            composable(TrassarScreen.Map.route) { MapScreen() }
            composable(TrassarScreen.Stats.route) { StatsScreen() }
            composable(TrassarScreen.Reports.route) { ReportsScreen() }
        }
    }
}

@Composable
private fun TrassarBottomBar(navController: NavHostController) {
    val navBackStackEntry by navController.currentBackStackEntryAsState()
    val currentRoute = navBackStackEntry?.destination?.route

    NavigationBar(containerColor = DarkCard) {
        TrassarScreen.entries.forEach { screen ->
            NavigationBarItem(
                icon = { Icon(screen.icon, contentDescription = screen.title) },
                label = { Text(screen.title, maxLines = 1) },
                selected = currentRoute == screen.route,
                onClick = {
                    navController.navigate(screen.route) {
                        popUpTo(navController.graph.startDestinationId) { saveState = true }
                        launchSingleTop = true
                        restoreState = true
                    }
                },
                colors = NavigationBarItemDefaults.colors(
                    selectedIconColor = TrassarOrange,
                    selectedTextColor = TrassarOrange,
                    unselectedIconColor = Color(0xFF808080),
                    unselectedTextColor = Color(0xFF808080),
                    indicatorColor = TrassarOrange.copy(alpha = 0.15f),
                ),
            )
        }
    }
}
