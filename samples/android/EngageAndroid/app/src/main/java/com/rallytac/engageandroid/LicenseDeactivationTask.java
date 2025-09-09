//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Context;
import android.os.AsyncTask;

import org.json.JSONObject;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;

public class LicenseDeactivationTask extends AsyncTask<String, Void, String>
{
    private static final String TAG = LicenseDeactivationTask.class.getSimpleName();

    private static final int CONNECT_TIMEOUT = 10000;
    private static final int READ_TIMEOUT = 10000;

    public interface ITaskCompletionNotification
    {
        void onLicenseDeactivationTaskComplete(int result, String resultMessage);
    }

    private Context _ctx;
    private String _url;
    private String _key;
    private String _deviceId;
    private String _hValue;
    private ITaskCompletionNotification _completionNotification;

    private int _result;
    private String _resultMessage;

    public LicenseDeactivationTask(Context ctx,
                                   String url,
                                   String key,
                                   String deviceId,
                                   String hValue,
                                   ITaskCompletionNotification completionNotification)
    {
        _ctx = ctx;
        _url = url;
        _key = key;
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
            _completionNotification.onLicenseDeactivationTaskComplete(_result, _resultMessage);
        }
    }

    @Override
    protected String doInBackground(String... params)
    {
        _result = -1;
        _resultMessage = null;

        HttpURLConnection httpConnection = null;

        try
        {
            JSONObject obj = new JSONObject();

            obj.put("deviceSerialNumber", _deviceId);//NON-NLS
            obj.put("licenseId", _key);//NON-NLS
            obj.put("h", _hValue);//NON-NLS

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
            }
            else
            {
                throw new Exception("HTTP failure " + httpResultCode);
            }
        }
        catch (Exception ex)
        {
            _result = -1;
            _resultMessage = ex.getLocalizedMessage();
        }
        finally
        {
            if(httpConnection != null)
            {
                httpConnection.disconnect();
            }
        }

        return null;
    }
}
