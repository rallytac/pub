//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

public class SmallCardFragmentForTextMessaging extends CardFragment
{
    private static String TAG = SmallCardFragmentForTextMessaging.class.getSimpleName();

    @Override
    protected int getLayoutId()
    {
        return R.layout.fragment_small_card_for_text_messaging;
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
        goToMultiView();
    }

    @Override
    protected void onCardSwiped(GestureDirection direction)
    {
        if(direction == GestureDirection.gdLeft || direction == GestureDirection.gdRight)
        {
            goToMultiView();
        }
    }

    private void goToMultiView()
    {
        ((SimpleUiMainActivity) getActivity()).showMultiView();
    }
}
