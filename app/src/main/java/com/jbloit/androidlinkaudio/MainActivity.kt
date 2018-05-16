package com.jbloit.androidlinkaudio

import android.Manifest
import android.animation.TimeAnimator
import android.content.pm.PackageManager
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.support.v4.app.ActivityCompat
import android.widget.SeekBar
import android.widget.Toast
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity(), SeekBar.OnSeekBarChangeListener {

    private val animationLoop: TimeAnimator = TimeAnimator()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        initPermissions()

        AudioEngine.create()
        AudioEngine.createAudioStream()

        // register widget callbacks
        toggle_linkEnable.setOnCheckedChangeListener{ _, isChecked ->
            AudioEngine.linkEnable(isChecked)
        }

        toggle_playAudio.setOnCheckedChangeListener{ _, isChecked ->
            AudioEngine.playAudio(isChecked)
        }

        toggle_latencyDetection.setOnCheckedChangeListener{_, isChecked ->
            AudioEngine.detectLatency(isChecked)
        }

        seekBar_latencyMs.setOnSeekBarChangeListener(this)

        startAnimationLoop()
    }

    override fun onStop() {
        super.onStop()
        stopAnimationLoop()
    }

    // Seekbar callbacks
    override fun onStartTrackingTouch(seekBar: SeekBar?) {

    }

    override fun onStopTrackingTouch(seekBar: SeekBar?) {

    }

    override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
        AudioEngine.setLatency(progress)
        text_latencyMs.text = "Output Latency : $progress ms \n Set to a little more than half of round-trip latency"
    }


    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }

    //region HELPERS
    fun initPermissions() {

        val recordPermission = Manifest.permission.RECORD_AUDIO
        val permissions = arrayOf(recordPermission)

        if (!hasPermission(recordPermission)) {
            ActivityCompat.requestPermissions(this, permissions, 0)
            return
        }
    }
    private fun hasPermission(permission: String): Boolean {
        val check = ActivityCompat.checkSelfPermission(this, permission)
        return check == PackageManager.PERMISSION_GRANTED
    }

    //region ANIMATION LOOP
    fun pollEngine(delta: Long){

        val detectedLatencyMs = AudioEngine.getLatency()
        text_latencyDetection.text = "Detected round-trip latency: $detectedLatencyMs ms"
    }
    fun startAnimationLoop(){
        animationLoop.setTimeListener(TimeAnimator.TimeListener({ animation, totalTime, deltaTime ->  pollEngine(deltaTime)}))
        animationLoop.start()
    }
    fun stopAnimationLoop(){
        animationLoop.cancel()
        animationLoop.setTimeListener(null)
        animationLoop.removeAllListeners()
    }

}
