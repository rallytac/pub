//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

public class MicroCardFragment extends CardFragment
{
    private static String TAG = MicroCardFragment.class.getSimpleName();

    @Override
    protected int getLayoutId()
    {
        return R.layout.fragment_micro_card;
    }

    @Override
    protected int getCardResId(boolean secure)
    {
        return (secure ? R.drawable.ic_multi_channel_background_secure_idle : R.drawable.ic_multi_channel_background_clear_idle);
    }

    @Override
    protected int getCardTxResId(boolean secure)
    {
        return (secure ? R.drawable.ic_multi_channel_background_secure_tx : R.drawable.ic_multi_channel_background_clear_tx);
    }

    @Override
    protected void onCardDoubleTap()
    {
        setSingleMultiPrimaryGroup();
    }

    @Override
    protected void onCardSwiped(GestureDirection direction)
    {
        if(direction == GestureDirection.gdLeft || direction == GestureDirection.gdRight)
        {
            setSingleMultiPrimaryGroup();
        }
    }

    private void setSingleMultiPrimaryGroup()
    {
        if(_gd != null)
        {
            ((SimpleUiMainActivity) getActivity()).setSingleMultiPrimaryGroup(_gd.id);
        }
    }
}
