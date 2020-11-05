//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid.Biometrics;

import java.util.Random;

public class RandomHumanBiometricGenerator
{
    private Random _r;
    private int _min;
    private int _max;
    private int _lastVal;
    private int _variance;

    public RandomHumanBiometricGenerator(int min, int max, int variance, int startAt)
    {
        _r = new Random();
        _min = min;
        _max = max;
        _variance = variance;
        _lastVal = startAt;
    }

    public int nextInt()
    {
        int val;
        int upOrDown;

        upOrDown = (_r.nextInt() < 0 ? -1 : 1);
        val = _lastVal + ((_r.nextInt((Math.abs(_r.nextInt(_variance)) + 1)) * upOrDown));

        if(val < _min)
        {
            val = _min;
        }
        else if(val > _max)
        {
            val = _max;
        }

        _lastVal = val;

        return val;
    }
}
