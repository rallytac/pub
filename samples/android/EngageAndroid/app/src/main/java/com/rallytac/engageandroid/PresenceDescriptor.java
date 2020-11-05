//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.location.Location;
import android.util.Log;

import com.rallytac.engage.engine.Engine;
import com.rallytac.engageandroid.Biometrics.DataSeries;
import com.rallytac.engageandroid.Biometrics.NodeUserBiometrics;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.Calendar;
import java.util.HashMap;

public class PresenceDescriptor
{
    private static String TAG = PresenceDescriptor.class.getSimpleName();

    public class Connectivity
    {
        public int type;
        public int strength;
        public int rating;
    }

    public class Power
    {
        public int source;
        public int state;
        public int level;
    }

    public boolean self;
    public String nodeId;
    public String type;
    public String format;
    public String userId;
    public String displayName;
    public String comment;
    public String custom;
    public Location location;
    public Calendar lastUpdate;
    public HashMap<String, String> groupAliases;

    public NodeUserBiometrics userBiometrics = null;
    public Connectivity connectivity = null;
    public Power power = null;

    public void clear()
    {
        self = false;
        nodeId = null;
        type = null;
        userId = null;
        displayName = null;
        comment = null;
        custom = null;
        location = null;
        lastUpdate = null;
        groupAliases = null;

        // NOTE !! userBiometrics is not cleared!!
    }

    public boolean deserialize(String json)
    {
        clear();

        try
        {
            JSONObject root = new JSONObject(json);

            // Is this us?
            self = root.optBoolean(Engine.JsonFields.PresenceDescriptor.self, false);

            // Miscellaneous
            comment = root.optString(Engine.JsonFields.PresenceDescriptor.comment);
            custom = root.optString(Engine.JsonFields.PresenceDescriptor.custom);

            // Identity object is required
            {
                JSONObject identity = Utils.getJsonObject(root, Engine.JsonFields.Identity.objectName);
                if (identity != null)
                {
                    // NodeId is required, all else is optional
                    nodeId = identity.getString(Engine.JsonFields.Identity.nodeId);

                    userId = identity.optString(Engine.JsonFields.Identity.userId);
                    displayName = identity.optString(Engine.JsonFields.Identity.displayName);
                    type = identity.optString(Engine.JsonFields.Identity.type);
                    format = identity.optString(Engine.JsonFields.Identity.format);
                }
            }

            // Location is optional
            {
                JSONObject loc = Utils.getJsonObject(root, Engine.JsonFields.Location.objectName);
                if (loc != null)
                {
                    location = new Location("");
                    location.setLongitude(loc.getDouble(Engine.JsonFields.Location.longitude));
                    location.setLatitude(loc.getDouble(Engine.JsonFields.Location.latitude));

                    double d;

                    d = loc.optDouble(Engine.JsonFields.Location.altitude);
                    if (!Double.isNaN(d))
                    {
                        location.setAltitude(d);
                    }

                    d = loc.optDouble(Engine.JsonFields.Location.speed);
                    if (!Double.isNaN(d))
                    {
                        location.setSpeed((float) d);
                    }

                    d = loc.optDouble(Engine.JsonFields.Location.direction);
                    if (!Double.isNaN(d))
                    {
                        location.setBearing((float) d);
                    }

                    // Let's make sure we're good here - if not, whack it!
                    if (!Utils.isLocationValid(location))
                    {
                        Log.w(TAG, "received location object failed validation");//NON-NLS
                        location = null;
                    }
                }
            }

            // Power is optional
            {
                JSONObject pwr = Utils.getJsonObject(root, Engine.JsonFields.Power.objectName);
                if (pwr != null)
                {
                    power = new Power();
                    power.source = pwr.optInt(Engine.JsonFields.Power.source, -1);
                    power.state = pwr.optInt(Engine.JsonFields.Power.state, 0);
                    power.level = pwr.optInt(Engine.JsonFields.Power.level, 0);
                }
            }

            // Connectivity is optional
            {
                JSONObject cty = Utils.getJsonObject(root, Engine.JsonFields.Connectivity.objectName);
                if (cty != null)
                {
                    connectivity = new Connectivity();
                    connectivity.type = cty.optInt(Engine.JsonFields.Connectivity.type, -1);
                    connectivity.strength = cty.optInt(Engine.JsonFields.Connectivity.strength, 0);
                    connectivity.rating = cty.optInt(Engine.JsonFields.Connectivity.rating, 0);
                }
            }

            // Group aliases
            {
                JSONArray ga = root.getJSONArray(Engine.JsonFields.PresenceDescriptor.GroupAlias.arrayName);
                if (ga != null && ga.length() > 0)
                {
                    groupAliases = new HashMap<>();
                    for (int x = 0; x < ga.length(); x++)
                    {
                        JSONObject g = ga.getJSONObject(x);
                        if (g != null)
                        {
                            groupAliases.put(g.getString(Engine.JsonFields.PresenceDescriptor.GroupAlias.groupId),
                                    g.getString(Engine.JsonFields.PresenceDescriptor.GroupAlias.alias));
                        }
                    }
                }
            }

            return true;
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    public boolean updateFromPresenceDescriptor(PresenceDescriptor pd)
    {
        if(nodeId.compareTo(pd.nodeId) != 0)
        {
            return false;
        }

        self = pd.self;
        if(!Utils.isEmptyString(pd.type)) type = pd.type;
        if(!Utils.isEmptyString(pd.format)) format = pd.format;
        if(!Utils.isEmptyString(pd.userId)) userId = pd.userId;
        if(!Utils.isEmptyString(pd.displayName)) displayName = pd.displayName;
        if(!Utils.isEmptyString(pd.comment)) comment = pd.comment;
        if(!Utils.isEmptyString(pd.custom)) custom = pd.custom;
        if(pd.location != null) location = pd.location;
        if(pd.groupAliases != null) groupAliases = pd.groupAliases;
        if(pd.power != null) power = pd.power;
        if(pd.connectivity != null) connectivity = pd.connectivity;

        return true;
    }

    public boolean updateBioMetrics(DataSeries ds)
    {
        boolean rc = false;

        try
        {
            if(userBiometrics == null)
            {
                userBiometrics = new NodeUserBiometrics();
            }

            rc = userBiometrics.merge(ds);
        }
        catch (Exception e)
        {
            rc = false;
        }

        return rc;
    }
}
