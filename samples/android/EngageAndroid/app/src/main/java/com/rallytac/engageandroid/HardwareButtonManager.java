package com.rallytac.engageandroid;

import android.content.Context;
import android.os.Build;
import android.util.Log;

public class HardwareButtonManager implements IPushToTalkRequestHandler,
                                              BluetoothManager.IBtNotification
{
    private static final String TAG = HardwareButtonManager.class.getSimpleName();

    private Context _ctx;
    private IPushToTalkRequestHandler _handler;
    private SonimBroadcastReceiver _sonimBroadcastReceiver = null;

    private String pttOn = "+PTT=P";//NON-NLS
    private String pttOff = "+PTT=R";//NON-NLS
    private BluetoothManager _btm;
    private BluetoothManager.IBtNotification _btNotification;

    HardwareButtonManager(Context ctx,
                          IPushToTalkRequestHandler handler,
                          BluetoothManager.IBtNotification btNotification)
    {
        _ctx = ctx;
        _handler = handler;
        _btNotification = btNotification;
    }

    public void start()
    {
        if(Build.MANUFACTURER.toUpperCase().contains("SONIM"))//NON-NLS
        {
            _sonimBroadcastReceiver = new SonimBroadcastReceiver(_ctx, this);
            _sonimBroadcastReceiver.start();
        }

        boolean useBt = Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_BT_DEVICE_USE, false);
        if(useBt)
        {
            String btDeviceAddress = Globals.getSharedPreferences().getString(PreferenceKeys.USER_BT_DEVICE_ADDRESS, null);
            if (!Utils.isEmptyString(btDeviceAddress))
            {
                _btm = new BluetoothManager(_ctx, this, this);
                _btm.start(btDeviceAddress, pttOn, pttOff);
            }
        }
    }

    public void stop()
    {
        if(_btm != null)
        {
            _btm.stop();
            _btm = null;
        }

        if(_sonimBroadcastReceiver != null)
        {
            _sonimBroadcastReceiver.stop();
            _sonimBroadcastReceiver = null;
        }

    }

    @Override
    public void onBluetoothDeviceConnected()
    {
        Log.d(TAG, "onBluetoothDeviceConnected");//NON-NLS
        if(_btNotification != null)
        {
            _btNotification.onBluetoothDeviceConnected();
        }
    }

    @Override
    public void onBluetoothDeviceDisconnected()
    {
        Log.d(TAG, "onBluetoothDeviceDisconnected");//NON-NLS
        if(_btNotification != null)
        {
            _btNotification.onBluetoothDeviceDisconnected();
        }
    }

    @Override
    public void requestPttOn(int priority, int flags)
    {
        _handler.requestPttOn(priority, flags);
    }

    @Override
    public void requestPttOff()
    {
        _handler.requestPttOff();
    }
}
