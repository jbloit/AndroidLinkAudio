package com.jbloit.androidlinkaudio

import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.widget.CompoundButton
import android.widget.Toast
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Example of a call to a native method
        sample_text.text = stringFromJNI()

        linkToggleButton.setOnCheckedChangeListener{ buttonView, isChecked ->
            Toast.makeText(this,isChecked.toString(),Toast.LENGTH_SHORT).show()
            AudioEngine.linkEnable(isChecked)
        }

        audioStreamToggleButton.setOnCheckedChangeListener{ buttonView, isChecked ->
            Toast.makeText(this,isChecked.toString(),Toast.LENGTH_SHORT).show()
            if (isChecked){
                AudioEngine.createAudioStream()
            }
        }

        AudioEngine.create()
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
}
