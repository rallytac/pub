//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Context;
import android.location.Location;
import android.os.Looper;
import androidx.annotation.NonNull;
import android.util.Log;

import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationResult;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;

public class LocationManager
{
    private static String TAG = LocationManager.class.getSimpleName();

    public interface ILocationUpdateNotifications
    {
        void onLocationUpdated(Location loc);
    }

    private Context _ctx;
    private FusedLocationProviderClient _fusedLocationProviderClient;
    private ILocationUpdateNotifications _notificationSubscriber;
    private LocationCallback _cb;
    private LocationRequest _req;

    public static final int PRIORITY_HIGH_ACCURACY = LocationRequest.PRIORITY_HIGH_ACCURACY;
    public static final int PRIORITY_BALANCED_POWER_ACCURACY = LocationRequest.PRIORITY_BALANCED_POWER_ACCURACY;
    public static final int PRIORITY_LOW_POWER = LocationRequest.PRIORITY_LOW_POWER;
    public static final int PRIORITY_NO_POWER = LocationRequest.PRIORITY_NO_POWER;

    private int _priority;
    private int _minIntervalMs;
    private int _intervalMs;
    private float _minDisplacement;

    public LocationManager(Context ctx,
                           ILocationUpdateNotifications notificationSubscriber,
                           int priority,
                           int intervalMs,
                           int minIntervalMs,
                           float minDisplacement)
    {
        _ctx = ctx;
        _notificationSubscriber = notificationSubscriber;
        _priority = priority;
        _minIntervalMs = minIntervalMs;
        _intervalMs = intervalMs;
        _minDisplacement = minDisplacement;
    }

    public void start()
    {
        try
        {
            _fusedLocationProviderClient = LocationServices.getFusedLocationProviderClient(_ctx);

            requestLastKnownLocation();

            _cb = new LocationCallback()
            {
                @Override
                public void onLocationResult(LocationResult locationResult) {
                    for (Location location : locationResult.getLocations())
                    {
                        Globals.getLogger().d(TAG, "locationUpdate: " + location.toString());//NON-NLS
                        onLocationUpdated(location);
                    }
                }
            };

            _req = LocationRequest.create();
            _req.setPriority(_priority);
            _req.setInterval(_intervalMs);
            _req.setFastestInterval(_minIntervalMs);
            _req.setMaxWaitTime(_minIntervalMs);
            _req.setSmallestDisplacement(_minDisplacement);

            _fusedLocationProviderClient.requestLocationUpdates(_req, _cb, Looper.getMainLooper());
        }
        catch(SecurityException se)
        {
            se.printStackTrace();
            stop();
        }
        catch(Exception e)
        {
            e.printStackTrace();
            stop();
        }
    }

    public void stop()
    {
        if(_cb != null)
        {
            _fusedLocationProviderClient.removeLocationUpdates(_cb);
            _cb = null;
        }

        _fusedLocationProviderClient = null;
    }

    private void onLocationUpdated(Location location)
    {
        //Globals.getLogger().d(TAG, "processNewLocationData " + location.toString());

        if(_notificationSubscriber != null)
        {
            _notificationSubscriber.onLocationUpdated(location);
        }
    }

    public void requestLastKnownLocation()
    {
        try
        {
            _fusedLocationProviderClient.getLastLocation().addOnCompleteListener(new OnCompleteListener<Location>()
             {
                 @Override
                 public void onComplete(@NonNull Task<Location> task)
                 {
                     if (task.isSuccessful() && task.getResult() != null)
                     {
                         Location location = task.getResult();

                         //Globals.getLogger().d(TAG, "onLicenseActivationTaskComplete success : " + location.toString());
                         onLocationUpdated(location);
                     }
                     else
                     {
                         Globals.getLogger().e(TAG, "onLicenseActivationTaskComplete failure");//NON-NLS
                     }
                 }
             }
            );
        }
        catch (SecurityException se)
        {
            se.printStackTrace();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
}
