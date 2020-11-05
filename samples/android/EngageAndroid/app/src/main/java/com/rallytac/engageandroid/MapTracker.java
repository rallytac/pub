//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.Marker;

public class MapTracker
{
    public PresenceDescriptor  _pd = null;
    public Marker _marker = null;
    public boolean _gone = false;
    public boolean _removeFromMap = false;
    public LatLng _latLng = null;
    public LatLng _lastLatLng = null;
    public boolean _locationChanged = false;
    public String _title = null;
}
