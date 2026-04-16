package com.trassar.v3

import android.app.Application
import com.trassar.v3.data.remote.TrassarApiService
import com.trassar.v3.data.remote.TrassarWebSocket
import com.trassar.v3.data.repository.TrassarRepository
import org.osmdroid.config.Configuration

class TrassarApp : Application() {

    lateinit var repository: TrassarRepository
        private set

    override fun onCreate() {
        super.onCreate()
        instance = this

        // osmdroid config
        Configuration.getInstance().userAgentValue = packageName
        Configuration.getInstance().osmdroidBasePath = cacheDir

        // Default connection target — ESP32 AP mode
        val baseUrl = "http://192.168.4.1"
        val api = TrassarApiService(baseUrl)
        val ws = TrassarWebSocket(baseUrl)
        repository = TrassarRepository(api, ws)
    }

    companion object {
        lateinit var instance: TrassarApp
            private set
    }
}
