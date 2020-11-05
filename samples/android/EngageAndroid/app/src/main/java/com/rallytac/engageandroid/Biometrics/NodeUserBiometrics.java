//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid.Biometrics;

import com.rallytac.engage.engine.Engine;
import com.rallytac.engageandroid.Charting.EntrySet;

public class NodeUserBiometrics
{
    private final int MAX_SAMPLES_TO_TRACK = 100;

    private EntrySet _entriesHeartRate = new EntrySet("Heart Rate", 40, 200, MAX_SAMPLES_TO_TRACK, 75, 25, 50, 75);//NON-NLS
    private EntrySet _entriesSkinTemp = new EntrySet("Skin Temperature", 30, 40, MAX_SAMPLES_TO_TRACK, 36, 3, 5, 6);//NON-NLS
    private EntrySet _entriesCoreTemp = new EntrySet("Core Temperature", 30, 40, MAX_SAMPLES_TO_TRACK, 36, 2, 3, 5);//NON-NLS
    private EntrySet _entriesHydration = new EntrySet("Hydration %", 50, 100, MAX_SAMPLES_TO_TRACK, 80, 10, 15, 20);//NON-NLS
    private EntrySet _entriesBloodOxygenation = new EntrySet("Blood Oxygenation %", 50, 100, MAX_SAMPLES_TO_TRACK, 90, 5, 8, 100);//NON-NLS
    private EntrySet _entriesFatigueLevel = new EntrySet("Fatigue Level", 0, 10, MAX_SAMPLES_TO_TRACK, 4, 2, 3, 5);//NON-NLS
    private EntrySet _entriesTaskEffectivenessLevel = new EntrySet("Task Effectiveness Level", 0, 10, MAX_SAMPLES_TO_TRACK, 3, 2, 4, 6);//NON-NLS

    public NodeUserBiometrics()
    {
    }

    public boolean merge(DataSeries ds)
    {
        Engine.HumanBiometricsElement elem = Engine.HumanBiometricsElement.fromInt(ds.getBinaryId());

        switch (elem)
        {
            case heartRate:
                return _entriesHeartRate.merge(ds);
            case skinTemp:
                return _entriesSkinTemp.merge(ds);
            case coreTemp:
                return _entriesCoreTemp.merge(ds);
            case hydration:
                return _entriesHydration.merge(ds);
            case bloodOxygenation:
                return _entriesBloodOxygenation.merge(ds);
            case fatigueLevel:
                return _entriesFatigueLevel.merge(ds);
            case taskEffectiveness:
                return _entriesTaskEffectivenessLevel.merge(ds);
        }

        return false;
    }

    public EntrySet getEsHeartrate()
    {
        return _entriesHeartRate;
    }

    public EntrySet getEsSkinTemp()
    {
        return _entriesSkinTemp;
    }

    public EntrySet getEsCoreTemp()
    {
        return _entriesCoreTemp;
    }

    public EntrySet getEsBloodOxygenation()
    {
        return _entriesBloodOxygenation;
    }

    public EntrySet getEsHydrationLevel()
    {
        return _entriesHydration;
    }

    public EntrySet getEsFatigueLevel()
    {
        return _entriesFatigueLevel;
    }

    public EntrySet getEsTaskEffectivenessLevel()
    {
        return _entriesTaskEffectivenessLevel;
    }
}
