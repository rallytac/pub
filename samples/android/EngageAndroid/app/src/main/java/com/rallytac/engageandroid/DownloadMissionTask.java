//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class DownloadMissionTask extends AsyncTask<String, Void, String>
{
    public static String BUNDLE_RESULT_MSG = "BUNDLE_RESULT_MSG";//NON-NLS
    public static String BUNDLE_RESULT_DATA = "BUNDLE_RESULT_DATA";//NON-NLS

    private int _responseCode = -1;
    private String _resultMsg = null;
    private String _resultData = null;
    private Handler _handler = null;

    DownloadMissionTask(Handler handler)
    {
        _handler = handler;
    }

    protected void onPreExecute()
    {
    }

    protected String doInBackground(String... params)
    {
        try
        {
            URL url = new URL(params[0]);

            HttpURLConnection httpCon = (HttpURLConnection) url.openConnection();
            httpCon.setDoInput(true);
            httpCon.setRequestMethod("GET");//NON-NLS
            httpCon.connect();

            _responseCode = httpCon.getResponseCode();
            _resultData = null;

            try
            {
                BufferedReader in = new BufferedReader(new InputStreamReader(httpCon.getInputStream()));
                StringBuffer sb = new StringBuffer();
                String line;

                while ((line = in.readLine()) != null)
                {
                    sb.append(line);
                }

                in.close();

                _resultData = sb.toString();
            }
            catch (Exception e)
            {
                _responseCode = -1;
                _resultMsg = "Exception: " + e.getMessage();//NON-NLS
            }
        }
        catch(Exception e)
        {
            _responseCode = -1;
            _resultMsg = "Exception: " + e.getMessage();//NON-NLS
        }

        return _resultMsg;
    }

    @Override
    protected void onPostExecute(final String result)
    {
        if(_handler != null)
        {
            Bundle bundle = new Bundle();
            bundle.putString(BUNDLE_RESULT_MSG, _resultMsg);
            bundle.putString(BUNDLE_RESULT_DATA, _resultData);

            Message msg = new Message();
            msg.arg1 = _responseCode;
            msg.setData(bundle);
            _handler.sendMessage(msg);
        }
    }
}
