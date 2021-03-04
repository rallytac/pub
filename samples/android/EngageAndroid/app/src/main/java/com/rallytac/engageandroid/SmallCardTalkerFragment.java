//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

public class SmallCardTalkerFragment extends TalkerFragment
{
    @Override
    public int getContainerResourceId()
    {
        return R.layout.fragment_talker_list_small_card;
    }

    @Override
    public int getItemResourceId()
    {
        return R.layout.fragment_talker_item_small_card;
    }
}
