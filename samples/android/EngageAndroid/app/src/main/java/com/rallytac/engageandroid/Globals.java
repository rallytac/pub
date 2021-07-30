//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Context;
import android.content.SharedPreferences;

public class Globals
{
    private static Context _ctx = null;
    private static EngageApplication _app = null;
    private static SharedPreferences _sp = null;
    private static SharedPreferences.Editor _spEd = null;
    private static AudioPlayerManager _apm = null;
    private static ILogger _logger = new SimpleLogger();

    public static void setContext(Context ctx)
    {
        _ctx = ctx;
    }

    public static Context getContext()
    {
        return _ctx;
    }

    public static void setEngageApplication(EngageApplication app)
    {
        _app = app;
    }

    public static ILogger getLogger()
    {
        return _logger;
    }

    public static EngageApplication getEngageApplication()
    {
        return _app;
    }

    public static void setSharedPreferences(SharedPreferences sp)
    {
        _sp = sp;
        _spEd = sp.edit();
    }

    public static SharedPreferences getSharedPreferences()
    {
        return _sp;
    }

    public static SharedPreferences.Editor getSharedPreferencesEditor()
    {
        return _spEd;
    }

    public static void setAudioPlayerManager(AudioPlayerManager apm)
    {
        _apm = apm;
    }

    public static AudioPlayerManager getAudioPlayerManager()
    {
        return _apm;
    }
}
