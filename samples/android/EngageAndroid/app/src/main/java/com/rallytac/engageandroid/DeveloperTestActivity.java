//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

/*


    ********************************************************************************************

    This module is provided for developer use only.  It is essentially a "petri dish" where you
    can experiment with Engine API calls and other functionality within the main program without
    affecting the rest of the software.

    In order to have the menu option appear to display this activity, open the "About" activity
    and tap on the application logo 7 times.  This will toggle developer mode.

    ********************************************************************************************

*/

package com.rallytac.engageandroid;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;

import com.rallytac.engage.engine.Engine;
import com.rallytac.engageandroid.Biometrics.DataSeries;

import org.json.JSONObject;

import java.util.Random;

public class DeveloperTestActivity extends AppCompatActivity
{
    private final String TAG = DeveloperTestActivity.class.getSimpleName();

    EngageApplication _app;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_developer_test);
        _app = Globals.getEngageApplication();

        installDeveloperTestingMission();
    }

    public void onClickSendBlob(View view)
    {
        testSendBlob();
    }
    public void onClickSendHumanBiometrics(View view)
    {
        testSendHumanBiometrics();
    }
    public void onClickSendRaw(View view)
    {
        testSendRaw();
    }
    public void onClickSendCustomRtp(View view)
    {
        testSendCustomRtp();
    }
    public void onClickRegisterRtp(View view)
    {
        testRegisterRtp();
    }
    public void onClickUnregisterRtp(View view)
    {
        testUnregisterRtp();
    }

    private GroupDescriptor getFirstPresenceGroup()
    {
        ActiveConfiguration ac = _app.getActiveConfiguration();
        for(GroupDescriptor gd : ac.getMissionGroups())
        {
            if(gd.type == GroupDescriptor.Type.gtPresence)
            {
                return gd;
            }
        }

        return null;
    }

    private GroupDescriptor getFirstAudioGroup()
    {
        ActiveConfiguration ac = _app.getActiveConfiguration();
        for(GroupDescriptor gd : ac.getMissionGroups())
        {
            if(gd.type == GroupDescriptor.Type.gtAudio)
            {
                return gd;
            }
        }

        return null;
    }

    private GroupDescriptor getFirstRawGroup()
    {
        ActiveConfiguration ac = _app.getActiveConfiguration();
        for(GroupDescriptor gd : ac.getMissionGroups())
        {
            if(gd.type == GroupDescriptor.Type.gtRaw)
            {
                return gd;
            }
        }

        return null;
    }



    private void installDeveloperTestingMission()
    {
        ActiveConfiguration.installMissionJson(this, Utils.getStringResource(this, R.raw.development_testing_mission_template), false);
    }


    // --------- Raw-related
    /*
        - Raw data can only be sent on Raw groups (type 3)

        - The "jsonParams" parameter is ignored at this time and should be set to null or an empry string
    */

    private void testSendRaw()
    {
        ActiveConfiguration ac = _app.getActiveConfiguration();
        String blobString = "Hello world raw";//NON-NLS
        byte[] blob = blobString.getBytes();
        String jsonParams = "";
        GroupDescriptor gd = getFirstRawGroup();
        if(gd == null)
        {
            Toast.makeText(this, "Cannot find a raw group in the current mission", Toast.LENGTH_SHORT).show();//NON-NLS
            return;
        }

        _app.getEngine().engageSendGroupRaw(gd.id, blob, blob.length, jsonParams);
    }

    // --------- Blob-related
    /*
        - Blobs can only be sent on RTP groups (type 1) or Presence groups (type 2)

        - The "jsonParams" parameter is optional but, if provided, may contain the following
        {
            "target" : "{engageTargetNodeId}",
            "payloadType" : <one of the JsonFields.BlobInfo.PT_xxxxx values>,
            "rtpHeader":
            {
                "pt": numeric_rtp_payload_type,
                "marker:" boolean_to_indicate_whether_to_set_rtp_marker_bit,
                "seq": numeric_rtp_sequence_number,
                "ssrc": numeric_rtp_ssrc_number,
                "ts": numeric_timestamp_in_rtp_units
            }
        }

        - If a JSON parameter is not provided, the Engine will assign a value of 0, false, or empty
          string as appropriate
        - IMPORTANT: "rtpHeader" is ignored if the blob is transmitted on a non-audio (i.e. RTP) group.
    */

    private void testSendBlob()
    {
        try
        {
            ActiveConfiguration ac = _app.getActiveConfiguration();
            String blobString = "Hello world blob";//NON-NLS
            byte[] blob = blobString.getBytes();

            JSONObject bi = new JSONObject();
            bi.put(Engine.JsonFields.BlobInfo.payloadType, Engine.BlobType.appTextUtf8.toInt());
            String jsonParams = bi.toString();

            GroupDescriptor gd = getFirstPresenceGroup();
            if(gd == null)
            {
                Toast.makeText(this, "Cannot find a presence group in the current mission", Toast.LENGTH_SHORT).show();//NON-NLS
                return;
            }

            _app.getEngine().engageSendGroupBlob(gd.id, blob, blob.length, jsonParams);
        }
        catch (Exception e)
        {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_SHORT).show();
        }
    }

    DataSeries _dsHeartrate = null;
    Random _rnd = new Random();
    long _beats = 0;

    private void testSendHumanBiometrics()
    {
        try
        {
            final int NUM_SAMPLES = 64;
            final int INTERVAL_INCREMENT = 1;


            // We'll "go back in time" when we generate our data
            int startingTimestamp = (int)((System.currentTimeMillis() / 1000L) - (NUM_SAMPLES * INTERVAL_INCREMENT));

            ActiveConfiguration ac = _app.getActiveConfiguration();

            DataSeries dsHeartrate = new DataSeries();

            dsHeartrate.setTimestamp(startingTimestamp);
            dsHeartrate.setBinaryId(Engine.HumanBiometricsElement.heartRate.toInt());

            // Generate a random heartrate - anywhere from 45bpm t0 165 bpm.  This
            // will look pretty bad on the receiving side - as if our user is
            // having a heart attack, but we won't pay too much attention to the
            // interpretation of these values - just that we have valid values.
            Random rnd = new Random();
            for(int x = 0; x < NUM_SAMPLES; x++)
            {
                int nextInt = rnd.nextInt();
                if(nextInt < 0)
                {
                    nextInt *= -1;
                }
                int hr = (45 + (nextInt % 120));
                dsHeartrate.addElement((byte)(INTERVAL_INCREMENT * x), (byte)hr);
            }

            // Pack the data series in a byte array
            byte[] blobHeartRate = dsHeartrate.toByteArray();

            // Our JSON parameters indicate that the payload is binary human biometric data in Engage format
            JSONObject bi = new JSONObject();
            bi.put(Engine.JsonFields.BlobInfo.payloadType, Engine.BlobType.engageHumanBiometrics.toInt());
            String jsonParams = bi.toString();

            // Send the blob to the presence group
            GroupDescriptor gd = getFirstPresenceGroup();
            if(gd == null)
            {
                Toast.makeText(this, "Cannot find a presence group in the current mission", Toast.LENGTH_SHORT).show();//NON-NLS
                return;
            }

            _app.getEngine().engageSendGroupBlob(gd.id, blobHeartRate, blobHeartRate.length, jsonParams);
        }
        catch (Exception e)
        {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_SHORT).show();
        }
    }


    // --------- Custom RTP-related
    /*
        - Custom RTP payloads can only be sent on RTP groups (type 1)

        - The "jsonParams" parameter is required and is of type "rtpHeader" as in:
        {
            "pt": numeric_rtp_payload_type,
            "marker:" boolean_to_indicate_whether_to_set_rtp_marker_bit,
            "seq": numeric_rtp_sequence_number,
            "ssrc": numeric_rtp_ssrc_number,
            "ts": numeric_timestamp_in_rtp_units
        }

        - Only the "pt" element is required.  All others will be defaulted to 0/false by the Engine
    */
    private void testRegisterRtp()
    {
        ActiveConfiguration ac = _app.getActiveConfiguration();
        GroupDescriptor gd = getFirstAudioGroup();
        if(gd == null)
        {
            Toast.makeText(this, "Cannot find an audio group in the current mission", Toast.LENGTH_SHORT).show();//NON-NLS
            return;
        }

        _app.getEngine().engageRegisterGroupRtpHandler(gd.id, 109);
    }

    private void testUnregisterRtp()
    {
        ActiveConfiguration ac = _app.getActiveConfiguration();
        GroupDescriptor gd = getFirstAudioGroup();
        if(gd == null)
        {
            Toast.makeText(this, "Cannot find an audio group in the current mission", Toast.LENGTH_SHORT).show();//NON-NLS
            return;
        }

        _app.getEngine().engageUnregisterGroupRtpHandler(gd.id, 109);
    }

    private void testSendCustomRtp()
    {
        GroupDescriptor gd = getFirstAudioGroup();

        if(gd == null)
        {
            Toast.makeText(this, "Cannot find an audio group in the current mission", Toast.LENGTH_SHORT).show();//NON-NLS
            return;
        }

        try
        {
            JSONObject rtpHeader = new JSONObject();

            rtpHeader.put(Engine.JsonFields.RtpHeader.pt, 109);        // Required
            rtpHeader.put(Engine.JsonFields.RtpHeader.marker, false);  // Optional
            rtpHeader.put(Engine.JsonFields.RtpHeader.seq, 1);         // Optional
            rtpHeader.put(Engine.JsonFields.RtpHeader.ssrc, 2);        // Optional
            rtpHeader.put(Engine.JsonFields.RtpHeader.ts, 3);          // Optional

            String jsonParams = rtpHeader.toString();

            String rtpPayloadString = "Hello world RTP";//NON-NLS
            byte[] rtpPayload = rtpPayloadString.getBytes();


            _app.getEngine().engageSendGroupRtp(gd.id, rtpPayload, rtpPayload.length, jsonParams);
        }
        catch (Exception e)
        {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_SHORT).show();
        }
    }
}
