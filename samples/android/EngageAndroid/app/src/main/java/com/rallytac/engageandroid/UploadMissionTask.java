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
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.URL;

public class UploadMissionTask extends AsyncTask<String, Void, String>
{
    public static String BUNDLE_RESULT_MSG = "BUNDLE_RESULT_MSG";//NON-NLS

    private int _responseCode = -1;
    private String _resultMsg = null;
    private Handler _handler = null;

    UploadMissionTask(Handler handler)
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
            String urlBase = params[0];
            String fn = params[1];
            String content = params[2];
            URL url = new URL(urlBase + "/" + fn);

            HttpURLConnection httpCon = (HttpURLConnection) url.openConnection();
            httpCon.setDoInput(true);
            httpCon.setDoOutput(true);
            httpCon.setRequestMethod("PUT");//NON-NLS
            OutputStreamWriter out = new OutputStreamWriter(httpCon.getOutputStream());
            out.write(content);
            out.close();

            _responseCode = httpCon.getResponseCode();
            _resultMsg = null;

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

                _resultMsg = sb.toString();
            }
            catch (Exception e)
            {
                e.printStackTrace();
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
            bundle.putString(BUNDLE_RESULT_MSG, result);

            Message msg = new Message();
            msg.arg1 = _responseCode;
            msg.setData(bundle);
            _handler.sendMessage(msg);
        }
    }
}
