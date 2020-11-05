//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid.Biometrics;

public class SeriesElement
{
    private int _timeOffset;
    private int _value;

    public SeriesElement(int timeOffset, int value)
    {
        setTimesoffset(timeOffset);
        setValue(value);
    }

    @Override
    public String toString()
    {
        StringBuilder sb = new StringBuilder();

        sb.append("_timeOffset=");//NON-NLS
            sb.append(_timeOffset);
        sb.append("_value=");//NON-NLS
            sb.append(_value);

        return sb.toString();
    }

    public int getTimeoffset()
    {
        return _timeOffset;
    }

    public void setTimesoffset(int v)
    {
        _timeOffset = v;
    }

    public void setValue(int v)
    {
        _value = v;
    }

    public int getValue()
    {
        return _value;
    }
}

