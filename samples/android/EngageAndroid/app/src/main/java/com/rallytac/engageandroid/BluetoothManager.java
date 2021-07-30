//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Set;
import java.util.UUID;

public class BluetoothManager
{
    private static String TAG = BluetoothManager.class.getSimpleName();

    public interface IBtNotification
    {
        void onBluetoothDeviceConnected();
        void onBluetoothDeviceDisconnected();
    }

    private static final UUID BT_UUID_SECURE = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");//NON-NLS
    private static final UUID BT_UUID_NON_SECURE = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");//NON-NLS

    private Context _ctx;
    private String _desiredDeviceAddress = null;
    private String _pttOnString = null;
    private String _pttOffString = null;
    private BluetoothAdapter _bluetoothAdapter = null;
    private BluetoothHeadset _bluetoothHeadset = null;
    private BluetoothProfile.ServiceListener _profileListener = null;
    private ReaderThread _readerThread = null;
    private MyBluetoothBroadcastReceiver _receiver = new MyBluetoothBroadcastReceiver();
    private IBtNotification _notificationSink;
    private IPushToTalkRequestHandler _pttRequestHandler;

    public BluetoothManager(Context ctx,
                            IBtNotification notificationSink,
                            IPushToTalkRequestHandler pttRequestHandler)
    {
        _ctx = ctx;
        _notificationSink = notificationSink;
        _pttRequestHandler = pttRequestHandler;
    }

    private class MyBluetoothBroadcastReceiver extends BroadcastReceiver
    {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);

