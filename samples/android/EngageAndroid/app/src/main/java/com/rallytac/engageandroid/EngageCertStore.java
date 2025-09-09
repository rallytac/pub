//
//  Copyright (c) 2020 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import com.rallytac.engage.engine.Engine;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.ByteArrayInputStream;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.text.SimpleDateFormat;
import java.util.Locale;

public class EngageCertStore
{
    private String _id;
    private String _fileName;
    private JSONObject _descriptor;
    private String _passwordHexString;
    private boolean _isAppDefault = false;
    private boolean _isRtsFactory = false;

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
            else if(_isRtsFactory)
            {
                _cachedDisplayName = Globals.getEngageApplication().getString(R.string.rts_factory_certificate_store_display_name);
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
            if(Utils.isEmptyString(id))
            {
                id = Globals.getContext().getString(R.string.descriptor_certificate_id_placeholder);
            }

            JSONArray certificates = _descriptor.optJSONArray(Engine.JsonFields.CertStoreDescriptor.certificates);
            sb.append(String.format(Globals.getContext().getString(R.string.descriptor_certificates_fmt), id, certificates.length()));

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
            rc._isRtsFactory = fn.contains(Constants.RTS_FACTORY_CERTSTORE_FN);
            rc._fileName = fn;
            rc._descriptor = Globals.getEngageApplication().getCertificateStoreDescriptorForFile(fn);
            if(rc._descriptor == null)
            {
                throw new Exception("Cannot get certificate store descriptor");//NON-NLS
            }

            rc._id = rc._descriptor.optString(Engine.JsonFields.CertStoreDescriptor.id, "");
            rc._passwordHexString = rc._descriptor.getString(Constants.CERTSTORE_JSON_INTERNAL_PASSWORD_HEX_STRING);
            //rc._cachedDisplayName = rc._descriptor.optString("name", "");

            JSONArray kvp = rc._descriptor.optJSONArray("kvp");
            if(kvp != null)
            {
                for (int x = 0; x < kvp.length(); x++)
                {
                    JSONObject pair = kvp.getJSONObject(x);
                    if(pair != null)
                    {
                        String key = pair.optString("key", "");
                        String value = pair.optString("value", "");

                        if(!Utils.isEmptyString(key) && !Utils.isEmptyString(value))
                        {
                            if(key.equalsIgnoreCase("_DISPLAY"))
                            {
                                rc._cachedDisplayName = value;
                                break;
                            }
                        }
                    }
                }
            }
        }
        catch (Exception e)
        {
            rc = null;
            e.printStackTrace();
        }

        return rc;
    }

    public String getPopupDescription()
    {
        StringBuilder sb = new StringBuilder();

        try
        {
            JSONArray certificates = _descriptor.getJSONArray(Engine.JsonFields.CertStoreCertificateElement.arrayName);
            for(int x = 0; x < certificates.length(); x++)
            {
                JSONObject certificate = certificates.getJSONObject(x);

                if (sb.length() > 0)
                {
                    sb.append("\n");
                }

                sb.append(certificate.getString(Engine.JsonFields.CertStoreCertificateElement.id));
                if (certificate.getBoolean(Engine.JsonFields.CertStoreCertificateElement.hasPrivateKey))
                {
                    sb.append(" ");
                    sb.append(Globals.getContext().getString(R.string.cs_popup_plus_private_key));
                }

                sb.append("\n");
                sb.append("[");
                sb.append(certificate.getString(Engine.JsonFields.CertStoreCertificateElement.tags));
                sb.append("]");
                sb.append("\n");

                String pem = certificate.optString("certificatePem");
                if(!Utils.isEmptyString(pem))
                {
                    try
                    {
                        CertificateFactory factory = CertificateFactory.getInstance("X.509");
                        ByteArrayInputStream inputStream = new ByteArrayInputStream(pem.getBytes());
                        X509Certificate x509 = (X509Certificate) factory.generateCertificate(inputStream);

                        String subject = x509.getSubjectDN().getName();
                        String issuer = x509.getIssuerDN().getName();
                        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.US);
                        String validFrom = dateFormat.format(x509.getNotBefore());
                        String validUntil = dateFormat.format(x509.getNotAfter());

                        String details = "Subject: " + subject + "\n\n" +
                                "Issuer: " + issuer + "\n\n" +
                                "Valid From: " + validFrom + "\n\n" +
                                "Valid Until: " + validUntil + "\n\n" +
                                "Serial Number: " + x509.getSerialNumber().toString() + "\n\n" +
                                "Signature Algorithm: " + x509.getSigAlgName() + "\n";

                        sb.append(details);
                        sb.append("\n");
                    } catch (Exception e) {
                    }
                }
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

        return sb.toString();
    }

    public String getPopupDescriptionAsHtml()
    {
        StringBuilder sb = new StringBuilder();

        try
        {
            JSONArray certificates = _descriptor.getJSONArray(Engine.JsonFields.CertStoreCertificateElement.arrayName);
            for(int x = 0; x < certificates.length(); x++)
            {
                JSONObject certificate = certificates.getJSONObject(x);

                if (sb.length() > 0)
                {
                    sb.append("<br>");
                }

                sb.append("<font color='blue' size='1'><b>");
                sb.append(certificate.getString(Engine.JsonFields.CertStoreCertificateElement.id));
                sb.append("</font></b>");
                if (certificate.getBoolean(Engine.JsonFields.CertStoreCertificateElement.hasPrivateKey))
                {
                    sb.append(" ");
                    sb.append(Globals.getContext().getString(R.string.cs_popup_plus_private_key));
                }

                sb.append("<br>");
                sb.append("<b>Tags: </b>");
                sb.append(certificate.getString(Engine.JsonFields.CertStoreCertificateElement.tags));
                sb.append("<br>");

                String pem = certificate.optString("certificatePem");
                if(!Utils.isEmptyString(pem))
                {
                    try
                    {
                        CertificateFactory factory = CertificateFactory.getInstance("X.509");
                        ByteArrayInputStream inputStream = new ByteArrayInputStream(pem.getBytes());
                        X509Certificate x509 = (X509Certificate) factory.generateCertificate(inputStream);

                        String subject = x509.getSubjectDN().getName();
                        String issuer = x509.getIssuerDN().getName();
                        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.US);
                        String validFrom = dateFormat.format(x509.getNotBefore());
                        String validUntil = dateFormat.format(x509.getNotAfter());

                        String details = "<b>Subject: </b>" + subject + "<br>" +
                                "<b>Issuer: </b>" + issuer + "<br>" +
                                "<b>Valid From: </b>" + validFrom + "<br>" +
                                "<b>Valid Until: </b>" + validUntil + "<br>" +
                                "<b>Serial Number: </b>" + x509.getSerialNumber().toString() + "<br>" +
                                "<b>Signature Algorithm: </b>" + x509.getSigAlgName() + "<br>";

                        sb.append(details);
                        sb.append("<br>");
                    }
                    catch (Exception e)
                    {
                    }
                }
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

        return sb.toString();
    }
}
