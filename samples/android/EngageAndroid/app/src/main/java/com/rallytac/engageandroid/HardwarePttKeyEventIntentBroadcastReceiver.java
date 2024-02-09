//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.view.KeyEvent;

public class HardwarePttKeyEventIntentBroadcastReceiver extends BroadcastReceiver
{
    private static String TAG = HardwarePttKeyEventIntentBroadcastReceiver.class.getSimpleName();

    private Context _ctx;
    private IPushToTalkRequestHandler _pttRequestHandler;
    private String _filterActionName = null;

    public HardwarePttKeyEventIntentBroadcastReceiver(Context ctx, IPushToTalkRequestHandler handler, String filterActionName)
    {
        _ctx = ctx;
        _pttRequestHandler = handler;
        _filterActionName = filterActionName;
    }

    public void start()
    {
        IntentFilter intentFilter;

        intentFilter = new IntentFilter(_filterActionName);
        intentFilter.setPriority(999);
        _ctx.registerReceiver(this, intentFilter);
    }

    public void stop()
    {
        _ctx.unregisterReceiver(this);
    }

    @Override
    public void onReceive(Context context, Intent intent)
    {
        Globals.getLogger().d(TAG, "onReceive: " + intent.toString());//NON-NLS

        Bundle extras = intent.getExtras();
        if(extras != null)
        {
            KeyEvent ke = (KeyEvent) extras.get(Intent.EXTRA_KEY_EVENT);
            if(ke != null)
            {
                if(ke.getAction() == KeyEvent.ACTION_DOWN)
                {
                    _pttRequestHandler.requestPttOn(-1, -1);
                }
                else if(ke.getAction() == KeyEvent.ACTION_UP)
                {
                    _pttRequestHandler.requestPttOff();
                }
                else
                {
                    Globals.getLogger().e(TAG, "Unhandled notification: " + intent.toString());//NON-NLS
                }
            }

            for (String key : extras.keySet())
            {
                Globals.getLogger().i(TAG, key + " : " + (extras.get(key) != null ? extras.get(key) : "NULL"));
            }
        }
    }
}
