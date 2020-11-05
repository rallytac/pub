//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import com.rallytac.engage.engine.Engine;

import org.json.JSONObject;
import java.util.Date;

public class LicenseDescriptor
{
    public Engine.LicenseType _theType;
    public String _deviceId;
    public String _key;
    public String _activationCode;
    public Date _expires;
    public String _expiresFormatted = null;
    public Engine.LicensingStatusCode _status;

    LicenseDescriptor()
    {
        _theType = Engine.LicenseType.unknown;
        _deviceId = null;
        _key = null;
        _activationCode = null;
        _expires = null;
        _expiresFormatted = null;
        _status = Engine.LicensingStatusCode.generalFailure;
    }

    LicenseDescriptor(LicenseDescriptor ld)
    {
        this._theType = ld._theType;
        this._deviceId = ld._deviceId;
        this._key = ld._key;
        this._activationCode = ld._activationCode;
        this._expires = ld._expires;
        this._expiresFormatted = ld._expiresFormatted;
        this._status = ld._status;
    }

    public boolean isValid()
    {
        return (_theType != Engine.LicenseType.unknown );
    }

    public boolean equals(LicenseDescriptor ld)
    {
        return Utils.stringsMatch(_deviceId, ld._deviceId) &&
                Utils.stringsMatch(_key, ld._key) &&
                Utils.stringsMatch(_activationCode, ld._activationCode);
    }

    static public LicenseDescriptor fromJson(String jsonData)
    {
        LicenseDescriptor rc = new LicenseDescriptor();

        rc._theType = Engine.LicenseType.unknown;

        try
        {
            JSONObject obj = new JSONObject(jsonData);

            long unixSeconds = obj.optInt(Engine.JsonFields.License.expires, 0);

            rc._theType = Engine.LicenseType.fromInt(obj.getInt(Engine.JsonFields.License.type));
            rc._status = Engine.LicensingStatusCode.fromInt(obj.getInt(Engine.JsonFields.License.status));
            rc._deviceId = obj.getString(Engine.JsonFields.License.deviceId);
            rc._key = obj.optString(Engine.JsonFields.License.key, "");
            rc._activationCode = obj.optString(Engine.JsonFields.License.activationCode, "");

            if(unixSeconds > 0)
            {
                rc._expires = Utils.javaDateFromUnixSeconds(unixSeconds);
                rc._expiresFormatted = Utils.formatDateUtc(rc._expires);
            }
            else
            {
                rc._expires = null;
                rc._expiresFormatted = null;
            }
        }
        catch (Exception e)
        {
            rc = new LicenseDescriptor();
            rc._theType = Engine.LicenseType.unknown;
        }

        return rc;
    }
}
