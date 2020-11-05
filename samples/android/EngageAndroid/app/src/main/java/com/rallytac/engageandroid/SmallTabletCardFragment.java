//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

public class SmallTabletCardFragment extends SmallCardFragment
{
    private static String TAG = SmallTabletCardFragment.class.getSimpleName();

    @Override
    protected int getLayoutId()
    {
        return R.layout.fragment_small_card;
        //return -1;
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
        goToSingleViewForThisCard();
    }

    @Override
    protected void onCardSwiped(GestureDirection direction)
    {
        if(direction == GestureDirection.gdLeft || direction == GestureDirection.gdRight)
        {
            goToSingleViewForThisCard();
        }
    }

    private void goToSingleViewForThisCard()
    {
        if(_gd != null)
        {
            ((SimpleUiMainActivity) getActivity()).showSingleView(_gd.id);
        }
    }
}
