//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.SharedPreferences;

import com.rallytac.engage.engine.Engine;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;

public class MissionDatabase
{
    private static String TAG = MissionDatabase.class.getSimpleName();

    public ArrayList<DatabaseMission> _missions = new ArrayList<>();

    public void clear()
    {
        _missions.clear();
    }

    public void save(SharedPreferences sp, String keyName)
    {
        JSONArray root = new JSONArray();

        for(DatabaseMission mission : _missions)
        {
            root.put(mission.toJson());
        }

        SharedPreferences.Editor ed = sp.edit();
        ed.putString(keyName, root.toString());
        ed.apply();
    }

    public static MissionDatabase load(SharedPreferences sp, String keyName)
    {
        MissionDatabase database;

        try
        {
            database = new MissionDatabase();

            String jsonData = sp.getString(keyName, "");
            JSONArray root = new JSONArray(jsonData);
            for(int x = 0; x < root.length(); x++)
            {
                String missionJson = root.getString(x);
                DatabaseMission mission = DatabaseMission.parse(missionJson);
                if(mission != null)
                {
                    database._missions.add(mission);
                }
            }
        }
        catch (Exception e)
        {
            database = null;
        }

        return database;
    }

    public DatabaseMission getMissionById(String id)
    {
        for(DatabaseMission mission : _missions)
        {
            if(mission._id.compareTo(id) == 0)
            {
                return mission;
            }
        }

        return null;
    }

    public boolean deleteMissionById(String id)
    {
        for(DatabaseMission mission : _missions)
        {
            if(mission._id.compareTo(id) == 0)
            {
                _missions.remove(mission);
                return true;
            }
        }

        return false;
    }

    @SuppressWarnings("BooleanMethodIsAlwaysInverted")
    public boolean updateMissionById(String id, DatabaseMission updatedMission)
    {
        int index = 0;

        for(DatabaseMission mission : _missions)
        {
            if(mission._id.compareTo(id) == 0)
            {
                _missions.set(index, updatedMission);
                return true;
            }

            index++;
        }

        return false;
    }


    public boolean addOrUpdateMissionFromActiveConfiguration(ActiveConfiguration ac)
    {
        boolean rc;

        try
        {
            DatabaseMission mission;

            mission = new DatabaseMission();
            mission._id = ac.getMissionId();
            mission._name = ac.getMissionName();
            mission._description = ac.getMissionDescription();
            mission._modPin = ac.getMissionModPin();

            mission._useRp = ac.getUseRp();
            mission._rpAddress = ac.getRpAddress();
            mission._rpPort = ac.getRpPort();

            // Presence
            for (GroupDescriptor gd : ac.getMissionGroups())
            {
                if (gd.type == GroupDescriptor.Type.gtPresence)
                {
                    JSONObject jo = new JSONObject(gd.jsonConfiguration);
                    mission._mcId = gd.id;
                    mission._mcAddress = jo.getJSONObject(Engine.JsonFields.Rx.objectName).optString(Engine.JsonFields.Rx.address, "");
                    mission._mcPort = jo.getJSONObject(Engine.JsonFields.Rx.objectName).optInt(Engine.JsonFields.Rx.port, 0);
                    mission._mcCryptoPassword = jo.optString(Engine.JsonFields.Group.cryptoPassword, "");
                    break;
                }
            }

            for (GroupDescriptor gd : ac.getMissionGroups())
            {
                if (gd.type == GroupDescriptor.Type.gtAudio)
                {
                    JSONObject jo = new JSONObject(gd.jsonConfiguration);
                    DatabaseGroup dbg = new DatabaseGroup();

                    dbg._id = gd.id;
                    dbg._name = gd.name;
                    dbg._cryptoPassword = jo.optString(Engine.JsonFields.Group.cryptoPassword);
                    dbg._useCrypto = !Utils.isEmptyString(dbg._cryptoPassword);

                    JSONObject opt;

                    opt = jo.optJSONObject(Engine.JsonFields.Rx.objectName);
                    {
                        if (opt == null)
                        {
                            opt = new JSONObject();
                        }

                        dbg._rxAddress = opt.optString(Engine.JsonFields.Rx.address, "");
                        dbg._rxPort = opt.optInt(Engine.JsonFields.Rx.port, 0);
                    }

                    opt = jo.optJSONObject(Engine.JsonFields.Tx.objectName);
                    {
                        if (opt == null)
                        {
                            opt = new JSONObject();
                        }

                        dbg._txAddress = opt.optString(Engine.JsonFields.Tx.address, "");
                        dbg._txPort = opt.optInt(Engine.JsonFields.Tx.port, 0);
                    }

                    opt = jo.optJSONObject(Engine.JsonFields.TxAudio.objectName);
                    {
                        if (opt == null)
                        {
                            opt = new JSONObject();
                        }

                        dbg._txCodecId = opt.optInt(Engine.JsonFields.TxAudio.encoder, Constants.DEFAULT_ENCODER);
                        dbg._txFramingMs = opt.optInt(Engine.JsonFields.TxAudio.framingMs, Constants.DEFAULT_TX_FRAMING_MS);
                        dbg._noHdrExt = opt.optBoolean(Engine.JsonFields.TxAudio.noHdrExt, false);
                        dbg._fdx = opt.optBoolean(Engine.JsonFields.TxAudio.fdx, false);
                        dbg._maxTxSecs = opt.optInt(Engine.JsonFields.TxAudio.maxTxSecs, Constants.DEFAULT_TX_SECS);
                    }

                    mission._groups.add(dbg);
                }
            }

            if(!updateMissionById(mission._id, mission))
            {
                _missions.add(mission);
            }

            rc = true;
        }
        catch (Exception e)
        {
            rc = false;
        }

        return rc;
    }
}
