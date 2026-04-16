package com.trassar.v3.ui.map

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.viewmodel.compose.viewModel
import com.trassar.v3.ui.components.ConnectionBanner
import com.trassar.v3.ui.components.MetricRow
import com.trassar.v3.ui.theme.DarkCard
import com.trassar.v3.ui.theme.GpsFix
import com.trassar.v3.ui.theme.GpsNoFix
import com.trassar.v3.ui.theme.StatusPainting
import org.osmdroid.tileprovider.tilesource.TileSourceFactory
import org.osmdroid.util.GeoPoint
import org.osmdroid.views.MapView
import org.osmdroid.views.overlay.Marker
import org.osmdroid.views.overlay.Polyline

@Composable
fun MapScreen(vm: MapViewModel = viewModel()) {
    val status by vm.status.collectAsState()
    val connected by vm.connected.collectAsState()
    val context = LocalContext.current

    val mapView = remember {
        MapView(context).apply {
            setTileSource(TileSourceFactory.MAPNIK)
            setMultiTouchControls(true)
            controller.setZoom(17.0)
            // Default center — Poland
            controller.setCenter(GeoPoint(52.0, 21.0))
        }
    }

    val marker = remember { Marker(mapView) }
    val trackLine = remember {
        Polyline().apply {
            outlinePaint.color = android.graphics.Color.rgb(0, 230, 118)  // GunOn green
            outlinePaint.strokeWidth = 6f
        }
    }

    // Track points collected during painting
    val trackPoints = remember { mutableListOf<GeoPoint>() }

    // Update marker position when GPS data changes
    LaunchedEffect(status.gpsLat, status.gpsLng, status.gpsFix) {
        if (status.gpsFix) {
            val lat = status.gpsLatDouble
            val lng = status.gpsLngDouble
            if (lat != 0.0 && lng != 0.0) {
                val point = GeoPoint(lat, lng)
                marker.position = point
                marker.title = "${status.speed} km/h | ${status.pattern}"
                marker.setAnchor(Marker.ANCHOR_CENTER, Marker.ANCHOR_BOTTOM)

                if (!mapView.overlays.contains(marker)) {
                    mapView.overlays.add(marker)
                }

                // Add to track if painting
                if (status.isPainting) {
                    trackPoints.add(point)
                    trackLine.setPoints(trackPoints.toList())
                    if (!mapView.overlays.contains(trackLine)) {
                        mapView.overlays.add(0, trackLine)
                    }
                }

                mapView.controller.animateTo(point)
                mapView.invalidate()
            }
        }
    }

    DisposableEffect(Unit) {
        onDispose { mapView.onDetach() }
    }

    Column(modifier = Modifier.fillMaxSize()) {
        ConnectionBanner(connected)

        // GPS info overlay
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 12.dp, vertical = 8.dp),
            shape = RoundedCornerShape(12.dp),
            colors = CardDefaults.cardColors(containerColor = DarkCard.copy(alpha = 0.95f)),
        ) {
            Column(modifier = Modifier.padding(12.dp)) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    Text(
                        text = if (status.gpsFix) "GPS FIX" else "GPS — brak sygnalu",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold,
                        color = if (status.gpsFix) GpsFix else GpsNoFix,
                    )
                    Text(
                        text = "Sat: ${status.gpsSat}",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
                if (status.gpsFix) {
                    MetricRow("Lat", status.gpsLat, "\u00B0")
                    MetricRow("Lng", status.gpsLng, "\u00B0")
                    MetricRow("Predkosc GPS", status.gpsSpeed, "km/h")
                    MetricRow("HDOP", status.gpsHdop)
                }
                if (status.gpxRec) {
                    MetricRow("Nagrywanie", "${status.gpxPts} punktow",
                        valueColor = StatusPainting)
                }
            }
        }

        // Map
        Box(modifier = Modifier.fillMaxSize()) {
            AndroidView(
                factory = { mapView },
                modifier = Modifier.fillMaxSize(),
            )

            if (!status.gpsFix) {
                Box(
                    modifier = Modifier.fillMaxSize(),
                    contentAlignment = Alignment.Center,
                ) {
                    Card(
                        shape = RoundedCornerShape(12.dp),
                        colors = CardDefaults.cardColors(containerColor = DarkCard.copy(alpha = 0.85f)),
                    ) {
                        Column(
                            modifier = Modifier.padding(24.dp),
                            horizontalAlignment = Alignment.CenterHorizontally,
                        ) {
                            Text(
                                text = "Oczekiwanie na sygnal GPS...",
                                style = MaterialTheme.typography.titleMedium,
                                color = Color.White,
                            )
                            Spacer(Modifier.height(4.dp))
                            Text(
                                text = "Satelity: ${status.gpsSat}",
                                style = MaterialTheme.typography.bodyMedium,
                                color = MaterialTheme.colorScheme.onSurfaceVariant,
                            )
                        }
                    }
                }
            }
        }
    }
}
