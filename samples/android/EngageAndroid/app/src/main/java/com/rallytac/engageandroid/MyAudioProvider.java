//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//
package com.rallytac.engageandroid;

import android.annotation.SuppressLint;
import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.media.audiofx.AcousticEchoCanceler;
import android.media.audiofx.NoiseSuppressor;
import android.os.Build;

import androidx.annotation.RequiresApi;

import com.rallytac.engage.engine.Engine;

public class MyAudioProvider implements Engine.IAudioProvider
{
    final String TAG = MyAudioProvider.class.getSimpleName();

    private AudioManager _manager = null;
    private AudioRecord _rec = null;
    private AudioTrack _track = null;
    private NoiseSuppressor _noiseSuppressor = null;
    private AcousticEchoCanceler _aec = null;
    private RecordingThread _recordingThread = null;
    private PlayoutThread _playoutThread = null;
    private int _recMinBufferSize = -1;

    public MyAudioProvider()
    {
    }

    public AudioManager init()
    {
        Globals.getLogger().d(TAG, "init");
        _manager = (AudioManager) Globals.getEngageApplication().getApplicationContext().getSystemService(Context.AUDIO_SERVICE);
        return getManager();
    }

    public void deInit()
    {
        _manager = null;
    }

    public AudioManager getManager()
    {
        return _manager;
    }

    public AudioRecord getRecorder()
    {
        return _rec;
    }

    public AudioTrack getTrack()
    {
        return _track;
    }

    public NoiseSuppressor getAudioNoiseSuppressor()
    {
        return _noiseSuppressor;
    }

