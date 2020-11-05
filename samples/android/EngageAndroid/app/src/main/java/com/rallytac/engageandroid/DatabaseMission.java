//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;

public class DatabaseMission
{
    public String _id;
    public String _modPin;
    public String _name;
    public String _description;

    public boolean _useRp;
    public String _rpAddress;
    public int _rpPort;
    public int _multicastFailoverPolicy;

    public String _mcId;
    public String _mcAddress;
    public int _mcPort;
    public String _mcCryptoPassword;


    public ArrayList<DatabaseGroup> _groups = new ArrayList<>();

    @Override
    public String toString()
    {
        JSONObject obj = toJson();
        return (obj != null ? obj.toString() : "null");
    }

    public DatabaseGroup getGroupById(String id)
    {
        for(DatabaseGroup group : _groups)
        {
            if(group._id.compareTo(id) == 0)
            {
                return group;
            }
        }

        return null;
    }

    public boolean deleteGroupById(String id)
    {
        for(DatabaseGroup group : _groups)
        {
            if(group._id.compareTo(id) == 0)
            {
                _groups.remove(group);
                return true;
            }
        }

        return false;
    }

    public boolean updateGroupById(String id, DatabaseGroup updatedGroup)
    {
        int index = 0;

        for(DatabaseGroup group : _groups)
        {
            if(group._id.compareTo(id) == 0)
            {
                _groups.set(index, updatedGroup);
                return true;
            }

            index++;
        }

        return false;
    }


    public JSONObject toJson()
    {
        JSONObject root;

        try
        {
            root = new JSONObject();
            root.put("_id", Utils.trimString(_id));//NON-NLS
            root.put("_modPin", Utils.trimString(_modPin));//NON-NLS
            root.put("_name", Utils.trimString(_name));//NON-NLS
            root.put("_description", Utils.trimString(_description));//NON-NLS
            root.put("_useRp", _useRp);//NON-NLS
            root.put("_rpAddress", Utils.trimString(_rpAddress));//NON-NLS
            root.put("_rpPort", _rpPort);//NON-NLS

            root.put("multicastFailoverPolicy", _multicastFailoverPolicy);//NON-NLS

            root.put("_mcId", Utils.trimString(_mcId));//NON-NLS
            root.put("_mcAddress", Utils.trimString(_mcAddress));//NON-NLS
            root.put("_mcPort", _mcPort);//NON-NLS
            root.put("_mcCryptoPassword", Utils.trimString(_mcCryptoPassword));//NON-NLS

            JSONArray groups = new JSONArray();
            for(DatabaseGroup group : _groups)
            {
                groups.put(group.toJson());
            }

            root.put("groups", groups);//NON-NLS
        }
        catch (Exception e)
        {
            root = null;
        }

        return root;
    }

    public static DatabaseMission parse(String json)
    {
        DatabaseMission mission = new DatabaseMission();

        try
        {
            JSONObject root = new JSONObject(json);
            mission._id = Utils.trimString(root.getString("_id"));//NON-NLS
            mission._modPin = Utils.trimString(root.optString("_modPin"));//NON-NLS
            mission._name = Utils.trimString(root.optString("_name"));//NON-NLS
            mission._description = Utils.trimString(root.optString("_description"));//NON-NLS
            mission._useRp = root.optBoolean("_useRp");//NON-NLS
            mission._rpAddress = Utils.trimString(root.optString("_rpAddress"));//NON-NLS
            mission._rpPort = root.optInt("_rpPort");//NON-NLS
            mission._multicastFailoverPolicy = root.optInt("multicastFailoverPolicy");//NON-NLS
            mission._mcId = Utils.trimString(root.getString("_mcId"));//NON-NLS
            mission._mcAddress = Utils.trimString(root.getString("_mcAddress"));//NON-NLS
            mission._mcPort = root.getInt("_mcPort");//NON-NLS
            mission._mcCryptoPassword = Utils.trimString(root.getString("_mcCryptoPassword"));//NON-NLS

            JSONArray groups = root.optJSONArray("groups");//NON-NLS
            if(groups != null)
            {
                for(int x = 0; x < groups.length(); x++)
                {
                    DatabaseGroup group = DatabaseGroup.parse(groups.get(x).toString());
                    if(group != null)
                    {
                        mission._groups.add(group);
                    }
                }
            }
        }
        catch (Exception e)
        {
            mission = null;
        }

        return mission;
    }
}
