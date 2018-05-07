package com.jbloit.androidlinkaudio

/**
 * Created by bloit on 07/05/2018.
 */


object AudioEngine {
    private var mEngineHandle: Long = 0
    external fun createEngine(): Long
    external fun deleteEngine(engineHandle: Long)


    fun create(): Boolean{
        if (mEngineHandle == 0L) {
            mEngineHandle = createEngine()
        }
        return mEngineHandle != 0L
    }

    fun delete() {
        if (mEngineHandle != 0L) {
            deleteEngine(mEngineHandle)
        }
        mEngineHandle = 0
    }
}