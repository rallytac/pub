//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.os.Build;

import java.util.ArrayList;
import java.util.Set;
import java.util.UUID;

public class MyBleReader extends BluetoothGattCallback
{
    public interface EventListener
    {
        void onBleDeviceConnected(BluetoothDevice device);
        void onBleDeviceDisconnected(BluetoothDevice device);
        void onBleDataReceived(BluetoothDevice device, byte[] data);
    };

    private static final String TAG = MyBleReader.class.getSimpleName();

    private final UUID BLE_CHARACTERISTIC_READ = UUID.fromString("0000ffe1-0000-1000-8000-00805f9b34fb");

    static public ArrayList<BluetoothDevice> getBleDevices()
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
                    if(device.getType() == BluetoothDevice.DEVICE_TYPE_LE)
                    {
                        rc.add(device);
                    }
                }
            }
        }

        return rc;
    }

    private Context _ctx = null;
    private BluetoothDevice _device = null;
    private EventListener _eventListener = null;
    private BluetoothGatt _gatt = null;
    private GattCallbackHandler _gattCallbackHandler = null;

    public MyBleReader(Context ctx, BluetoothDevice device, EventListener eventListener)
    {
        _ctx = ctx;
        _device = device;
        _eventListener = eventListener;
    }

    public void start()
    {
        _gattCallbackHandler = new GattCallbackHandler();
        connect();
    }

    public void stop()
    {
        if(_gatt != null)
        {
            _gatt.close();
            _gatt = null;
        }

        _gattCallbackHandler = null;
    }

    private void connect()
    {
        if (Build.VERSION.SDK_INT < 23)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":connectGatt");
            _gatt = _device.connectGatt(_ctx, true, _gattCallbackHandler);
        }
        else
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":connectGatt,LE");
            _gatt = _device.connectGatt(_ctx, true, _gattCallbackHandler, BluetoothDevice.TRANSPORT_LE);
        }
    }

    private class GattCallbackHandler extends BluetoothGattCallback
    {
        private int MAX_MTU = 512;
        private final String TAG = GattCallbackHandler.class.getSimpleName();

        @Override
        public void onPhyUpdate(BluetoothGatt gatt, int txPhy, int rxPhy, int status)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":onPhyUpdate");
        }

        @Override
        public void onPhyRead(BluetoothGatt gatt, int txPhy, int rxPhy, int status)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":onPhyRead");
        }

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":onConnectionStateChange");

            if (newState == BluetoothProfile.STATE_CONNECTED)
            {
                Globals.getLogger().d(TAG,_device.getName() +  ":connect status STATE_CONNECTED");
                _eventListener.onBleDeviceConnected(_device);

                // TODO: Check return from gatt.discoverServices
                if(!_gatt.discoverServices())
                {
                    Globals.getLogger().e(TAG, _device.getName() +  ":discoverServices failed");
                }
            }
            else if (newState == BluetoothProfile.STATE_DISCONNECTED)
            {
                Globals.getLogger().d(TAG,_device.getName() +  ":connect status STATE_DISCONNECTED");
                _eventListener.onBleDeviceDisconnected(_device);
            }
            else
            {
                Globals.getLogger().d(TAG, _device.getName() +  ":unknown connect state " + newState + " " + status);
            }
        }


        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":onServicesDiscovered");

            BluetoothGattCharacteristic readCharacteristic = null;

            for (BluetoothGattService gattService : gatt.getServices())
            {
                readCharacteristic = gattService.getCharacteristic(BLE_CHARACTERISTIC_READ);
                if(readCharacteristic != null)
                {
                    if(!_gatt.setCharacteristicNotification(readCharacteristic, true))
                    {
                        Globals.getLogger().e(TAG, _device.getName() +  ":setCharacteristicNotification failed");
                    }
                }
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic,
                                         int status)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":onCharacteristicRead");
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":onCharacteristicWrite");
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":onCharacteristicChanged");
            byte[] data = characteristic.getValue();
            if(data != null)
            {
                Globals.getLogger().d(TAG, _device.getName() +  ":data length = " + data.length);
                _eventListener.onBleDataReceived(_device, data);
            }
            else
            {
                Globals.getLogger().e(TAG, _device.getName() +  ":data is null");
            }
        }

        @Override
        public void onDescriptorRead(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":onDescriptorRead");
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":onDescriptorWrite");
        }

        @Override
        public void onReliableWriteCompleted(BluetoothGatt gatt, int status)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":onReliableWriteCompleted");
        }

        @Override
        public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":onReadRemoteRssi");
        }

        @Override
        public void onMtuChanged(BluetoothGatt gatt, int mtu, int status)
        {
            Globals.getLogger().d(TAG, _device.getName() +  ":onMtuChanged: mtu=" + mtu + ", status=" + status);
        }
    }
}
