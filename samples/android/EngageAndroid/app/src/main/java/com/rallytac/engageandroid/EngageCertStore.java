//
//  Copyright (c) 2020 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import com.rallytac.engage.engine.Engine;

import org.json.JSONArray;
import org.json.JSONObject;

public class EngageCertStore
{
    private String _id;
    private String _fileName;
    private JSONObject _descriptor;
    private String _passwordHexString;
    private boolean _isAppDefault = false;

    private String _cachedDisplayName = null;
    private String _cachedDescription = null;

    public String getDisplayName()
    {
        if(Utils.isEmptyString(_cachedDisplayName))
        {
            if(_isAppDefault)
            {
                _cachedDisplayName = Globals.getEngageApplication().getString(R.string.application_internal_certificate_store_display_name);
            }
            else
            {
                _cachedDisplayName = _fileName;

                if (!Utils.isEmptyString(_cachedDisplayName))
                {
                    int pos = _cachedDisplayName.indexOf("}-");
                    _cachedDisplayName = _cachedDisplayName.substring(pos + 2);
                }
                else
                {
                    _cachedDisplayName = Globals.getEngageApplication().getString(R.string.no_certstore_name);
                }
            }
        }

        return _cachedDisplayName;
    }

    public boolean idMatches(String s)
    {
        if(Utils.isEmptyString(s) && Utils.isEmptyString(_id))
        {
            return true;
        }
        else
        {
            if(!Utils.isEmptyString(s) && !Utils.isEmptyString(_id))
            {
                return (s.compareToIgnoreCase(_id) == 0);
            }
            else
            {
                return false;
            }
        }
    }

    public boolean isAppDefault()
    {
        return _isAppDefault;
    }

    public String getId()
    {
        return _id;
    }

    public String getFileName()
    {
        return _fileName;
    }

    public String getPasswordhexString()
    {
        return _passwordHexString;
    }

    public String getDescription()
    {
        if(Utils.isEmptyString(_cachedDescription))
        {
            StringBuilder sb = new StringBuilder();

            String id = _id;
            if(!Utils.isEmptyString(id))
            {
                sb.append(id);
                sb.append(", ");//NON-NLS
            }

                /*
                sb.append("V");//NON-NLS
                sb.append(_descriptor.optInt(Engine.JsonFields.CertStoreDescriptor.version, 0));
                sb.append(", ");//NON-NLS
                */

            JSONArray certificates = _descriptor.optJSONArray(Engine.JsonFields.CertStoreDescriptor.certificates);
            sb.append(certificates.length());
            sb.append(" certificates");//NON-NLS

                /*
                sb.append("\nhash [");//NON-NLS
                StringBuilder hashInput = new StringBuilder();
                hashInput.append(_descriptor.optInt(Engine.JsonFields.CertStoreDescriptor.version, 0));
                hashInput.append(certificates.toString());
                sb.append(Utils.md5HashOfString(hashInput.toString()));
                sb.append("]");//NON-NLS
                */

            _cachedDescription = sb.toString();
        }

        return _cachedDescription;
    }

    public static EngageCertStore loadStoreFrom(String fn)
    {
        EngageCertStore rc = new EngageCertStore();

        try
        {
            rc._isAppDefault = fn.contains(Constants.INTERNAL_DEFAULT_CERTSTORE_FN);
            rc._fileName = fn;
            rc._descriptor = Globals.getEngageApplication().getCertificateStoreDescriptorForFile(fn);
            if(rc._descriptor == null)
            {
                throw new Exception("Cannot get certificate store descriptor");//NON-NLS
            }

            rc._id = rc._descriptor.optString(Engine.JsonFields.CertStoreDescriptor.id, "");
            rc._passwordHexString = rc._descriptor.getString(Constants.CERTSTORE_JSON_INTERNAL_PASSWORD_HEX_STRING);
        }
        catch (Exception e)
        {
            rc = null;
            e.printStackTrace();
        }

        return rc;
    }
}
