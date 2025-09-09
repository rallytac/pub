//
//  Copyright (c) 2020 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

public class EngageAppPermission
{
    EngageAppPermission(String p, boolean r)
    {
        _permission = p;
        _required = r;
        _granted = false;
    }

    public String getPermission()
    {
        return _permission;
    }

    public boolean getRequired()
    {
        return _required;
    }

    public void setGranted(boolean g)
    {
        _granted = g;
    }

    public boolean getGranted()
    {
        return _granted;
    }

    private String _permission;
    private boolean _required;
    private boolean _granted;
}
