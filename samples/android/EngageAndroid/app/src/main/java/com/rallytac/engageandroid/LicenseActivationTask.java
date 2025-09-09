//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.os.AsyncTask;
import android.os.Build;

import com.rallytac.engage.engine.Engine;

import org.json.JSONObject;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;

public class LicenseActivationTask extends AsyncTask<String, Void, String>
{
    private static final String TAG = LicenseActivationTask.class.getSimpleName();

    private static final int CONNECT_TIMEOUT = 10000;
    private static final int READ_TIMEOUT = 10000;

    public interface ITaskCompletionNotification
    {
        void onLicenseActivationTaskComplete(int result, String activationCode, String resultMessage);
    }

    private String _url;
    private String _entitlement;
    private String _key;
    private String _activationCode;
    private String _deviceId;
    private String _hValue;
    private ITaskCompletionNotification _completionNotification;

    private int _result;
    private String _resultActivationCode;
    private String _resultMessage;

    public LicenseActivationTask(String url,
                                 String entitlement,
                                 String key,
                                 String activationCode,
                                 String deviceId,
                                 String hValue,
                                 ITaskCompletionNotification completionNotification)
    {
        _url = url;
        _entitlement = entitlement;
        _key = key;
        _activationCode = activationCode;
        _deviceId = deviceId;
        _hValue = hValue;
        _completionNotification = completionNotification;
    }

    @Override
    protected void onPreExecute()
    {
        super.onPreExecute();
    }

    @Override
    protected void onPostExecute(String s)
    {
        super.onPostExecute(s);

        if(_completionNotification != null)
        {
            _completionNotification.onLicenseActivationTaskComplete(_result, _resultActivationCode, _resultMessage);
        }
    }

    @Override
    protected String doInBackground(String... params)
    {
        _result = -1;
        _resultActivationCode = null;
        _resultMessage = null;

        // Test the data to make sure we're at least sending over good information
        try
        {
            LicenseDescriptor testDescriptor = LicenseDescriptor.fromJson(Globals.getEngageApplication()
                    .getEngine()
                    .engageGetLicenseDescriptor(Globals.getEngageApplication().getString(R.string.licensing_entitlement),
                            _key,
                            _activationCode,
                            Globals.getEngageApplication().getString(R.string.manufacturer_id)));

            if(testDescriptor == null)
            {
                throw new Exception("cannot parse into testDescriptor");
            }

            if(testDescriptor._status != Engine.LicensingStatusCode.ok && testDescriptor._status != Engine.LicensingStatusCode.requiresActivation)
            {
                throw new Exception("license type does not require activation");
            }
        }
        catch (Exception e)
        {
            Globals.getLogger().e(TAG, "exception: _result=" + _result + ", _resultMessage=" + _resultMessage + ", _resultActivationCode=" + _resultActivationCode);//NON-NLS
            return null;
        }


        HttpURLConnection httpConnection = null;
        JSONObject deviceInfo;

        try
        {
            deviceInfo = new JSONObject();

            deviceInfo.put("manufacturer", Build.MANUFACTURER);//NON-NLS
            deviceInfo.put("device", Build.DEVICE);//NON-NLS
            deviceInfo.put("type", Build.TYPE);//NON-NLS
            deviceInfo.put("board", Build.BOARD);//NON-NLS
            deviceInfo.put("model", Build.MODEL);//NON-NLS
            deviceInfo.put("cpuAbi", Build.CPU_ABI);//NON-NLS
            deviceInfo.put("display", Build.DISPLAY);//NON-NLS
            deviceInfo.put("hardware", Build.HARDWARE);//NON-NLS
            deviceInfo.put("host", Build.HOST);//NON-NLS
            deviceInfo.put("id", Build.ID);//NON-NLS
            deviceInfo.put("user", Build.USER);//NON-NLS
            deviceInfo.put("product", Build.PRODUCT);//NON-NLS
            deviceInfo.put("tags", Build.TAGS);//NON-NLS
        }
        catch (Exception e)
        {
            deviceInfo = null;
        }

        try
        {
            JSONObject obj = new JSONObject();

            obj.put("deviceSerialNumber", _deviceId);//NON-NLS

            obj.put("entitlementKey", _entitlement);//NON-NLS
            obj.put("licenseId", _key);//NON-NLS
            obj.put("h", _hValue);//NON-NLS

            if(!Utils.isEmptyString(_activationCode))
            {
                obj.put("activationCode", _activationCode);//NON-NLS
            }

            if(deviceInfo != null)
            {
                obj.put("deviceInfo", deviceInfo);//NON-NLS
            }

            obj.put("appVersion", BuildConfig.VERSION_NAME);//NON-NLS
            obj.put("appPackage", Globals.getContext().getPackageName());//NON-NLS

            String dataToPost = obj.toString();

            URL url = new URL(_url);

            httpConnection = (HttpURLConnection) url.openConnection();

            byte[] bytes = dataToPost.getBytes();
            int len = bytes.length;

            httpConnection.setRequestMethod("POST");//NON-NLS
            httpConnection.setRequestProperty("Content-length", Integer.toString(len));//NON-NLS
            httpConnection.setUseCaches(false);
            httpConnection.setAllowUserInteraction(false);
            httpConnection.setConnectTimeout(CONNECT_TIMEOUT);
            httpConnection.setReadTimeout(READ_TIMEOUT);
            httpConnection.setDoOutput(true);
            httpConnection.setChunkedStreamingMode(0);
            httpConnection.connect();

            OutputStream out = new BufferedOutputStream(httpConnection.getOutputStream());
            out.write(bytes);
            out.flush();
            out.close();

            int httpResultCode = httpConnection.getResponseCode();
            String httpResultMessage = httpConnection.getResponseMessage();

            if (httpResultCode == HttpURLConnection.HTTP_OK)
            {
                BufferedReader br = new BufferedReader(new InputStreamReader(httpConnection.getInputStream()));
                StringBuilder sb = new StringBuilder();
                String line;

                while ((line = br.readLine()) != null)
                {
                    sb.append(line);
                }
                br.close();

                JSONObject rc = new JSONObject(sb.toString());
                Globals.getLogger().d(TAG, rc.toString());

                _result = rc.getInt("returnCode");//NON-NLS
                _resultMessage = rc.optString("returnCodeDescr", null);//NON-NLS
                _resultActivationCode = rc.optString("activationCode", null);//NON-NLS

                Globals.getLogger().d(TAG, "_result=" + _result + ", _resultMessage=" + _resultMessage + ", _resultActivationCode=" + _resultActivationCode);//NON-NLS
            }
            else
            {
                Globals.getLogger().e(TAG, "exception: HTTP failure " + httpResultCode);//NON-NLS
                throw new Exception("HTTP failure " + httpResultCode);
            }
        }
        catch (Exception ex)
        {
            _result = -1;
            _activationCode = null;
            _resultMessage = ex.getLocalizedMessage();
            Globals.getLogger().e(TAG, "exception: _result=" + _result + ", _resultMessage=" + _resultMessage + ", _resultActivationCode=" + _resultActivationCode);//NON-NLS
        }
        finally
        {
            if(httpConnection != null)
            {
                httpConnection.disconnect();
            }
        }

        Globals.getLogger().d(TAG, "_result=" + _result + ", _resultMessage=" + _resultMessage + ", _resultActivationCode=" + _resultActivationCode);//NON-NLS

        return null;
    }
}
