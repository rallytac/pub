//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.journeyapps.barcodescanner.Util;
import com.rallytac.engage.engine.Engine;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;

public class GroupDescriptor implements Parcelable
{
    private static String TAG = GroupDescriptor.class.getSimpleName();

    public enum Type {gtUnknown, gtAudio, gtPresence, gtRaw}

    // Config
    public Type type;
    public String id;
    public String name;
    public boolean isEncrypted;
    public boolean fdx;
    public String jsonConfiguration;
    public int ept;
    public boolean anonymousAlias;

    private boolean _isDynamic;

    private HashMap<String, GroupMembershipTracker> memberNodes = new HashMap<>();

    // State
    public boolean selectedForSingleView;
    public boolean selectedForMultiView;
    public boolean created;
    public boolean createError;
    public boolean joined;
    public boolean joinError;
    public boolean rx;
    public boolean tx;
    public boolean txPending;
    public boolean txError;
    public boolean txUsurped;
    public boolean rxMuted;
    public boolean txSelected;
    public ArrayList<TalkerDescriptor> talkerList = new ArrayList<>();
    public long lastTxStartTime;

    public static final Creator CREATOR = new Creator()
    {
        public GroupDescriptor createFromParcel(Parcel in)
        {
            return new GroupDescriptor(in);
        }

        public GroupDescriptor[] newArray(int size)
        {
            return new GroupDescriptor[size];
        }
    };

    public GroupDescriptor()
    {
        resetState();

        created = false;
        createError = false;
        joined = false;
        joinError = false;
        _isDynamic = false;
        lastTxStartTime = 0;

        this.type = Type.gtUnknown;
    }

    GroupDescriptor(Parcel in)
    {
        resetState();

        created = false;
        createError = false;
        joined = false;
        joinError = false;
        _isDynamic = false;
        lastTxStartTime = 0;

        this.jsonConfiguration = in.readString();
        this.type = Type.values()[in.readInt()];
        this.id = in.readString();
        this.name = in.readString();
        this.isEncrypted = (in.readInt() == 1);
        this.fdx = (in.readInt() == 1);
        this.ept = in.readInt();
        this.anonymousAlias = (in.readInt() == 1);

        this.selectedForSingleView = (in.readInt() == 1);
        this.selectedForMultiView = (in.readInt() == 1);
        this.created = (in.readInt() == 1);
        this.createError = (in.readInt() == 1);
        this.joined = (in.readInt() == 1);
        this.joinError = (in.readInt() == 1);
        this.rx = (in.readInt() == 1);
        this.tx = (in.readInt() == 1);
        this.txPending = (in.readInt() == 1);
        this.txError = (in.readInt() == 1);
        this.txUsurped = (in.readInt() == 1);
        this.rxMuted = (in.readInt() == 1);
        this.txSelected = (in.readInt() == 1);

        this._isDynamic = (in.readInt() == 1);
        this.lastTxStartTime = in.readLong();
    }

    public boolean isDynamic()
    {
        return _isDynamic;
    }

    public void setDynamic(boolean b)
    {
        _isDynamic = b;
    }

    public boolean isConnectedInSomeForm()
    {
        return Globals.getEngageApplication().isGroupConnectedInSomeWay(id);
    }

    public void resetState()
    {
        rx = false;
        tx = false;
        txPending = false;
        txError = false;
        txUsurped = false;
        rxMuted = false;
        txSelected = false;
        talkerList.clear();
        memberNodes.clear();
        lastTxStartTime = 0;
    }

    @Override
    public int describeContents()
    {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
        dest.writeString(this.jsonConfiguration);
        dest.writeInt(type.ordinal());
        dest.writeString(this.id);
        dest.writeString(this.name);
        dest.writeInt(this.isEncrypted ? 1 : 0);
        dest.writeInt(this.fdx ? 1 : 0);
        dest.writeInt(this.ept);
        dest.writeInt(this.anonymousAlias ? 1 : 0);

        dest.writeInt(this.selectedForSingleView ? 1 : 0);
        dest.writeInt(this.selectedForMultiView ? 1 : 0);
        dest.writeInt(this.created ? 1 : 0);
        dest.writeInt(this.createError ? 1 : 0);
        dest.writeInt(this.joined ? 1 : 0);
        dest.writeInt(this.joinError ? 1 : 0);
        dest.writeInt(this.rx ? 1 : 0);
        dest.writeInt(this.tx ? 1 : 0);
        dest.writeInt(this.txPending ? 1 : 0);
        dest.writeInt(this.txError ? 1 : 0);
        dest.writeInt(this.txUsurped ? 1 : 0);
        dest.writeInt(this.rxMuted ? 1 : 0);
        dest.writeInt(this.txSelected ? 1 : 0);

        dest.writeInt(this._isDynamic ? 1 : 0);
        dest.writeLong(this.lastTxStartTime);
    }

    public void updateTalkers(ArrayList<TalkerDescriptor> list)
    {
        synchronized (this)
        {
            talkerList.clear();
            if (list != null)
            {
                for (TalkerDescriptor td : list)
                {
                    talkerList.add(td);
                }
            }
        }
    }

