//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

public class GroupMembershipTracker
{
    GroupMembershipTracker(String nodeId, String groupId, int statusFlags)
    {
        _nodeId = nodeId;
        _groupId = groupId;
        _statusFlags = statusFlags;
    }

    public String _nodeId;
    public String _groupId;
    public int _statusFlags;
}
