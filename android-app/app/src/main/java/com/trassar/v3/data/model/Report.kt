package com.trassar.v3.data.model

import kotlinx.serialization.Serializable

@Serializable
data class Report(
    val name: String,
    val size: Long = 0,
)

@Serializable
data class Track(
    val name: String,
    val size: Long = 0,
)
