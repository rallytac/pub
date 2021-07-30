//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.net.wifi.WifiManager;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.os.PowerManager;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;
import android.util.Log;

import com.rallytac.engage.engine.Engine;

public class EngageService extends Service
{
    private final static String TAG = EngageService.class.toString();

    private final static String INTENT_ACTION_WAKEUP = "{26f5802d-cc33-4ee6-b074-c3d5044377a8}";

    private static final int NOTIFICATION_ID = 1;

    private boolean _initialized = false;

    private final IBinder _binder = new EngageServiceBinder();
    private NotificationManager _notificationManager = null;
    private NotificationChannel _notificationChannel = null;

    private MyBroadcastReceiver _br = null;
    private WifiManager _wifiManager = null;
    private WifiManager.WifiLock _wifiLock = null;
    private WifiManager.MulticastLock _multicastLock = null;
    private PowerManager _powerManager = null;
    private PowerManager.WakeLock _wakeLock = null;

    // A general-purpose broadcast receiver
    private class MyBroadcastReceiver extends BroadcastReceiver
    {
        private String[] _requestActions =
                {
                        // Intent actions we want to receive in this service
                        INTENT_ACTION_WAKEUP
                };

        @Override
        public void onReceive(Context context, Intent intent)
        {
            // Ignore any intents if we're not initialized
            if(!_initialized)
            {
                Globals.getLogger().w(TAG, "ignoring intent - not initialized");//NON-NLS
                return;
            }

            String action = intent.getAction();
            if(action == null || action.isEmpty())
            {
                Globals.getLogger().e(TAG, "received empty action!");//NON-NLS
                return;
            }

            Globals.getLogger().i(TAG, "received intent [" + action + "]");//NON-NLS

            // TODO: handle intent actions

            if (action.compareTo(INTENT_ACTION_WAKEUP) == 0)
            {
                Globals.getEngageApplication().wakeup();
            }
            else
            {
                Globals.getLogger().e(TAG, "unhandled request action '" + action + "'");
            }
        }

        public void start()
        {
            for (int x = 0; x < _requestActions.length; x++)
            {
                registerReceiver(this, new IntentFilter(_requestActions[x]));
            }
        }

        public void stop()
        {
            try
            {
                unregisterReceiver(this);
            }
            catch (Exception e)
            {
            }
        }
    }

    public class EngageServiceBinder extends Binder
    {
        EngageService getService()
        {
            return EngageService.this;
        }
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent)
    {
        return _binder;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
        Globals.getLogger().i(TAG, "=====================onStartCommand: intent=" + ((intent != null) ? intent.toString() : "null") + ", flags=" + flags + ", startId=" + startId);//NON-NLS
        super.onStartCommand(intent, flags, startId);

        if(startId == 1)
        {
            showOsNotification(getString(R.string.app_name), String.format(getString(R.string.android_notification_service_is_running), getString(R.string.app_name)), R.drawable.ic_app_logo);
        }

        return START_STICKY;
    }

    @Override
    public void onCreate()
    {
        Globals.getLogger().d(TAG, "onCreate");//NON-NLS
        super.onCreate();

        initializeOsNotifications();
        initializeService();
    }

    @Override
    public void onDestroy()
    {
        Globals.getLogger().d(TAG, "onDestroy");//NON-NLS
        super.onDestroy();

        deinitializeService();
        shutdownOsNotifications();
    }

    private void initializeService()
    {
        if(_initialized)
        {
            Globals.getLogger().w(TAG, "attempt to initialize when already initialized");//NON-NLS
            return;
        }

        try
        {
            _initialized = true;

            _wifiManager = (WifiManager)getApplicationContext().getSystemService(Context.WIFI_SERVICE);

            _wifiLock = _wifiManager.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, BuildConfig.APPLICATION_ID + getString(R.string.wifi_lock_name));
            _wifiLock.setReferenceCounted(false);
            _wifiLock.acquire();

            _multicastLock = _wifiManager.createMulticastLock(BuildConfig.APPLICATION_ID + getString(R.string.multicast_lock_name));
            _multicastLock.setReferenceCounted(false);
            _multicastLock.acquire();

            _powerManager = (PowerManager)getSystemService(Context.POWER_SERVICE);
            _wakeLock = _powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP, BuildConfig.APPLICATION_ID + getString(R.string.wake_lock_name));
            _wakeLock.acquire();

            _br = new MyBroadcastReceiver();
            _br.start();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            deinitializeService();
        }
    }

    private void deinitializeService()
    {
        if (!_initialized)
        {
            return;
        }

        _initialized = false;

        // Shutdown the broadcast receiver
        try
        {
            if(_br != null)
            {
                _br.stop();
                _br = null;
            }
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }

        // Handle issues shutting down the network managers
        try
        {
            if(_multicastLock != null)
            {
                if(_multicastLock.isHeld())
                {
                    _multicastLock.release();
                }

                _multicastLock = null;
            }

            if(_wifiLock != null)
            {
                if(_wifiLock.isHeld())
                {
                    _wifiLock.release();
                }

                _wifiLock = null;
            }

            _wifiManager = null;

            if(_wakeLock != null)
            {
                if(_wakeLock.isHeld())
                {
                    _wakeLock.release();
                }

                _wakeLock = null;
            }

            _powerManager = null;
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    private void initializeOsNotifications()
    {
        try
        {
            if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
            {
                if(_notificationManager == null)
                {
                    _notificationManager = getSystemService(NotificationManager.class);
                }

                if(_notificationChannel == null)
                {
                    _notificationChannel = new NotificationChannel(
                            BuildConfig.APPLICATION_ID + getString(R.string.android_notification_channel_id),
                            BuildConfig.APPLICATION_ID + getString(R.string.android_notification_channel_name),
                            NotificationManager.IMPORTANCE_HIGH);

                    _notificationChannel.setDescription(String.format(getString(R.string.android_notitication_channel_description),getString(R.string.app_name)));
                    _notificationChannel.enableLights(true);
                    _notificationChannel.setLightColor(Color.RED);
                    _notificationChannel.setShowBadge(true);

                    _notificationManager.createNotificationChannel(_notificationChannel);
                }
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    private void shutdownOsNotifications()
    {
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
        {
            if(_notificationManager != null)
            {
                _notificationManager.cancelAll();
                _notificationManager.deleteNotificationChannel(BuildConfig.APPLICATION_ID + getString(R.string.android_notification_channel_id));
            }
        }
    }

    private void showOsNotification(String title, String msg, int iconId)
    {
        try
        {
            Notification notification = new NotificationCompat.Builder(this, BuildConfig.APPLICATION_ID + getString(R.string.android_notification_channel_id))
                    .setContentTitle(title)
                    .setContentText(msg)
                    .setSmallIcon(iconId)
                    .build();

            notification.flags |= (Notification.FLAG_ONGOING_EVENT | Notification.FLAG_AUTO_CANCEL);

            Intent i = new Intent(INTENT_ACTION_WAKEUP);
            PendingIntent pendingIntent = PendingIntent.getBroadcast(this, 0, i, 0);
            notification.contentIntent = pendingIntent;

            if(_notificationManager != null)
            {
                _notificationManager.notify(NOTIFICATION_ID, notification);
            }

            startForeground(NOTIFICATION_ID, notification);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
}
