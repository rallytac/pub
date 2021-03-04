//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

public class LargeCardTalkerFragment extends TalkerFragment
{
    @Override
    public int getContainerResourceId()
    {
        return R.layout.fragment_talker_list_large_card;
    }

    @Override
    public int getItemResourceId()
    {
        return R.layout.fragment_talker_item_large_card;
    }
}
