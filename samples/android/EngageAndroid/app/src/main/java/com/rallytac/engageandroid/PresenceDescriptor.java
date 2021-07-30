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
    public HashMap<String, GroupMembershipTracker> groupMembership;

    public NodeUserBiometrics userBiometrics = null;
    public Connectivity connectivity = null;
    public Power power = null;

    public String getFriendlyName()
    {
        if(!Utils.isEmptyString(displayName))
        {
            return displayName;
        }
        else if(!Utils.isEmptyString(userId))
        {
            return userId;
        }
        else
        {
            return nodeId;
        }
    }


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
        groupMembership = null;

        // NOTE !! userBiometrics is not cleared!!
    }

    public void clearMemberships()
    {
        groupMembership.clear();
        groupMembership = null;
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

            if(Utils.isEmptyString(nodeId))
            {
                throw new Exception("No nodeId for PD");
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
                        Globals.getLogger().w(TAG, "received location object failed validation");//NON-NLS
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

            // Group membership
            {
                JSONArray ga = root.optJSONArray(Engine.JsonFields.PresenceDescriptor.GroupItem.arrayName);
                if (ga != null && ga.length() > 0)
                {
                    groupMembership = new HashMap<>();
                    for (int x = 0; x < ga.length(); x++)
                    {
                        JSONObject g = ga.getJSONObject(x);
                        if (g != null)
                        {
                            String gid = g.optString(Engine.JsonFields.PresenceDescriptor.GroupItem.id, null);
                            if(!Utils.isEmptyString(gid))
                            {
                                groupMembership.put(gid,
                                        new GroupMembershipTracker(nodeId,
                                                                   gid,
                                                                   g.optInt(Engine.JsonFields.PresenceDescriptor.GroupItem.status, 0)));
                            }
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
        type = pd.type;
        format = pd.format;
        userId = pd.userId;
        displayName = pd.displayName;
        comment = pd.comment;
        custom = pd.custom;
        location = pd.location;
        groupMembership = new HashMap<>();

        if(pd.groupMembership != null && !pd.groupMembership.isEmpty())
        {
            for(GroupMembershipTracker gmt: pd.groupMembership.values())
            {
                groupMembership.put(gmt._groupId, gmt);
            }
        }
        else
        {
            groupMembership = null;
        }

        power = pd.power;
        connectivity = pd.connectivity;

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