    public AcousticEchoCanceler getAec()
    {
        return _aec;
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    @SuppressLint("MissingPermission")
    public int createAudioSubsystem(String jsonParams)
    {
        Globals.getLogger().d(TAG, "createAudioSubsystem");

        int rc = -1;

        try
        {
            int sampleRate = 8000;
            int channels = 1;
            //int source = MediaRecorder.AudioSource.VOICE_COMMUNICATION;
            int source = MediaRecorder.AudioSource.MIC;
            boolean useNs = true;
            boolean useAec = true;
            int usage = AudioAttributes.USAGE_VOICE_COMMUNICATION;
            int contentType = AudioAttributes.CONTENT_TYPE_SPEECH;

            if(_rec == null)
            {
                _recMinBufferSize = AudioRecord.getMinBufferSize(
                        sampleRate,
                        (channels == 1 ? AudioFormat.CHANNEL_IN_MONO : AudioFormat.CHANNEL_IN_STEREO),
                        AudioFormat.ENCODING_PCM_16BIT);

                _rec = new AudioRecord(source,
                        sampleRate,
                        (channels == 1 ? AudioFormat.CHANNEL_IN_MONO : AudioFormat.CHANNEL_IN_STEREO),
                        AudioFormat.ENCODING_PCM_16BIT,
                        _recMinBufferSize);

                if(_rec == null)
                {
                    throw new Exception("failed to create audio recorder");
                }

                if(useNs)
                {
                    if(NoiseSuppressor.isAvailable())
                    {
                        _noiseSuppressor = NoiseSuppressor.create(_rec.getAudioSessionId());
                        if(_noiseSuppressor == null)
                        {
                            Globals.getLogger().w(TAG, "cannot create noise suppressor for audio recording");
                        }
                        else
                        {
                            _noiseSuppressor.setEnabled(true);
                            if(!_noiseSuppressor.getEnabled())
                            {
                                Globals.getLogger().w(TAG, "cannot enable noise suppressor for audio recording");
                            }
                        }
                    }
                }

                if(useAec)
                {
                    if(AcousticEchoCanceler.isAvailable())
                    {
                        _aec = AcousticEchoCanceler.create(_rec.getAudioSessionId());
                        if(_aec == null)
                        {
                            Globals.getLogger().w(TAG, "cannot create aec for audio recording");
                        }
                        else
                        {
                            _aec.setEnabled(true);
                            if(!_aec.getEnabled())
                            {
                                Globals.getLogger().w(TAG, "cannot enable aec for audio recording");
                            }
                        }
                    }
                }

                _track = new AudioTrack.Builder()
                        .setAudioAttributes(new AudioAttributes.Builder()
                                .setUsage(usage)
                                .setContentType(contentType)
                                .build())
                        .setAudioFormat(new AudioFormat.Builder()
                                .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                                .setSampleRate(sampleRate)
                                .setChannelMask((channels == 1 ? AudioFormat.CHANNEL_OUT_MONO : AudioFormat.CHANNEL_OUT_STEREO))
                                .build())
                        .setSessionId(_rec.getAudioSessionId())
                        .setTransferMode(AudioTrack.MODE_STREAM)
                        .build();

                if(_track == null)
                {
                    throw new Exception("failed to create audio track");
                }

                rc = 0;
            }
            else
            {
                rc = 0;
            }
        }
        catch (Exception e)
        {
            Globals.getLogger().e(TAG, e.getMessage());
            rc = -1;
        }

        return rc;
    }

    public int destroyAudioSubsystem(String jsonParams)
    {
        Globals.getLogger().d(TAG, "destroyAudioSubsystem");

        int rc = 0;

        if(_noiseSuppressor != null)
        {
            _noiseSuppressor.release();
            _noiseSuppressor = null;
        }

        if(_aec != null)
        {
            _aec.release();
            _aec = null;
        }

        if(_rec != null)
        {
            _rec.release();
            _rec = null;
        }

        if(_track != null)
        {
            _track.release();
            _track = null;
        }

        return rc;
    }

    public int startAudioRecording(String jsonParams)
    {
        Globals.getLogger().d(TAG, "startAudioRecording");

        int rc = 0;

        if(_rec != null && _recordingThread == null)
        {
            _recordingThread = new RecordingThread(_rec, _recMinBufferSize);
            _recordingThread.start();
        }

        return rc;
    }

    public int stopAudioRecording(String jsonParams)
    {
        Globals.getLogger().d(TAG, "stopAudioRecording");

        int rc = 0;

        if(_recordingThread != null)
        {
            _recordingThread.close();
            _recordingThread = null;
        }

        return rc;
    }

    public int startAudioPlayout(String jsonParams)
    {
        Globals.getLogger().d(TAG, "startAudioPlayout");

        int rc = 0;

        if(_track != null && _playoutThread == null)
        {
            _playoutThread = new PlayoutThread(_track);
            _playoutThread.start();
        }

        return rc;
    }

    public int stopAudioPlayout(String jsonParams)
    {
        Globals.getLogger().d(TAG, "stopAudioPlayout");

        int rc = 0;

        if(_playoutThread != null)
        {
            _playoutThread.close();
            _playoutThread = null;
        }

        return rc;
    }

    // The recording thread
    private class RecordingThread extends Thread
    {
        final String TAG = MyAudioProvider.class.getSimpleName() + "." + RecordingThread.class.getSimpleName();

        private boolean _running = true;
        private AudioRecord _recorder = null;
        private int _minBufferSizeIn = 0;

        RecordingThread(AudioRecord recorder, int minBufferSizeIn)
        {
            _recorder = recorder;
            _minBufferSizeIn = minBufferSizeIn;
        }

        public void close()
        {
            _running = false;
            try
            {
                join();
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
        }

        public void run()
        {
            Globals.getLogger().d(TAG, "starting recording");

            short[] audioData = new short[_minBufferSizeIn];
            _recorder.startRecording();

            while( _running )
            {
                try
                {
                    int numRead = _recorder.read(audioData, 0, _minBufferSizeIn);
                    if(numRead > 0)
                    {
                        Globals.getEngageApplication().getEngine().writeAndroidAudio(audioData, 0, numRead);
                    }
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                }
            }

            _recorder.stop();

            Globals.getLogger().d(TAG, "ended recording");
        }
    }

    // The playout thread
    private class PlayoutThread extends Thread
    {
        final String TAG = MyAudioProvider.class.getSimpleName() + "." + PlayoutThread.class.getSimpleName();

        private boolean _running = true;
        private AudioTrack _track = null;

        PlayoutThread(AudioTrack track)
        {
            _track = track;
        }

        public void close()
        {
            _running = false;

            try
            {
                join();
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
        }

        public void run()
        {
            final int BUFSZ = 512;
            short[] buf = new short[BUFSZ];
            int numWritten;
            int numLeft;
            int ofs;

            Globals.getLogger().d(TAG, "starting playout");

            _track.play();

            while( _running )
            {
                numLeft = Globals.getEngageApplication().getEngine().readAndroidAudio(buf, 0, BUFSZ);
                if(numLeft > 0)
                {
                    ofs = 0;
                    while(numLeft > 0)
                    {
                        numWritten = _track.write(buf, ofs, numLeft);
                        //Globals.getLogger().d(TAG, "wrote " + numWritten + " samples");
                        numLeft -= numWritten;
                        ofs += numWritten;
                    }
                }
                else
                {
                    Globals.getLogger().d(TAG, "no samples available for speaker");
                    try
                    {
                        Thread.sleep(100);
                    }
                    catch (Exception e)
                    {
                    }
                }
            }

            _track.stop();

            Globals.getLogger().d(TAG, "ended playout");
        }
    }
}
