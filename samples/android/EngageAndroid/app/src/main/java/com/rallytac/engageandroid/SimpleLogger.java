//
//  Copyright (c) 2020 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.util.Log;

import com.rallytac.engage.engine.Engine;

public class SimpleLogger implements ILogger
{
    private boolean _logToEngage = false;

    SimpleLogger()
    {
    }

    public void setLogToEngage(boolean doLogging)
    {
        _logToEngage = doLogging;
    }

    public boolean getLogToEngage()
    {
        return _logToEngage;
    }

    @Override
    public void d(String tag, String msg)
    {
        Log.d(tag, msg);
        logToEngage(Engine.LoggingLevel.debug, tag, msg);
    }

    @Override
    public void i(String tag, String msg)
    {
        Log.i(tag, msg);
        logToEngage(Engine.LoggingLevel.information, tag, msg);
    }

    @Override
    public void w(String tag, String msg)
    {
        Log.w(tag, msg);
        logToEngage(Engine.LoggingLevel.warning, tag, msg);
    }

    @Override
    public void e(String tag, String msg)
    {
        Log.e(tag, msg);
        logToEngage(Engine.LoggingLevel.error, tag, msg);
    }

    @Override
    public void f(String tag, String msg)
    {
        Log.wtf(tag, msg);
        logToEngage(Engine.LoggingLevel.fatal, tag, msg);
    }

    private void logToEngage(Engine.LoggingLevel level, String tag, String msg)
    {
        if(_logToEngage)
        {
            EngageApplication app = Globals.getEngageApplication();
            if(app != null)
            {
                Engine engine = app.getEngine();
                if( engine != null )
                {
                    engine.engageLogMsg(level.toInt(), tag, msg);
                }
            }
        }
    }
}
