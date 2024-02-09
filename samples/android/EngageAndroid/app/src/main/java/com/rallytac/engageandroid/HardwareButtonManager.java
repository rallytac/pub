//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.os.Build;

import java.util.ArrayList;
import java.util.Arrays;

public class HardwareButtonManager implements IPushToTalkRequestHandler,
                                              BluetoothManager.IBtNotification,
                                              MyBleReader.EventListener
{
    private static final String TAG = HardwareButtonManager.class.getSimpleName();

    private Context _ctx;
    private IPushToTalkRequestHandler _handler;
    private SonimBroadcastReceiver _sonimBroadcastReceiver = null;
    private HardwarePttKeyEventIntentBroadcastReceiver _hardwarePttKeyEventIntentBroadcastReceiver = null;

    private String pttOn = "+PTT=P";//NON-NLS
    private String pttOff = "+PTT=R";//NON-NLS
    private BluetoothManager _btm = null;
    private MyBleReader _mybler = null;
    private BluetoothManager.IBtNotification _btNotification;

    private static final byte[] PRYME_BLE_PTT_ON = {0x01};
    private static final byte[] PRYME_BLE_PTT_OFF = {0x00};

    private byte[] BT_PTT_ON = null;
    private byte[] BT_PTT_OFF = null;

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
        String filterActionName = FlavorSpecific.getHardwarePttKeyEventIntentFilterActionName();
        if(!Utils.isEmptyString(filterActionName))
        {
            _hardwarePttKeyEventIntentBroadcastReceiver = new HardwarePttKeyEventIntentBroadcastReceiver(_ctx, this, filterActionName);
            _hardwarePttKeyEventIntentBroadcastReceiver.start();
        }
        else
        {
            if(Build.MANUFACTURER.toUpperCase().contains("SONIM"))//NON-NLS
            {
                _sonimBroadcastReceiver = new SonimBroadcastReceiver(_ctx, this);
                _sonimBroadcastReceiver.start();
            }
        }

        /*
        boolean useBt = Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_BT_PTT_USE, false);
        if(useBt)
        {
            String btDeviceAddress = Globals.getSharedPreferences().getString(PreferenceKeys.USER_BT_PTT_ADDRESS, null);
            if (!Utils.isEmptyString(btDeviceAddress))
            {
                _btm = new BluetoothManager(_ctx, this, this);
                _btm.start(btDeviceAddress, pttOn, pttOff);
            }
        }
        */

        // TODO: This is dreadful!  Need a cleaner and more generic way to support BTLE buttons
        if(Globals.getEngageApplication().getResources().getBoolean(R.bool.opt_support_pryme_btle_ptt_button))
        {
            ArrayList<BluetoothDevice> btDevices = MyBleReader.getBleDevices();
            if(btDevices != null)
            {
                for(BluetoothDevice dev : btDevices)
                {
                    if(dev.getName().compareTo("PTT-Z") == 0)       // Pryme name their BLE PTT button 'PTT-Z'
                    {
                        BT_PTT_ON = PRYME_BLE_PTT_ON;
                        BT_PTT_OFF = PRYME_BLE_PTT_OFF;

                        _mybler = new MyBleReader(_ctx, dev, this);
                        _mybler.start();

                        break;
                    }
                }
            }
        }
    }

    public void stop()
    {
        if(_mybler != null)
        {
            _mybler.stop();
            _mybler = null;
        }

        if(_btm != null)
        {
            _btm.stop();
            _btm = null;
        }

        if(_hardwarePttKeyEventIntentBroadcastReceiver != null)
        {
            _hardwarePttKeyEventIntentBroadcastReceiver.stop();
            _hardwarePttKeyEventIntentBroadcastReceiver = null;
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
        Globals.getLogger().d(TAG, "onBluetoothDeviceConnected");//NON-NLS
        if(_btNotification != null)
        {
            _btNotification.onBluetoothDeviceConnected();
        }
    }

    @Override
    public void onBluetoothDeviceDisconnected()
    {
        Globals.getLogger().d(TAG, "onBluetoothDeviceDisconnected");//NON-NLS
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

    // MyBleReader events
    @Override
    public void onBleDeviceConnected(BluetoothDevice device)
    {
        Globals.getLogger().d(TAG, "onBleDeviceConnected " + device.getName());
        onBluetoothDeviceConnected();
    }

    @Override
    public void onBleDeviceDisconnected(BluetoothDevice device)
    {
        Globals.getLogger().d(TAG, "onBleDeviceDisconnected " + device.getName());
        onBluetoothDeviceDisconnected();

    }

    @Override
    public void onBleDataReceived(BluetoothDevice device, byte[] data)
    {
        Globals.getLogger().d(TAG, "onBleDataReceived: " + device.getName() + ", data=[" + Utils.toHexString(data) + "]");
        if(data != null && data.length >= 1)
        {
            if(Arrays.equals(data, BT_PTT_ON))
            {
                requestPttOn(0, 0);
            }
            else if(Arrays.equals(data, BT_PTT_OFF))
            {
                requestPttOff();
            }
            else
            {
                Globals.getLogger().w(TAG, "unhandled data received from BLE device " + device.getName() + ", data=[" + Utils.toHexString(data) + "]");
            }
        }
    }
}
