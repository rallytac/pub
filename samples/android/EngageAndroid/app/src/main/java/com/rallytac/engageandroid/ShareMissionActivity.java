//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import androidx.core.content.FileProvider;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Switch;
import android.widget.Toast;

import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;

public class ShareMissionActivity extends AppCompatActivity
{
    private static String TAG = SettingsActivity.class.getSimpleName();

    private EngageApplication _app = null;

    private String _pwd;
    private String _deflectionUrl;
    private Bitmap _bm;
    private boolean _qrCodeZoomed = false;
    private String _base91DataString = null;
    private JSONObject _jsonConfiguration = null;
    private byte[] _compressedDataBytes = null;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_share_mission);

        _app = (EngageApplication)getApplication();
        ((EditText)findViewById(R.id.etDeflectionUrl)).setText(Globals.getSharedPreferences().getString(PreferenceKeys.LAST_QRCODE_DEFLECTION_URL, ""));

        LinearLayout lay = findViewById(R.id.layoutQrCode);
        lay.setVisibility(View.INVISIBLE);

        ActionBar ab = getSupportActionBar();
        if(ab != null)
        {
            ab.setDisplayHomeAsUpEnabled(true);
        }

        // Hide the upload switch if there's no upload address
        String urlBase = getString(R.string.mission_hub_address);
        if(Utils.isEmptyString(urlBase))
        {
            findViewById(R.id.swUpload).setVisibility(View.GONE);
        }

        setTitle(R.string.share_title);

        restoreSavedState(savedInstanceState);
        setElements();
        setBitmap();
        updateZoomView();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        if (item.getItemId() == android.R.id.home)
        {
            finish();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState)
    {
        Log.d(TAG, "onSaveInstanceState");//NON-NLS
        saveState(outState);
        super.onSaveInstanceState(outState);
    }

    private void saveState(Bundle bundle)
    {
        getElements();

        bundle.putString("PWD", _pwd);//NON-NLS
        bundle.putString("DEFLECTION_URL", _deflectionUrl);//NON-NLS
        bundle.putString("DATASTRING", _base91DataString);//NON-NLS
        bundle.putBoolean("ZOOMED", _qrCodeZoomed);//NON-NLS
        bundle.putByteArray("COMPRESSED_DATA_BYTES", _compressedDataBytes);//NON-NLS
    }

    private void restoreSavedState(Bundle bundle)
    {
        if(bundle == null)
        {
            return;
        }

        _pwd = bundle.getString("PWD");//NON-NLS
        _deflectionUrl = bundle.getString("DEFLECTION_URL", null);//NON-NLS
        _base91DataString = bundle.getString("DATASTRING", null);//NON-NLS
        _qrCodeZoomed = bundle.getBoolean("ZOOMED", false);//NON-NLS
        _compressedDataBytes = bundle.getByteArray("COMPRESSED_DATA_BYTES");//NON-NLS

        buildBitmap();
    }

    public void onClickQrCode(View view)
    {
        _qrCodeZoomed = !_qrCodeZoomed;
        updateZoomView();
    }

    private void updateZoomView()
    {
        LinearLayout lay = findViewById(R.id.layoutQrControls);
        if(_qrCodeZoomed)
        {
            lay.setVisibility(View.GONE);
        }
        else
        {
            lay.setVisibility(View.VISIBLE);
        }
    }

    private void getElements()
    {
        _pwd = ((EditText)findViewById(R.id.etPassword)).getText().toString();
        _deflectionUrl = ((EditText)findViewById(R.id.etDeflectionUrl)).getText().toString();
    }

    private void setElements()
    {
        ((EditText)findViewById(R.id.etPassword)).setText(_pwd);
        ((EditText)findViewById(R.id.etDeflectionUrl)).setText(_deflectionUrl);
    }

    private void setBitmap()
    {
        ImageView iv = findViewById(R.id.ivQrCode);
        LinearLayout lay = findViewById(R.id.layoutQrCode);

        if(_bm != null)
        {
            iv.setImageBitmap(_bm);
            lay.setVisibility(View.VISIBLE);
        }
        else
        {
            lay.setVisibility(View.INVISIBLE);
        }
    }

    private void buildBitmap()
    {
        //TODO: Binary QR code
        /*
        if(_compressedDataBytes != null)
        {
            byte[] hdr = Constants.QR_CODE_HEADER.getBytes(Utils.getEngageCharSet());
            byte[] ver = Constants.QR_VERSION.getBytes(Utils.getEngageCharSet());
            byte[] ba = new byte[hdr.length + ver.length + _compressedDataBytes.length];

            System.arraycopy(hdr, 0, ba, 0, hdr.length);
            System.arraycopy(ver, 0, ba, hdr.length, ver.length);
            System.arraycopy(_compressedDataBytes, 0, ba, hdr.length + ver.length, _compressedDataBytes.length);

            _bm = Utils.byteArrayToQrCodeBitmap(ba, Constants.QR_CODE_WIDTH, Constants.QR_CODE_HEIGHT);
        }
        else
        {
            _bm = null;
        }
        */

        if(!Utils.isEmptyString(_base91DataString))
        {
            _bm = Utils.stringToQrCodeBitmap(_base91DataString, Constants.QR_CODE_WIDTH, Constants.QR_CODE_HEIGHT);
        }
        else
        {
            _bm = null;
        }
    }

    public void onClickGenerateQrCode(View view)
    {
        Utils.hideKeyboardFrom(this, view);

        _jsonConfiguration = _app.getActiveConfiguration().makeTemplate();
        if(_jsonConfiguration == null)
        {
            Utils.showPopupMsg(ShareMissionActivity.this, getString(R.string.share_failed_to_create_configuration_package));
            finish();
            return;
        }

        String json = _jsonConfiguration.toString();

        if(json != null)
        {
            getElements();

            // This is our data record (header signature + version + json)
            String textRecord = (Constants.QR_CODE_HEADER + Constants.QR_VERSION) + json;

            // Compress it
            byte[] dataBytes = textRecord.getBytes(Utils.getEngageCharSet());
            _compressedDataBytes = Utils.compress(dataBytes, 0, dataBytes.length);

            // It gets encrypted if we got a password
            if(!Utils.isEmptyString(_pwd))
            {
                String pwdHexString = Utils.toHexString(_pwd.getBytes(Utils.getEngageCharSet()));
                _compressedDataBytes = Globals.getEngageApplication().getEngine().encryptSimple(_compressedDataBytes, pwdHexString);
                if(_compressedDataBytes == null)
                {
                    Utils.showPopupMsg(ShareMissionActivity.this,getString(R.string.share_failed_to_encrypt_configuration_package));
                    finish();
                    return;
                }
            }

            // Convert to a Base91-encoded string
            _base91DataString = new String(Base91.encode(_compressedDataBytes), Utils.getEngageCharSet());

            // Precede with a deflection url if any (and save whatever was there anyway - even if it's nothing)
            Globals.getSharedPreferencesEditor().putString(PreferenceKeys.LAST_QRCODE_DEFLECTION_URL, _deflectionUrl);
            Globals.getSharedPreferencesEditor().apply();

            if(!Utils.isEmptyString(_deflectionUrl))
            {
                // Note the Constants.QR_DEFLECTION_URL_SEP - its for the decoder to see if there's a deflection URL
                _base91DataString = _deflectionUrl + Constants.QR_DEFLECTION_URL_SEP + _base91DataString;
            }

            // Finally, we can create our QR code!
            buildBitmap();
            setBitmap();

            Globals.getEngageApplication().logEvent(Analytics.MISSION_QR_CODE_DISPLAYED_FOR_SHARE);
        }
        else
        {
            Globals.getEngageApplication().logEvent(Analytics.MISSION_QR_CODE_FAILED_CREATE);
            Utils.showPopupMsg(ShareMissionActivity.this,getString(R.string.share_failed_to_create_shareable_configuration_package));
            finish();
        }
    }

    public void onClickShare(View view)
    {
        ActiveConfiguration ac = _app.getActiveConfiguration();
        String downloadUrl = null;

        if(((Switch) findViewById(R.id.swUpload)).isChecked())
        {
            UploadMissionTask umt = new UploadMissionTask(new Handler()
            {
                @Override
                public void handleMessage(Message msg)
                {
                    int responseCode = msg.arg1;
                    String resultMsg = msg.getData().getString(UploadMissionTask.BUNDLE_RESULT_MSG);

                    if(responseCode >= 200 && responseCode <= 299)
                    {
                        Toast.makeText(ShareMissionActivity.this, R.string.share_mission_mission_uploaded, Toast.LENGTH_LONG).show();
                    }
                    else
                    {
                        Toast.makeText(ShareMissionActivity.this, getString(R.string.share_mission_mission_upload_failed) + resultMsg, Toast.LENGTH_LONG).show();
                    }
                }
            });

            Globals.getEngageApplication().logEvent(Analytics.MISSION_UPLOAD_REQUESTED);

            String urlBase = getString(R.string.mission_hub_address);
            String fn = ac.getMissionId() + ".json";//NON-NLS
            String content = _jsonConfiguration.toString();

            umt.execute(urlBase, fn, content);

            downloadUrl = urlBase + "/" + fn;
        }

        boolean sharingJson = (((Switch) findViewById(R.id.swShareJson)).isChecked());

        try
        {
            String extraText;
            ShareableData data = new ShareableData();

            if(sharingJson)
            {
                if(Utils.isEmptyString(downloadUrl))
                {
                    extraText = String.format(getString(R.string.fmt_load_this_json_file_to_join_the_mission), ac.getMissionName());
                }
                else
                {
                    data.setUrl(downloadUrl);

                    extraText = String.format(getString(R.string.fmt_load_this_json_file_to_join_the_mission_or_download_from), ac.getMissionName(), downloadUrl);
                }

                File fd = File.createTempFile("mission-" + ac.getMissionName().replace(" ", "-"), ".json", Environment.getExternalStorageDirectory());//NON-NLS

                FileOutputStream fos = new FileOutputStream(fd);

                if(!Utils.isEmptyString(_pwd))
                {
                    String pwdHexString = Utils.toHexString(_pwd.getBytes(Utils.getEngageCharSet()));
                    byte[] encryptedBytes = Globals.getEngageApplication().getEngine().encryptSimple(_jsonConfiguration.toString().getBytes(), pwdHexString);
                    fos.write(encryptedBytes);
                }
                else
                {
                    fos.write(_jsonConfiguration.toString().getBytes());
                }

                fos.close();

                Uri u = FileProvider.getUriForFile(this, getString(R.string.file_content_provider), fd);

                fd.deleteOnExit();
                data.addUri(u);

                Globals.getEngageApplication().logEvent(Analytics.MISSION_SHARE_JSON);
            }
            else
            {
                if(Utils.isEmptyString(downloadUrl))
                {
                    extraText = String.format(getString(R.string.fmt_scan_this_qr_code_to_join_the_mission), ac.getMissionName());
                }
                else
                {
                    data.setUrl(downloadUrl);

                    extraText = String.format(getString(R.string.fmt_scan_this_qr_code_to_join_the_mission_or_download_from), ac.getMissionName(), downloadUrl);
                }

                File fd = File.createTempFile("qr-" + ac.getMissionName().replace(" ", "-"), ".jpg", Environment.getExternalStorageDirectory());//NON-NLS
                ByteArrayOutputStream bos = new ByteArrayOutputStream();
                _bm.compress(Bitmap.CompressFormat.JPEG, 100, bos);
                byte[] bitmapdata = bos.toByteArray();

                FileOutputStream fos = new FileOutputStream(fd);
                fos.write(bitmapdata);
                fos.close();

                Uri u = FileProvider.getUriForFile(this, getString(R.string.file_content_provider), fd);

                fd.deleteOnExit();
                data.addUri(u);

                Globals.getEngageApplication().logEvent(Analytics.MISSION_SHARE_QR);
            }

            data.setText(String.format(getString(R.string.share_mission_email_subject), getString(R.string.app_name), ac.getMissionName()));

            data.setHtml(extraText);

            data.setSubject(getString(R.string.app_name) + " : " + ac.getMissionName());
            startActivity(ShareHelper.buildShareIntent(this, data, getString(R.string.share_mission_upload_header)));
        }
        catch (Exception e)
        {
            Globals.getEngageApplication().logEvent(Analytics.MISSION_SHARE_EXCEPTION);
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_SHORT).show();
        }
    }
}