    public ArrayList<TalkerDescriptor> getTalkers()
    {
        ArrayList<TalkerDescriptor> rc = new ArrayList<>();

        synchronized (this)
        {
            for(TalkerDescriptor td : talkerList)
            {
                rc.add(td);

                /*
                // TODO: Make this nicer
                // Mark talker in emergency TX with an asterisk
                if((td.rxFlags & Constants.ENGAGE_RXFLAG_EMERGENCY) == Constants.ENGAGE_RXFLAG_EMERGENCY)
                {
                    sb.append("*");
                }
                */
            }
        }

        return rc;
    }

    public boolean loadFromJson(String json)
    {
        boolean rc;

        try
        {
            JSONObject obj = new JSONObject(json);

            id = obj.getString(Engine.JsonFields.Group.id);
            type = Type.values()[obj.getInt(Engine.JsonFields.Group.type)];
            name = obj.getString(Engine.JsonFields.Group.name);
            isEncrypted = !Utils.isEmptyString(obj.optString(Engine.JsonFields.Group.cryptoPassword, null));
            fdx = obj.optBoolean(Engine.JsonFields.TxAudio.fdx);
            ept = obj.optInt(Constants.EPT_ELEMENT_NAME, 0);
            anonymousAlias = obj.optBoolean(Constants.ANONYMOUS_ALIAS_ELEMENT_NAME, false);

            jsonConfiguration = json;

            rc = true;
        }
        catch (Exception e)
        {
            rc = false;
        }

        return rc;
    }

    public void updateStateFrom(GroupDescriptor gd)
    {
        synchronized (this)
        {
            this.selectedForSingleView = gd.selectedForSingleView;
            this.selectedForMultiView = gd.selectedForMultiView;
            this.created = gd.created;
            this.createError = gd.createError;
            this.joined = gd.joined;
            this.joinError = gd.joinError;
            this.rx = gd.rx;
            this.tx = gd.tx;
            this.txPending = gd.txPending;
            this.txError = gd.txError;
            this.txUsurped = gd.txUsurped;
            this.rxMuted = gd.rxMuted;
            this.txSelected = gd.txSelected;

            if (gd.talkerList != null && gd.talkerList.size() > 0)
            {
                for (TalkerDescriptor td : gd.talkerList)
                {
                    this.talkerList.add(td);
                }
            }
            else
            {
                this.talkerList.clear();
            }

            if (gd.memberNodes != null && gd.memberNodes.size() > 0)
            {
                for (GroupMembershipTracker gmt : gd.memberNodes.values())
                {
                    this.memberNodes.put(gmt._nodeId, gmt);
                }
            }
            else
            {
                this.memberNodes.clear();
            }

            this.lastTxStartTime = gd.lastTxStartTime;
        }
    }

    public int getMemberCountForStatus(int status)
    {
        int rc = 0;

        if(memberNodes != null && memberNodes.size() > 0)
        {
            for(GroupMembershipTracker gmt: memberNodes.values())
            {
                //Log.i(TAG, "#DBG#: getMemberCountForStatus: " + gmt._nodeId + ", status=" + gmt._statusFlags);
                if((gmt._statusFlags & status) == status)
                {
                    rc++;
                }
            }
        }

        return rc;
    }

    public HashMap<String, GroupMembershipTracker> getMemberNodes()
    {
        return memberNodes;
    }

    public boolean addOrUpdateMember(GroupMembershipTracker gmt)
    {
        boolean rc = false;

        GroupMembershipTracker existing = memberNodes.get(gmt._nodeId);
        if(existing == null)
        {
            memberNodes.put(gmt._nodeId, gmt);
            rc = true;
        }
        else
        {
            if(existing._statusFlags != gmt._statusFlags)
            {
                existing._statusFlags = gmt._statusFlags;
                rc = true;
            }
        }

        return rc;
    }

    public boolean removeMember(String nodeId)
    {
        if(memberNodes.containsKey(nodeId))
        {
            memberNodes.remove(nodeId);
            return true;
        }
        else
        {
            return false;
        }
    }

    public boolean couldWorkWithoutRallypoint()
    {
        boolean rc = false;

        try
        {
            JSONObject j = new JSONObject(jsonConfiguration);
            String rxAddr = null;
            int rxPort = 0;
            String txAddr = null;
            int txPort = 0;

            if(j.has("rx"))
            {
                rxAddr = j.getJSONObject("rx").getString(Engine.JsonFields.Address.address);
                rxPort = j.getJSONObject("rx").getInt(Engine.JsonFields.Address.port);
            }

            if(j.has("tx"))
            {
                txAddr = j.getJSONObject("tx").getString(Engine.JsonFields.Address.address);
                txPort = j.getJSONObject("tx").getInt(Engine.JsonFields.Address.port);
            }

            if(!Utils.isEmptyString(rxAddr) && rxPort > 0 &&
               !Utils.isEmptyString(txAddr) && txPort > 0)
            {
                rc = true;
            }
        }
        catch (Exception e)
        {
            rc = false;
        }

        return rc;
    }
}
