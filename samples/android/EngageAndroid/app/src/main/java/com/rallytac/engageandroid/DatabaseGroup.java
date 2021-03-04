//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import org.json.JSONObject;

public class DatabaseGroup
{
    public String _id;
    public int _type;
    public String _name;
    public boolean _useCrypto;
    public String _cryptoPassword;
    public String _rxAddress;
    public int _rxPort;
    public String _txAddress;
    public int _txPort;
    public int _txCodecId;
    public int _txFramingMs;
    public boolean _noHdrExt;
    public boolean _fdx;
    public int _maxTxSecs;
    public int _ept;
    public boolean _anonymousAlias;

    @Override
    public String toString()
    {
        JSONObject obj = toJson();
        return (obj != null ? obj.toString() : "null");
    }

    public JSONObject toJson()
    {
        JSONObject root;

        try
        {
            root = new JSONObject();
            root.put("_id", Utils.trimString(_id));//NON-NLS
            root.put("_type", _type);//NON-NLS
            root.put("_name", Utils.trimString(_name));//NON-NLS
            root.put("_useCrypto", _useCrypto);//NON-NLS
            root.put("_cryptoPassword", Utils.trimString(_cryptoPassword));//NON-NLS
            root.put("_rxAddress", Utils.trimString(_rxAddress));//NON-NLS
            root.put("_rxPort", _rxPort);//NON-NLS
            root.put("_txAddress", Utils.trimString(_txAddress));//NON-NLS
            root.put("_txPort", _txPort);//NON-NLS
            root.put("_txCodecId", _txCodecId);//NON-NLS
            root.put("_txFramingMs", _txFramingMs);//NON-NLS
            root.put("_noHdrExt", _noHdrExt);//NON-NLS
            root.put("_fdx", _fdx);//NON-NLS
            root.put("_ept", _ept);//NON-NLS
            root.put("_anonymousAlias", _anonymousAlias);//NON-NLS
            root.put("_maxTxSecs", _maxTxSecs);//NON-NLS
        }
        catch (Exception e)
        {
            root = null;
        }

        return root;
    }

    public static DatabaseGroup parse(String json)
    {
        DatabaseGroup group = new DatabaseGroup();

        try
        {
            JSONObject root = new JSONObject(json);
            group._id = root.getString("_id").trim();//NON-NLS
            group._type = root.getInt("_type");//NON-NLS
            group._name = Utils.trimString(root.optString("_name"));//NON-NLS
            group._useCrypto = root.optBoolean("_useCrypto");//NON-NLS
            group._cryptoPassword = Utils.trimString(root.optString("_cryptoPassword"));//NON-NLS
            group._rxAddress = Utils.trimString(root.optString("_rxAddress"));//NON-NLS
            group._rxPort = root.optInt("_rxPort");//NON-NLS
            group._txAddress = Utils.trimString(root.optString("_txAddress"));//NON-NLS
            group._txPort = root.optInt("_txPort");//NON-NLS
            group._txCodecId = root.optInt("_txCodecId");//NON-NLS
            group._txFramingMs = root.optInt("_txFramingMs");//NON-NLS
            group._noHdrExt = root.optBoolean("_noHdrExt");//NON-NLS
            group._fdx = root.optBoolean("_fdx");//NON-NLS
            group._ept = root.optInt("_ept");//NON-NLS
            group._anonymousAlias = root.optBoolean("_anonymousAlias");//NON-NLS
            group._maxTxSecs = root.optInt("_maxTxSecs");//NON-NLS
        }
        catch (Exception e)
        {
            group = null;
        }

        return group;
    }
}