            if(device != null)
            {
                if(device.getAddress().compareTo(_desiredDeviceAddress) == 0)
                {
                    if (BluetoothDevice.ACTION_ACL_CONNECTED.equals(action))
                    {
                        Globals.getLogger().d(TAG, "BT connected for the device we desire");//NON-NLS
                        connectToBluetoothDevice(device);
                    }
                    else if (BluetoothDevice.ACTION_ACL_DISCONNECTED.equals(action))
                    {
                        Globals.getLogger().d(TAG, "BT disconnected for the device we desire");//NON-NLS
                        disconnectFromBluetoothDevice();
                    }
                }
                else
                {
                    Globals.getLogger().d(TAG, "BT connected but not the device we want");//NON-NLS
                }
            }
            else
            {
                Globals.getLogger().d(TAG, "BT connected but no device provided");//NON-NLS
            }
        }
    }

    private BluetoothDevice getDeviceByAddress(String address)
    {
        BluetoothDevice rc = null;

        try
        {
            if(_bluetoothAdapter.isEnabled())
            {
                Set<BluetoothDevice> bondedDevices = _bluetoothAdapter.getBondedDevices();
                if (bondedDevices.size() > 0)
                {
                    for (BluetoothDevice device : bondedDevices)
                    {
                        if(device.getAddress().compareTo(address) == 0)
                        {
                            rc = device;
                            break;
                        }
                    }
                }
            }
        }
        catch (Exception e)
        {
            rc = null;
        }

        return rc;
    }

    private boolean isDeviceConnected(String address)
    {
        boolean rc = false;

        try
        {
            BluetoothDevice device = getDeviceByAddress(address);
            if(device != null)
            {
                BluetoothSocket tmp = device.createRfcommSocketToServiceRecord(BT_UUID_SECURE);
                tmp.close();
                rc = true;
            }
        }
        catch (Exception e)
        {
            rc = false;
        }

        return rc;
    }

    public void start(String desiredDeviceAddress, String pttOnString, String pttOffString)
    {
        _bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if(_bluetoothAdapter == null)
        {
            Globals.getLogger().e(TAG, "---------------------------BT: device does not support bluetooth");//NON-NLS
            return;
        }

        _desiredDeviceAddress = desiredDeviceAddress;
        _pttOnString = pttOnString;
        _pttOffString = pttOffString;

        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECT_REQUESTED);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        _ctx.registerReceiver(_receiver, filter);

        _profileListener = new BluetoothProfile.ServiceListener()
        {
            public void onServiceConnected(int profile, BluetoothProfile proxy)
            {
                if (profile == BluetoothProfile.HEADSET)
                {
                    Globals.getLogger().d(TAG, "---------------------------BT: onServiceConnected : HEADSET");//NON-NLS
                    _bluetoothHeadset = (BluetoothHeadset) proxy;
                }
                else
                {
                    Globals.getLogger().e(TAG, "---------------------------BT: onServiceConnected : OTHER");//NON-NLS
                }
            }

            public void onServiceDisconnected(int profile)
            {
                if (profile == BluetoothProfile.HEADSET)
                {
                    Globals.getLogger().d(TAG, "---------------------------BT: onServiceDisconnected : HEADSET");//NON-NLS
                    _bluetoothHeadset = null;
                }
                else
                {
                    Globals.getLogger().e(TAG, "---------------------------BT: onServiceDisconnected : OTHER");//NON-NLS
                }
            }
        };

        _bluetoothAdapter.getProfileProxy(_ctx, _profileListener, BluetoothProfile.HEADSET);

        if(isDeviceConnected(desiredDeviceAddress))
        {
            connectToBluetoothDevice(desiredDeviceAddress);
        }
    }

    public void stop()
    {
        if(_receiver != null)
        {
            _ctx.unregisterReceiver(_receiver);
            _receiver = null;
        }

        disconnectFromBluetoothDevice();

        if(_bluetoothAdapter != null)
        {
            if(_bluetoothHeadset != null)
            {
                _bluetoothAdapter.closeProfileProxy(BluetoothProfile.HEADSET, _bluetoothHeadset);
                _bluetoothHeadset = null;
            }

            _bluetoothAdapter = null;
        }
    }

    public static void enableBluetoothRecording(Context ctx)
    {
        AudioManager audioManager = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);

        if(audioManager != null)
        {
            audioManager.setMode(AudioManager.MODE_IN_COMMUNICATION);
            audioManager.setBluetoothScoOn(true);
            audioManager.startBluetoothSco();

            Globals.getLogger().d(TAG, "enableBluetoothRecording: bluetooth sco enabled for audio capture");//NON-NLS
        }
        else
        {
            Globals.getLogger().e(TAG, "enableBluetoothRecording: failed to acquire audio manager");//NON-NLS
        }
    }

    public static void disableBluetoothRecording(Context ctx)
    {
        AudioManager audioManager = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);

        if(audioManager != null)
        {
            audioManager.stopBluetoothSco();
            audioManager.setBluetoothScoOn(false);
            audioManager.setMode(AudioManager.MODE_NORMAL);

            Globals.getLogger().d(TAG, "disableBluetoothRecording: bluetooth sco disabled for audio capture");//NON-NLS
        }
        else
        {
            Globals.getLogger().e(TAG, "disableBluetoothRecording: failed to acquire audio manager");//NON-NLS
        }
    }

    public static ArrayList<BluetoothDevice> getDevices()
    {
        ArrayList<BluetoothDevice> rc = new ArrayList<>();

        BluetoothAdapter bta = BluetoothAdapter.getDefaultAdapter();

        if(bta != null)
        {
            Set<BluetoothDevice> pairedDevices = bta.getBondedDevices();

            if (pairedDevices.size() > 0)
            {
                for (BluetoothDevice device : pairedDevices)
                {
                    rc.add(device);
                }
            }
        }

        return rc;
    }

    public static String getAddressOfDeviceByName(String nm)
    {
        String rc = null;
        ArrayList<BluetoothDevice> devs = getDevices();

        for (BluetoothDevice device : devs)
        {
            if(device.getName().compareTo(nm) == 0)
            {
                rc = device.getAddress();
                break;
            }
        }

        return rc;
    }

    private void connectToBluetoothDevice(String address)
    {
        connectToBluetoothDevice(getDeviceByAddress(address));
    }

    private void connectToBluetoothDevice(BluetoothDevice device)
    {
        /*
        if(device != null)
        {
            if(_readerThread == null)
            {
                _readerThread = new ReaderThread(device);
                _readerThread.start();
            }
        }
         */
    }

    private void disconnectFromBluetoothDevice()
    {
        /*
        try
        {
            if(_readerThread != null)
            {
                _readerThread.cancel();
                _readerThread.join();
                _readerThread = null;
            }
        }
        catch (Exception e)
        {
        }

        _readerThread = null;
         */
    }

    private class ReaderThread extends Thread
    {
        private BluetoothSocket _socket;
        private BluetoothDevice _device;
        private boolean _running = true;
        private Object _wakeup = new Object();

        public ReaderThread(BluetoothDevice device)
        {
            _device = device;
        }

        public void run()
        {
            InputStream is;
            byte[] buffer = new byte[1024];
            int bytesRead;
            long timeToWait = 0;

            //_bluetoothAdapter.cancelDiscovery();

            while(_running)
            {
                try
                {
                    Globals.getLogger().d(TAG, "connecting to " + _device.toString());//NON-NLS
                    _socket = _device.createRfcommSocketToServiceRecord(BT_UUID_SECURE);
                    _socket.connect();
                    is = _socket.getInputStream();
                }
                catch (IOException connectException)
                {
                    connectException.printStackTrace();

                    // Unable to connect; close the socket and return.
                    try
                    {
                        _socket.close();
                    }
                    catch (IOException closeException)
                    {
                        Globals.getLogger().e(TAG, "could not close the client socket : " + closeException);//NON-NLS
                        closeException.printStackTrace();
                    }

                    try
                    {
                        timeToWait += 500;
                        if(timeToWait > 5000)
                        {
                            timeToWait = 5000;
                        }

                        synchronized (_wakeup)
                        {
                            _wakeup.wait(timeToWait);
                        }
                    }
                    catch (Exception e)
                    {
                        e.printStackTrace();
                    }

                    continue;
                }

                if(!_running)
                {
                    break;
                }

                timeToWait = 0;

                Globals.getLogger().d(TAG, "connected to " + _device.toString());//NON-NLS
                _notificationSink.onBluetoothDeviceConnected();

                while(_running)
                {
                    try
                    {
                        bytesRead = is.read(buffer);
                        if(bytesRead > 0)
                        {
                            if(_running)
                            {
                                buffer[bytesRead] = 0;
                                String s = new String(buffer, 0, bytesRead, StandardCharsets.UTF_8);
                                Globals.getLogger().d(TAG, "read " + bytesRead + " bytes [" + s + "]");//NON-NLS//NON-NLS

                                if(s.compareTo(_pttOnString) == 0)
                                {
                                    _pttRequestHandler.requestPttOn(-1, -1);
                                }
                                else if(s.compareTo(_pttOffString) == 0)
                                {
                                    _pttRequestHandler.requestPttOff();
                                }
                            }
                        }
                    }
                    catch (Exception e)
                    {
                        e.printStackTrace();
                        break;
                    }
                }

                Globals.getLogger().d(TAG, "disconnected from " + _device.toString());//NON-NLS
                _notificationSink.onBluetoothDeviceDisconnected();
            }
        }

        public void cancel()
        {
            _running = false;
            synchronized (_wakeup)
            {
                _wakeup.notifyAll();
            }

            try
            {
                _socket.close();
            }
            catch (IOException e)
            {
            }
        }
    }
}
