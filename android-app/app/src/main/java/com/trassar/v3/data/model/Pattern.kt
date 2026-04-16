package com.trassar.v3.data.model

data class PatternInfo(
    val index: Int,
    val code: String,
    val name: String,
    val group: String,
) {
    companion object {
        val ALL = listOf(
            PatternInfo(0, "P-1a", "Przerywana dluga", "P-1"),
            PatternInfo(1, "P-1b", "Przerywana krotka", "P-1"),
            PatternInfo(2, "P-1c", "Rozdzielajaca", "P-1"),
            PatternInfo(3, "P-1d", "Naprowadzajaca waska", "P-1"),
            PatternInfo(4, "P-1e", "Naprowadzajaca szeroka", "P-1"),
            PatternInfo(5, "P-2a", "Ciagla waska", "P-2"),
            PatternInfo(6, "P-2b", "Ciagla szeroka", "P-2"),
            PatternInfo(7, "P-3a", "Skrzyzowanie dluga", "P-3"),
            PatternInfo(8, "P-3b", "Skrzyzowanie krotka", "P-3"),
            PatternInfo(9, "P-4", "Podwojna ciagla", "P-4"),
            PatternInfo(10, "P-6", "Ostrzegawcza", "P-6"),
            PatternInfo(11, "P-7a", "Krawedz. przeryw. szer.", "P-7"),
            PatternInfo(12, "P-7b", "Krawedz. ciagla szer.", "P-7"),
            PatternInfo(13, "P-7c", "Krawedz. przeryw. wask.", "P-7"),
            PatternInfo(14, "P-7d", "Krawedz. ciagla wask.", "P-7"),
            PatternInfo(15, "Wlasny", "Wzorzec uzytkownika", "Custom"),
        )

        fun byIndex(idx: Int): PatternInfo? = ALL.getOrNull(idx)
    }
}
