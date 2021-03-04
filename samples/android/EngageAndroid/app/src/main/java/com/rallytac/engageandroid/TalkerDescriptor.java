//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

public class TalkerDescriptor
{
    public String alias;
    public String nodeId;
    public long rxFlags;
    public int txPriority;

    @Override
    public String toString()
    {
        StringBuilder sb = new StringBuilder();

        sb.append("alias=" + alias);
        sb.append(", nodeId=" + nodeId);
        sb.append(", rxFlags=" + rxFlags);
        sb.append(", txPriority=" + txPriority);

        return sb.toString();
    }
}
