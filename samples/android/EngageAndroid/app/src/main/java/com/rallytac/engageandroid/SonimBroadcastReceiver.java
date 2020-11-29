//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;

public class SonimBroadcastReceiver extends BroadcastReceiver
{
    private static String TAG = SonimBroadcastReceiver.class.getSimpleName();

    private String ACTION_PTT_DOWN_SONIM = "com.sonim.intent.action.PTT_KEY_DOWN";//NON-NLS
    private String ACTION_PTT_UP_SONIM = "com.sonim.intent.action.PTT_KEY_UP";//NON-NLS

    private Context _ctx;
    private IPushToTalkRequestHandler _pttRequestHandler;

    public SonimBroadcastReceiver(Context ctx, IPushToTalkRequestHandler handler)
    {
        _ctx = ctx;
        _pttRequestHandler = handler;
    }

    public void start()
    {
        IntentFilter intentFilter;

        intentFilter = new IntentFilter(ACTION_PTT_DOWN_SONIM);
        intentFilter.setPriority(999);
        _ctx.registerReceiver(this, intentFilter);

        intentFilter = new IntentFilter(ACTION_PTT_UP_SONIM);
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
        String action = intent.getAction();
        if(Utils.isEmptyString(action))
        {
            return;
        }

        Log.d(TAG, "onReceive: (" + action + "): " + intent.toString());//NON-NLS

        if(action.equals(ACTION_PTT_DOWN_SONIM))
        {
            // TODO: set ptt priority based on other Sonim buttons
            _pttRequestHandler.requestPttOn(-1, -1);
        }
        else if(action.equals(ACTION_PTT_UP_SONIM))
        {
            _pttRequestHandler.requestPttOff();
        }
        else
        {
            Log.e(TAG, "Unhandled notification: (" + action + "): " + intent.toString());//NON-NLS
        }
    }
}
