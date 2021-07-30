//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Context;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.os.Build;
import android.util.Log;

import java.util.HashMap;

public class AudioPlayerManager
{
    private static String TAG = AudioPlayerManager.class.getSimpleName();

    interface IPlayCompleteListener
    {
        void onAudioPlayCompleted(int id, Object tag);
    }

    private Context _ctx;
    private HashMap<Integer, MediaPlayer> _cache = new HashMap<>();

    AudioPlayerManager(Context ctx)
    {
        _ctx = ctx;
    }

    public void start()
    {
    }

    public void stop()
    {
        clear();
    }

    private float scaleVolume(int minIndex, int maxIndex, int index)
    {
        int MAX_VOLUME = ((maxIndex - minIndex) + 1);
        float l1 = (float)(Math.log(MAX_VOLUME - index) / Math.log(MAX_VOLUME));
        return (1 - l1);
    }

    private float getVolumeForUserNotifications()
    {
        AudioManager audioManager = (AudioManager) _ctx.getSystemService(Context.AUDIO_SERVICE);
        int minLevel;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P)
        {
            minLevel = audioManager.getStreamMinVolume(AudioManager.STREAM_NOTIFICATION);
        }
        else
        {
            minLevel = 0;
        }

        int maxLevel = audioManager.getStreamMaxVolume(AudioManager.STREAM_NOTIFICATION);
        int notificationLevel = audioManager.getStreamVolume(AudioManager.STREAM_NOTIFICATION);

        return scaleVolume(minLevel, maxLevel, notificationLevel);
    }

    public void playNotification(final int id, float volume, Runnable onPlayComplete) throws Exception
    {
        if(volume < 0.0)
        {
            volume = getVolumeForUserNotifications();
        }

        playResource(id, volume, volume, null, null, onPlayComplete);
    }

    public void playResource(final int id, Runnable onPlayComplete) throws Exception
    {
        playResource(id, (float)1.0, (float)1.0, null, null, onPlayComplete);
    }

    int _currentPlayResourceId = -1;

    private void playResource(final int id, float leftVolume, float rightVolume, final IPlayCompleteListener pcl, final Object tag, final Runnable onPlayComplete) throws Exception
    {
        // TODO: update playResource to be smart about playing the same tone as it's already playing - requires some rework for PTT tone and driving txUnmute
        /*
        if(id == _currentPlayResourceId)
        {
            Globals.getLogger().i(TAG, "playResource: play requested for already-playing resource - ignoring");//NON-NLS
            return;
        }
        */

        Globals.getLogger().d(TAG, "playResource: play starting id=" + id + ", left=" + leftVolume + ", right=" + rightVolume);//NON-NLS

        final MediaPlayer p = getPlayer(id);
        if(p != null)
        {
            p.setVolume(leftVolume, rightVolume);

            p.setOnCompletionListener(new MediaPlayer.OnCompletionListener()
            {
                @Override
                public void onCompletion(MediaPlayer mp)
                {
                    Globals.getLogger().d(TAG, "playResource: play complete id=" + id);//NON-NLS
                    if(id == _currentPlayResourceId)
                    {
                        _currentPlayResourceId = -1;
                    }

                    p.reset();
                    p.release();

                    if(pcl != null)
                    {
                        pcl.onAudioPlayCompleted(id, tag);
                    }

                    if(onPlayComplete != null)
                    {
                        Globals.getEngageApplication().runOnUiThread(onPlayComplete);
                    }
                }
            });

            _currentPlayResourceId = id;
            p.start();
        }
        else
        {
            Globals.getLogger().e(TAG, "playResource: cannot obtain a media player for resource " + id);//NON-NLS
            throw new Exception("cannot obtain a media player for resource " + id);
        }
    }

    private void clear()
    {
        synchronized (_cache)
        {
            for(MediaPlayer p : _cache.values())
            {
                p.stop();
                p.release();
            }

            _cache.clear();
        }
    }

    // TODO: use the cache for audio players!
    private MediaPlayer getPlayer(int id)
    {
        return MediaPlayer.create(_ctx, id);

        /*
        MediaPlayer ret = null;

        synchronized (_cache)
        {
            Object obj = _cache.get(id);
            if(obj == null)
            {
                ret = MediaPlayer.create(_ctx, id);
                _cache.put(id, ret);
            }
            else
            {
                ret = (MediaPlayer)obj;
                ret.stop();
                ret.prepare();
                ret.reset();
            }
        }

        return ret;
        */
    }
}
