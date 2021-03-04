//
//  Copyright (c) 2020 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

public class EngageEntity
{
    EngageEntity(String nid, String fn, int stat)
    {
        nodeId = nid;
        friendlyName = fn;
        status = stat;
    }

    public String nodeId;
    public String friendlyName;
    public int status;
}
