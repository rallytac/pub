//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.text.Editable;
import android.text.InputFilter;
import android.text.SpannableString;
import android.text.TextWatcher;
import android.text.method.LinkMovementMethod;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.PopupMenu;

import com.google.zxing.integration.android.IntentIntegrator;
import com.google.zxing.integration.android.IntentResult;
import com.rallytac.engage.engine.Engine;

import org.json.JSONObject;

import java.util.Date;

public class AboutActivity extends
                                AppCompatActivity
                           implements
                                LicenseActivationTask.ITaskCompletionNotification,
                                LicenseDeactivationTask.ITaskCompletionNotification

{
    private static String TAG = AboutActivity.class.getSimpleName();

    private enum ScanType {stUnknown, stLicenseKey, stActivationCode, stEnterpriseId}

    private ImageView _ivAppLogo;
    private TextView _tvAppName;
    private TextView _tvLicenseHeader;
    private TextView _tvLicensingMessage;
    private EditText _etDeviceId;
    private EditText _etLicenseKey;
    private EditText _etActivationCode;
    private EditText _etEnterpriseId;
    private boolean _creating;
    private ScanType _scanType;
    private ProgressDialog _progressDialog = null;
    private boolean _scanning = false;
    private InternalDescriptor _activeLd = null;
    private InternalDescriptor _newLd = null;
    private ImageView _ivScanLicenseKey = null;
    private ImageView _ivLoadLicenseKey = null;
    private ImageView _ivScanActivationCode = null;
    private ImageView _ivWebFetchActivationCode = null;

    private int _numberOfClicksOfAppLogo = 0;
    private int _numberOfClicksOfAppName = 0;

    private class InternalDescriptor
    {
        public LicenseDescriptor _ld;
        public boolean _needsSaving = false;

        InternalDescriptor()
        {
            _needsSaving = false;
        }

        InternalDescriptor(InternalDescriptor x)
        {
            this._ld = x._ld;
            this._needsSaving = x._needsSaving;
        }

        public boolean isValid()
        {
            return _ld.isValid();
        }

        public boolean equals(InternalDescriptor x)
        {
            return this._ld.equals(x._ld);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        Globals.getEngageApplication().pauseLicenseActivation();

        _creating = true;
        setContentView(R.layout.activity_about);

        ActionBar ab = getSupportActionBar();
        if(ab != null)
        {
            ab.setDisplayHomeAsUpEnabled(true);
        }

        // This is for developer-related testing.  How it works is that if you
        // click the app logo 7 times, the app toggles developer mode
        {
            _ivAppLogo = findViewById(R.id.ivAppLogo);
            _ivAppLogo.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    _numberOfClicksOfAppLogo++;
                    if(_numberOfClicksOfAppLogo >= 7)
                    {
                        _numberOfClicksOfAppLogo = 0;
                        boolean devModeActive = Globals.getSharedPreferences().getBoolean(PreferenceKeys.DEVELOPER_MODE_ACTIVE, false);
                        devModeActive = !devModeActive;
                        Globals.getSharedPreferencesEditor().putBoolean(PreferenceKeys.DEVELOPER_MODE_ACTIVE, devModeActive);
                        Globals.getSharedPreferencesEditor().apply();

                        if(devModeActive)
                        {
                            Toast.makeText(AboutActivity.this, getString(R.string.developer_mode_activated), Toast.LENGTH_LONG).show();
                        }
                        else
                        {
                            Toast.makeText(AboutActivity.this, getString(R.string.developer_mode_deactivated), Toast.LENGTH_LONG).show();
                        }
                    }
                }
            });

            _tvAppName = findViewById(R.id.tvAppName);
            _tvAppName.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    _numberOfClicksOfAppName++;
                    if(_numberOfClicksOfAppName >= 5)
                    {
                        _numberOfClicksOfAppName = 0;
                        boolean advModeActive = Globals.getSharedPreferences().getBoolean(PreferenceKeys.ADVANCED_MODE_ACTIVE, false);
                        advModeActive = !advModeActive;
                        Globals.getSharedPreferencesEditor().putBoolean(PreferenceKeys.ADVANCED_MODE_ACTIVE, advModeActive);
                        Globals.getSharedPreferencesEditor().apply();

                        if(advModeActive)
                        {
                            Toast.makeText(AboutActivity.this, getString(R.string.advanced_mode_activated), Toast.LENGTH_LONG).show();
                        }
                        else
                        {
                            Toast.makeText(AboutActivity.this, getString(R.string.advanced_mode_deactivated), Toast.LENGTH_LONG).show();
                        }
                    }
                }
            });
        }

        String ei = Globals.getSharedPreferences().getString(PreferenceKeys.USER_ENTERPRISE_ID, "");
        String key = Globals.getSharedPreferences().getString(PreferenceKeys.USER_LICENSING_KEY, "");
        String ac = Globals.getSharedPreferences().getString(PreferenceKeys.USER_LICENSING_ACTIVATION_CODE, "");

        // Get the license descriptor using the existing license key and activation code (if any)
        _activeLd = parseIntoInternalDescriptor(Globals.getEngageApplication()
                                                .getEngine()
                                                .engageGetLicenseDescriptor(getString(R.string.licensing_entitlement), key, ac, getString(R.string.manufacturer_id)));

        // At this point, the new one is the same as the active one
        _newLd = new InternalDescriptor(_activeLd);
        _newLd._needsSaving = false;

        _tvLicenseHeader = findViewById(R.id.tvLicenseHeader);
        _tvLicensingMessage = findViewById(R.id.tvLicensingMessage);
        _etDeviceId = findViewById(R.id.etDeviceId);
        _etEnterpriseId = findViewById(R.id.etEnterpriseId);
        _etEnterpriseId.setText(ei);
        _etEnterpriseId.addTextChangedListener(new TextWatcher()
        {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after)
            {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count)
            {
            }

            @Override
            public void afterTextChanged(Editable s)
            {
                String ei = s.toString();
                Globals.getSharedPreferencesEditor().putString(PreferenceKeys.USER_ENTERPRISE_ID, ei);
                Globals.getSharedPreferencesEditor().apply();
            }
        });

        _etLicenseKey = findViewById(R.id.etLicenseKey);
        _etLicenseKey.setFilters(new InputFilter[]{new InputFilter.AllCaps()});
        _etLicenseKey.addTextChangedListener(new TextWatcher()
        {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after)
            {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count)
            {
            }

            @Override
            public void afterTextChanged(Editable s)
            {
                _etActivationCode.setText(null);
                userChangedLicensedData();
            }
        });
        _etActivationCode = findViewById(R.id.etActivationCode);
        _etActivationCode.setFilters(new InputFilter[]{new InputFilter.AllCaps()});
        _etActivationCode.addTextChangedListener(new TextWatcher()
        {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after)
            {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count)
            {
            }

            @Override
            public void afterTextChanged(Editable s)
            {
                userChangedLicensedData();
            }
        });

        _ivScanLicenseKey = findViewById(R.id.ivScanLicenseKey);
        _ivLoadLicenseKey = findViewById(R.id.ivLoadLicenseKey);
        _ivScanActivationCode = findViewById(R.id.ivScanActivationCode);
        _ivWebFetchActivationCode = findViewById(R.id.ivWebFetchActivationCode);

        String s = _activeLd._ld._deviceId;
        if(Utils.isEmptyString(s))
        {
            s = getString(R.string.unknown_device_id);
        }

        _etDeviceId.setText(s);
        _etLicenseKey.setText(_activeLd._ld._key);
        _etActivationCode.setText(_activeLd._ld._activationCode);

        // Maybe lock down licensing
        if(Globals.getContext().getResources().getBoolean(R.bool.opt_locked_licensing))
        {
            _etLicenseKey.setEnabled(false);
            _etActivationCode.setEnabled(false);
            _ivScanLicenseKey.setVisibility(View.GONE);
            _ivLoadLicenseKey.setVisibility(View.GONE);
            _ivScanActivationCode.setVisibility(View.GONE);
            _ivWebFetchActivationCode.setVisibility(View.GONE);
        }

        String versionInfo;

        versionInfo = BuildConfig.VERSION_NAME + " (Engage Engine " + Globals.getEngageApplication().getEngine().engageGetVersion() + ")"; //NON-NLS

        ((TextView)findViewById(R.id.tvVersion)).setText(versionInfo);

        setTitle(R.string.title_about);

        updateUi();

        _creating = false;
    }

    @Override
    protected void onResume()
    {
        Globals.getEngageApplication().pauseLicenseActivation();
        super.onResume();
    }

    @Override
    protected void onStop()
    {
        super.onStop();
    }

    private void clearStoredLicensing()
    {
        Globals.getSharedPreferencesEditor().putString(PreferenceKeys.USER_LICENSING_KEY, "");
        Globals.getSharedPreferencesEditor().putString(PreferenceKeys.USER_LICENSING_ACTIVATION_CODE, "");
        Globals.getSharedPreferencesEditor().apply();

        // Put the new license into effect
        Globals.getEngageApplication().getEngine().engageUpdateLicense(getString(R.string.licensing_entitlement), "", "", getString(R.string.manufacturer_id));
    }

    private void saveLicenseData()
    {
        if(Utils.isEmptyString(_newLd._ld._key) || (_newLd.isValid() && _newLd._needsSaving))
        {
            _newLd._needsSaving = false;

            String key = Utils.emptyAs(_newLd._ld._key, "");
            String ac = Utils.emptyAs(_newLd._ld._activationCode, "");

            if(Utils.isEmptyString(key))
            {
                ac = "";
            }

            Globals.getLogger().i(TAG, "saving licensing [" + getString(R.string.licensing_entitlement) + "] [" + key + "] [" + ac + "]"); //NON-NLS

            Globals.getSharedPreferencesEditor().putString(PreferenceKeys.USER_LICENSING_KEY, key);
            Globals.getSharedPreferencesEditor().putString(PreferenceKeys.USER_LICENSING_ACTIVATION_CODE, ac);
            Globals.getSharedPreferencesEditor().apply();

            // Put the new license into effect
            Globals.getEngageApplication().getEngine().engageUpdateLicense(getString(R.string.licensing_entitlement), key, ac, getString(R.string.manufacturer_id));

            Globals.getEngageApplication().logEvent(Analytics.NEW_LICENSE_FROM_USER);
        }
    }

    private void userChangedLicensedData()
    {
        if(!_creating)
        {
            updateNewLdFromEnteredData();
            updateUi();
        }
    }

    private InternalDescriptor parseIntoInternalDescriptor(String jsonData)
    {
        InternalDescriptor rc = new InternalDescriptor();
        rc._ld = LicenseDescriptor.fromJson(jsonData);

        return rc;
    }

    private void updateNewLdFromEnteredData()
    {
        String key = _etLicenseKey.getText().toString();
        String ac = _etActivationCode.getText().toString();

        String entitlement = getString(R.string.licensing_entitlement);
        String mnfId = getString(R.string.manufacturer_id);

        String ldJson = Globals.getEngageApplication().getEngine().engageGetLicenseDescriptor(entitlement, key, ac, mnfId);

        _newLd = parseIntoInternalDescriptor(ldJson);

        _newLd._needsSaving = true;
    }

    private void updateUi()
    {
        String msg;

        if(_activeLd.equals(_newLd))
        {
            // If active and new are the same, we'll build from the active
            if(_activeLd.isValid())
            {
                if(_activeLd._ld._theType == Engine.LicenseType.expires)
                {
                    Date dt = new Date();
                    if(_activeLd._ld._expires.after(dt))
                    {
                        msg = String.format(getString(R.string.your_license_expires_on), Utils.formattedTimespan(dt.getTime(), _activeLd._ld._expires.getTime()));
                    }
                    else
                    {
                        msg = String.format(getString(R.string.this_license_expired_on), _activeLd._ld._expiresFormatted);
                    }
                }
                else
                {
                    if(Utils.isEmptyString(_activeLd._ld._activationCode))
                    {
                        msg = getString(R.string.existing_license_never_expires_but_requires_activation);
                    }
                    else
                    {
                        msg = getString(R.string.your_license_is_active);
                    }
                    //tryAutoActivate = true;
                }
            }
            else
            {
                msg = getString(R.string.you_dont_yet_have_a_valid_license);
            }
        }
        else
        {
            if(_newLd.isValid())
            {
                if(_newLd._ld._theType == Engine.LicenseType.expires && _newLd._ld._expires != null)
                {
                    Date dt = new Date();
                    if(_newLd._ld._expires.after(dt))
                    {
                        msg = String.format(getString(R.string.this_license_expires_on), Utils.formattedTimespan(dt.getTime(), _newLd._ld._expires.getTime()));
                    }
                    else
                    {
                        msg = String.format(getString(R.string.this_license_expired_on), _newLd._ld._expiresFormatted);
                    }
                }
                else
                {
                    if(Utils.isEmptyString(_newLd._ld._activationCode))
                    {
                        msg = getString(R.string.this_license_never_expires_but_requires_activation);
                    }
                    else
                    {
                        msg = getString(R.string.your_license_is_active);
                    }
                    //tryAutoActivate = true;
                }
            }
            else
            {
                if(Utils.isEmptyString(_newLd._ld._key))
                {
                    msg = getString(R.string.you_dont_yet_have_a_valid_license);
                }
                else
                {
                    msg = getString(R.string.not_a_valid_license);
                }
            }
        }

        _tvLicensingMessage.setText(msg);

        if(Globals.getContext().getResources().getBoolean(R.bool.opt_locked_licensing))
        {
            findViewById(R.id.btnDeactivate).setVisibility(View.INVISIBLE);
            findViewById(R.id.ivShareLicenseKey).setVisibility(View.GONE);

            _ivScanLicenseKey.setVisibility(View.GONE);
            _ivLoadLicenseKey.setVisibility(View.GONE);
            
            _etActivationCode.setEnabled(false);
            _ivScanActivationCode.setVisibility(View.GONE);
            _ivWebFetchActivationCode.setVisibility(View.GONE);

            findViewById(R.id.layActivationSection).setVisibility(View.GONE);
            findViewById(R.id.ivShareEnterpriseId).setVisibility(View.GONE);
            findViewById(R.id.layEnterpriseIdSection).setVisibility(View.GONE);
        }
        else
        {
            if(Utils.isEmptyString(_activeLd._ld._activationCode) || Utils.isEmptyString(_etActivationCode.getText().toString()))
            {
                findViewById(R.id.btnDeactivate).setVisibility(View.INVISIBLE);
            }
            else
            {
                findViewById(R.id.btnDeactivate).setVisibility(View.VISIBLE);
            }

            if(Utils.isEmptyString(_etLicenseKey.getText().toString()))
            {
                findViewById(R.id.ivShareLicenseKey).setVisibility(View.GONE);
            }
            else
            {
                findViewById(R.id.ivShareLicenseKey).setVisibility(View.VISIBLE);
            }

            String key = _etLicenseKey.getText().toString();
            String ac = _etActivationCode.getText().toString();

            if(Utils.isEmptyString(ac))
            {
                _etLicenseKey.setEnabled(true);
                _ivScanLicenseKey.setVisibility(View.VISIBLE);
                _ivLoadLicenseKey.setVisibility(View.VISIBLE);
            }
            else
            {
                _etLicenseKey.setEnabled(false);
                _ivScanLicenseKey.setVisibility(View.GONE);
                _ivLoadLicenseKey.setVisibility(View.GONE);
            }

            if(Utils.isEmptyString(key))
            {
                _etActivationCode.setEnabled(false);
                _ivScanActivationCode.setVisibility(View.GONE);
                _ivWebFetchActivationCode.setVisibility(View.GONE);
            }
            else
            {
                _etActivationCode.setEnabled(true);
                _ivScanActivationCode.setVisibility(View.VISIBLE);
                _ivWebFetchActivationCode.setVisibility(View.VISIBLE);
            }

            if( (_activeLd._ld._status == Engine.LicensingStatusCode.requiresActivation) ||
                (_newLd._ld._status == Engine.LicensingStatusCode.requiresActivation) )
            {
                findViewById(R.id.layActivationSection).setVisibility(View.VISIBLE);
            }
            else
            {
                findViewById(R.id.layActivationSection).setVisibility(View.GONE);
            }

            if(Globals.getContext().getResources().getBoolean(R.bool.opt_supports_enterprise_id))
            {
                findViewById(R.id.layEnterpriseIdSection).setVisibility(View.VISIBLE);

                String ei = _etEnterpriseId.getText().toString();
                if(!Utils.isEmptyString(ei))
                {
                    findViewById(R.id.ivShareEnterpriseId).setVisibility(View.VISIBLE);
                }
                else
                {
                    findViewById(R.id.ivShareEnterpriseId).setVisibility(View.GONE);
                }
            }
            else
            {
                findViewById(R.id.layEnterpriseIdSection).setVisibility(View.GONE);
            }
        }
    }

    @Override
    public void finish()
    {
        saveLicenseData();
        Globals.getEngageApplication().resumeLicenseActivation();
        super.finish();
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

    private JSONObject parseLicensing(String s)
    {
        JSONObject rc = null;

        if(!Utils.isEmptyString(s))
        {
            String localTrimmedCopy = Utils.trimString(s);
            if(!Utils.isEmptyString(localTrimmedCopy))
            {
                try
                {
                    rc = new JSONObject(Utils.trimString(localTrimmedCopy));
                }
                catch (Exception e)
                {
                    try
                    {
                        // If its not JSON then assume its just a license key.  And that license key MUST be 24 characters!
                        if(localTrimmedCopy.length() == 24)
                        {
                            rc = new JSONObject();

                            JSONObject licensingSubobject = new JSONObject();
                            licensingSubobject.put(Engine.JsonFields.EnginePolicy.Licensing.key, localTrimmedCopy);

                            rc.put(Engine.JsonFields.EnginePolicy.Licensing.objectName, licensingSubobject);
                        }
                        else
                        {
                            rc = null;
                        }
                    }
                    catch (Exception innerE)
                    {
                        rc = null;
                    }
                }
            }
        }

        return rc;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent)
    {
        /*
        String stringData = null;

        JSONObject complexObject = null;
        String licenseKey = null;
        String activationCode = null;
        String enterpriseId = null;


        try
        {
            // First, try to see if we have a URI to read from
            Uri uri = intent.getData();

            if(uri != null)
            {
                byte[] byteData = Utils.readBinaryFile(this, uri);

                // Perhaps it's a a bitmap (representing a QR code)
                Bitmap bm = BitmapFactory.decodeByteArray(byteData, 0, byteData.length);
                if(bm != null)
                {
                    stringData = Utils.qrCodeBitmapToString(bm);
                }
                else
                {
                    // Its not a bitmap.  Maybe its just a string
                    // TODO
                }
            }
            else
            {
                // OK, no URI, maybe its a camera scan of a QR code
                IntentResult result = IntentIntegrator.parseActivityResult(requestCode, resultCode, intent);
                if(result != null)
                {
                    stringData = result.getContents();
                }
            }
        }
        catch (Exception e)
        {
            complexObject = null;
        }
        */

         if(requestCode == Globals.getEngageApplication().getQrCodeScannerRequestCode())
        {
            _scanning = false;

            IntentResult result = IntentIntegrator.parseActivityResult(requestCode, resultCode, intent);
            if(result != null)
            {
                if (_scanType == ScanType.stLicenseKey || _scanType == ScanType.stActivationCode)
                {
                    JSONObject jsonLicensing = parseLicensing(result.getContents());

                    if (jsonLicensing != null)
                    {
                        try
                        {
                            if (_scanType == ScanType.stLicenseKey)
                            {
                                _etLicenseKey.setText(jsonLicensing.optJSONObject(Engine.JsonFields.EnginePolicy.Licensing.objectName).optString(Engine.JsonFields.EnginePolicy.Licensing.key, "").toUpperCase());
                                updateUi();
                            }
                            else if (_scanType == ScanType.stActivationCode)
                            {
                                _etActivationCode.setText(jsonLicensing.optJSONObject(Engine.JsonFields.EnginePolicy.Licensing.objectName).optString(Engine.JsonFields.EnginePolicy.Licensing.activationCode, "").toUpperCase());
                                updateUi();
                            }
                        }
                        catch (Exception e)
                        {
                            Utils.showErrorMsg(this, getString(R.string.failed_to_load_license_key_data));
                        }
                    }
                    else
                    {
                        Utils.showErrorMsg(this, getString(R.string.failed_to_load_license_key_data));
                    }
                }
                else if (_scanType == ScanType.stEnterpriseId)
                {
                    String scannedString = result.getContents();
                    if(!Utils.isEmptyString(scannedString))
                    {
                        scannedString = Utils.trimString(scannedString);
                        _etEnterpriseId.setText(scannedString);
                        updateUi();
                    }
                }
            }
        }
        else if(requestCode == Constants.OFFLINE_ACTIVATION_REQUEST_CODE)
        {
            if(intent != null)
            {
                String activationCode = intent.getStringExtra(OfflineActivationActivity.EXTRA_ACTIVATION_CODE);
                if (!Utils.isEmptyString(activationCode))
                {
                    _etActivationCode.setText(activationCode);
                    updateUi();
                }
            }
        }
        else if(requestCode == Constants.PICK_LICENSE_FILE_REQUEST_CODE)
        {
            if(intent != null)
            {
                if (resultCode == RESULT_OK)
                {
                    boolean ok = false;

                    try
                    {
                        String fileData = Utils.readTextFile(AboutActivity.this, intent.getData());

                        if(!Utils.isEmptyString(fileData))
                        {
                            JSONObject jsonLicensing = parseLicensing(fileData);

                            if (jsonLicensing != null)
                            {
                                try
                                {
                                    _etLicenseKey.setText(jsonLicensing.optJSONObject(Engine.JsonFields.EnginePolicy.Licensing.objectName).optString(Engine.JsonFields.EnginePolicy.Licensing.key, "").toUpperCase());
                                    updateUi();
                                    ok = true;

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
                        ok = false;
                    }

                    if(!ok)
                    {
                        Utils.showErrorMsg(AboutActivity.this, getString(R.string.failed_to_load_license_key_data));
                    }
                }
            }
        }
        /*
        else if(requestCode == Constants.PICK_ENTERPRISE_ID_FILE_REQUEST_CODE)
        {
            if(intent != null)
            {
                if (resultCode == RESULT_OK)
                {
                    boolean ok = false;

                    try
                    {
                        String idString = Utils.readTextFile(AboutActivity.this, intent.getData());

                        if(!Utils.isEmptyString(idString))
                        {
                            Utils.trimString(idString);
                            idString = idString.toUpperCase();
                            _etEnterpriseId.setText(idString);
                            updateUi();
                            ok = true;
                        }
                    }
                    catch (Exception e)
                    {
                        e.printStackTrace();
                        ok = false;
                    }

                    if(!ok)
                    {
                        Utils.showErrorMsg(AboutActivity.this, getString(R.string.failed_to_load_enterprise_id_data));
                    }
                }
            }
        }
        else if(requestCode == Constants.PICK_QR_LICENSE_FILE_REQUEST_CODE || requestCode == Constants.PICK_QR_ACTIVATION_FILE_REQUEST_CODE)
        {
            boolean ok = false;

            try
            {
                Uri uri = intent.getData();
                if(uri != null)
                {
                    byte[] qrCodeBytesFromFile = Utils.readBinaryFile(this, uri);

                    Bitmap bm = BitmapFactory.decodeByteArray(qrCodeBytesFromFile, 0, qrCodeBytesFromFile.length);
                    if(bm != null)
                    {
                        String s = Utils.qrCodeBitmapToString(bm);
                        if(!Utils.isEmptyString(s))
                        {
                            Log.e(TAG, s);

                            if(requestCode == Constants.PICK_QR_LICENSE_FILE_REQUEST_CODE)
                            {
                                JSONObject jo = new JSONObject(s);
                                String license = jo.optJSONObject("licensing").optString("key");
                                if(!Utils.isEmptyString(license))
                                {
                                    Utils.trimString(license);
                                    if(license.length() == 24)
                                    {
                                        license = license.toUpperCase();
                                        _etLicenseKey.setText(license);
                                        updateUi();
                                        ok = true;
                                    }
                                }
                            }
                            else
                            {

                            }
                        }
                    }
                }
            }
            catch (Exception e)
            {
                e.printStackTrace();
                ok = false;
            }

            if(!ok)
            {
                Utils.showErrorMsg(AboutActivity.this, getString(R.string.failed_to_load_license_key_data));
            }
        }
        */
    }

    private String machineInfo()
    {
        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        int densityDpi = (int) (dm.density * 160f);
        //double x = Math.pow(mWidthPixels / dm.xdpi, 2);
        //double y = Math.pow(mHeightPixels / dm.ydpi, 2);
        //int screenInches = Math.sqrt(x + y);
        //int rounded = df2.format(screenInches);


        StringBuilder sb = new StringBuilder();

        sb.append(getString(R.string.mi_hdr_system));
        sb.append(String.format(getString(R.string.mi_sys_id), Settings.Secure.getString(getContentResolver(), Settings.Secure.ANDROID_ID)));
        sb.append(String.format(getString(R.string.mi_sys_sizing_type), getString(R.string.ui_sizing_type)));

        sb.append(getString(R.string.mi_hdr_device));
        sb.append(String.format(getString(R.string.mi_device_manufacturer), Build.MANUFACTURER));
        sb.append(String.format(getString(R.string.mi_device_id), Build.ID));
        sb.append(String.format(getString(R.string.mi_device_brand), Build.BRAND));
        sb.append(String.format(getString(R.string.mi_device_model), Build.MODEL));
        sb.append(String.format(getString(R.string.mi_device_board), Build.BOARD));
        sb.append(String.format(getString(R.string.mi_device_hardware), Build.HARDWARE));
        sb.append(String.format(getString(R.string.mi_device_serial), Build.SERIAL));
        sb.append(String.format(getString(R.string.mi_device_bootloader), Build.BOOTLOADER));
        sb.append(String.format(getString(R.string.mi_device_user), Build.USER));
        sb.append(String.format(getString(R.string.mi_device_host), Build.HOST));
        sb.append(String.format(getString(R.string.mi_device_build_time), Long.toString(Build.TIME)));
        sb.append(String.format(getString(R.string.mi_device_version_release), Build.VERSION.RELEASE));
        sb.append(String.format(getString(R.string.mi_device_sdk_int), Build.VERSION.SDK_INT));

        sb.append(getString(R.string.mi_hdr_screen));
        sb.append(String.format(getString(R.string.mi_screen_density), dm.density));
        sb.append(String.format(getString(R.string.mi_screen_height), dm.heightPixels));
        sb.append(String.format(getString(R.string.mi_screen_width), dm.widthPixels));
        sb.append(String.format(getString(R.string.mi_screen_density), dm.density));
        sb.append(String.format(getString(R.string.mi_screen_scaled_density), dm.scaledDensity));
        sb.append(String.format(getString(R.string.mi_screen_density_dpi), dm.densityDpi));
        sb.append(String.format(getString(R.string.mi_screen_xdpi), dm.xdpi));
        sb.append(String.format(getString(R.string.mi_screen_ydpi), dm.ydpi));

        return sb.toString();
    }

    private void scanData(String prompt, ScanType st, View sourceMenuAnchor, String fileChooserPrompt, int fileChooserRequestCode)
    {
        _scanType = st;
        _scanning = true;

        Globals.getEngageApplication().scanQrCode(this, prompt, sourceMenuAnchor, fileChooserPrompt, fileChooserRequestCode);
    }

    public void onClickContact(View view)
    {
        Intent intent = new Intent(this, ContactActivity.class);
        startActivity(intent);
    }

    public void onClickSystemInfo(View view)
    {
        String s;

        s = machineInfo();

        final TextView message = new TextView(this);
        final SpannableString ss = new SpannableString(s);

        message.setText(ss);
        message.setMovementMethod(LinkMovementMethod.getInstance());
        message.setPadding(32, 32, 32, 32);

        AlertDialog dlg = new AlertDialog.Builder(this)
                .setTitle(getString(R.string.system_information))
                .setCancelable(true)
                .setNegativeButton(R.string.button_close, null)
                .setView(message).create();

        dlg.show();
    }

    public void onClickSetOrClearEnterpriseId(View view)
    {
        if(Globals.getEngageApplication().isActiveMissionIdFixedSampleId())
        {
            Utils.showErrorMsg(this, getString(R.string.cannot_change_enterprise_id_in_use));
            return;
        }

        String ei = Utils.trimString(_etEnterpriseId.getText().toString());

        PopupMenu popup = new PopupMenu(this, findViewById(R.id.ivSetOrClearEnterpriseId));

        MenuInflater inflater = popup.getMenuInflater();
        inflater.inflate(R.menu.about_box_enterprise_menu, popup.getMenu());

        // Hide the clear option if we don't have anything
        popup.getMenu().findItem(R.id.action_clear_id).setVisible(!Utils.isEmptyString(ei));

        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener()
        {
            @Override
            public boolean onMenuItemClick(MenuItem item)
            {
                int id = item.getItemId();

                if (id == R.id.action_set_id)
                {
                    promptForEnterpriseId();
                    return true;
                }
                else if (id == R.id.action_clear_id)
                {
                    promptToClearEnterpriseId();
                    return true;
                }

                return false;
            }
        });

        popup.show();
    }

    public void onClickLoadEnterpriseId(View view)
    {
        try
        {
            Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
            intent.setType("*/*");
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            startActivityForResult(Intent.createChooser(intent, getString(R.string.select_a_file)), Constants.PICK_ENTERPRISE_ID_FILE_REQUEST_CODE);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public void onClickShareEnterpriseId(View view)
    {
        String idString = _etEnterpriseId.getText().toString();
        if(Utils.isEmptyString(idString))
        {
            return;
        }

        Bitmap bm = Utils.stringToQrCodeBitmap(idString, Constants.QR_CODE_WIDTH, Constants.QR_CODE_HEIGHT);

        LayoutInflater layoutInflater = LayoutInflater.from(this);
        View dialogView = layoutInflater.inflate(R.layout.qr_code_displayer, null);
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);
        alertDialogBuilder.setView(dialogView);
        alertDialogBuilder.setCancelable(true);
        AlertDialog alert = alertDialogBuilder.create();

        ImageView iv = dialogView.findViewById(R.id.ivQrCode);
        iv.setImageBitmap(bm);

        alert.show();
    }

    public void onClickScanEnterpriseId(View view)
    {
        scanData(getString(R.string.scan_enterprise_id), ScanType.stEnterpriseId, view, getString(R.string.select_qr_code_file), Constants.PICK_QR_ENTERPRISE_ID_FILE_REQUEST_CODE);
    }

    public void onClickShareLicenseKey(View view)
    {
        String licenseString = _etLicenseKey.getText().toString();
        if(Utils.isEmptyString(licenseString))
        {
            return;
        }

        Bitmap bm = Utils.stringToQrCodeBitmap(licenseString, Constants.QR_CODE_WIDTH, Constants.QR_CODE_HEIGHT);

        LayoutInflater layoutInflater = LayoutInflater.from(this);
        View dialogView = layoutInflater.inflate(R.layout.qr_code_displayer, null);
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);
        alertDialogBuilder.setView(dialogView);
        alertDialogBuilder.setCancelable(true);
        AlertDialog alert = alertDialogBuilder.create();

        ImageView iv = dialogView.findViewById(R.id.ivQrCode);
        iv.setImageBitmap(bm);

        alert.show();
    }

    public void onClickLoadLicenseKey(View view)
    {
        try
        {
            Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
            intent.setType("*/*");
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            startActivityForResult(Intent.createChooser(intent, getString(R.string.select_a_file)), Constants.PICK_LICENSE_FILE_REQUEST_CODE);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public void onClickScanLicenseKey(View view)
    {
        scanData(getString(R.string.scan_license_key), ScanType.stLicenseKey, view, getString(R.string.select_qr_code_file), Constants.PICK_QR_LICENSE_FILE_REQUEST_CODE);
    }

    public void onClickScanActivationCode(View view)
    {
        scanData(getString(R.string.scan_activation_code), ScanType.stActivationCode, view, getString(R.string.select_qr_code_file), Constants.PICK_QR_ACTIVATION_FILE_REQUEST_CODE);
    }

    public void onClickGetActivationCodeOnline(View view)
    {
        attemptOnlineActivation();
    }

    public void onClickDeactivate(View view)
    {
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);

        alertDialogBuilder.setTitle(R.string.about_deactivate_title);

        alertDialogBuilder.setMessage(R.string.about_deactivate_really_sure);
        alertDialogBuilder.setCancelable(true);
        alertDialogBuilder.setNegativeButton(R.string.button_no, new DialogInterface.OnClickListener()
        {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
                dialog.cancel();
            }
        });
        alertDialogBuilder.setPositiveButton(R.string.button_yes, new DialogInterface.OnClickListener()
        {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
                attemptOnlineDeactivation();
            }
        });

        AlertDialog dlg = alertDialogBuilder.create();
        dlg.show();
    }


    @Override
    public void onLicenseActivationTaskComplete(int result, String activationCode, String resultMessage)
    {
        _progressDialog = Utils.hideProgressMessage(_progressDialog);

        if(result == 0 && !Utils.isEmptyString(activationCode))
        {
            _etActivationCode.setText(activationCode);
        }
        else
        {
            promptForOfflineActivation();
        }
    }

    private void attemptOnlineActivation()
    {
        String url;
        if(Globals.getSharedPreferences().getBoolean(PreferenceKeys.DEVELOPER_USE_DEV_LICENSING_SYSTEM, false))
        {
            url = getString(R.string.online_licensing_activation_url_dev);
        }
        else
        {
            url = getString(R.string.online_licensing_activation_url_prod);
        }

        String key = _etLicenseKey.getText().toString();
        String ac = _etActivationCode.getText().toString();
        String entitlementKey = getString(R.string.licensing_entitlement);

        if(Utils.isEmptyString(key))
        {
            Utils.showErrorMsg(this, getString(R.string.about_invalid_license_key));
            return;
        }

        if(Utils.isEmptyString(entitlementKey))
        {
            Utils.showErrorMsg(this, getString(R.string.about_invalid_entitlement_key));
            return;
        }

        try
        {
            LicenseDescriptor testDescriptor = LicenseDescriptor.fromJson(Globals.getEngageApplication()
                    .getEngine()
                    .engageGetLicenseDescriptor(Globals.getEngageApplication().getString(R.string.licensing_entitlement),
                            key,
                            ac,
                            Globals.getEngageApplication().getString(R.string.manufacturer_id)));

            if (testDescriptor == null)
            {
                Utils.showErrorMsg(this, getString(R.string.failed_to_validate_license_info));
                throw new Exception("");
            }

            if (testDescriptor._status != Engine.LicensingStatusCode.ok && testDescriptor._status != Engine.LicensingStatusCode.requiresActivation)
            {
                Utils.showErrorMsg(this, getString(R.string.about_invalid_license_key));
                throw new Exception("");
            }

            String stringToHash = key + _activeLd._ld._deviceId + entitlementKey;
            String hValue = Utils.md5HashOfString(stringToHash);

            LicenseActivationTask lat = new LicenseActivationTask(url, getString(R.string.licensing_entitlement), key, ac, _activeLd._ld._deviceId, hValue, this);

            _progressDialog = Utils.showProgressMessage(this, getString(R.string.obtaining_activation_code), _progressDialog);
            lat.execute();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    @Override
    public void onLicenseDeactivationTaskComplete(int result, String resultMessage)
    {
        _progressDialog = Utils.hideProgressMessage(_progressDialog);

        if(result == 0)
        {
            runOnUiThread(new Runnable()
            {
                @Override
                public void run()
                {
                    Globals.getEngageApplication().logEvent(Analytics.LICENSE_DEACTIVATED);
                    _etLicenseKey.setText(null);
                    _etActivationCode.setText(null);
                    clearStoredLicensing();
                    userChangedLicensedData();
                    updateUi();
                    Toast.makeText(AboutActivity.this, R.string.deactivated, Toast.LENGTH_LONG).show();
                    recreate();
                }
            });
        }
        else
        {
            promptForOfflineDeactivation();
        }
    }

    private void attemptOnlineDeactivation()
    {
        String url;
        if(Globals.getSharedPreferences().getBoolean(PreferenceKeys.DEVELOPER_USE_DEV_LICENSING_SYSTEM, false))
        {
            url = getString(R.string.online_licensing_deactivation_url_dev);
        }
        else
        {
            url = getString(R.string.online_licensing_deactivation_url_prod);
        }

        String key = _etLicenseKey.getText().toString();

        String entitlementKey = getString(R.string.licensing_entitlement);

        String stringToHash = key + _activeLd._ld._deviceId + entitlementKey;
        String hValue = Utils.md5HashOfString(stringToHash);

        LicenseDeactivationTask ldt = new LicenseDeactivationTask(this, url, key, _activeLd._ld._deviceId, hValue, this);

        _progressDialog = Utils.showProgressMessage(this, getString(R.string.deactivating), _progressDialog);
        ldt.execute();
    }

    private void promptForOfflineDeactivation()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(AboutActivity.this);

                alertDialogBuilder.setTitle(R.string.about_deactivate_title);

                alertDialogBuilder.setMessage(R.string.deactivation_failed_try_offline);
                alertDialogBuilder.setCancelable(true);
                alertDialogBuilder.setNegativeButton(R.string.button_no, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        dialog.cancel();
                    }
                });
                alertDialogBuilder.setPositiveButton(R.string.button_yes, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        Globals.getEngageApplication().logEvent(Analytics.LICENSE_DEACTIVATED);

                        Intent intent = new Intent(AboutActivity.this, OfflineDeactivationActivity.class);
                        intent.putExtra(OfflineActivationActivity.EXTRA_DEVICE_ID, _activeLd._ld._deviceId);
                        intent.putExtra(OfflineActivationActivity.EXTRA_LICENSE_KEY, _etLicenseKey.getText().toString());

                        _etLicenseKey.setText(null);
                        _etActivationCode.setText(null);
                        clearStoredLicensing();
                        userChangedLicensedData();
                        updateUi();
                        Toast.makeText(AboutActivity.this, R.string.deactivated, Toast.LENGTH_LONG).show();
                        recreate();

                        startActivity(intent);
                    }
                });

                AlertDialog dlg = alertDialogBuilder.create();
                dlg.show();
            }
        });
    }

    private void promptForOfflineActivation()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(AboutActivity.this);

                alertDialogBuilder.setTitle(R.string.about_activate_title);

                alertDialogBuilder.setMessage(R.string.activation_failed_try_offline);
                alertDialogBuilder.setCancelable(true);
                alertDialogBuilder.setNegativeButton(R.string.button_no, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        dialog.cancel();
                    }
                });
                alertDialogBuilder.setPositiveButton(R.string.button_yes, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        String key = _etLicenseKey.getText().toString();
                        if(Utils.isEmptyString(key))
                        {
                            Utils.showErrorMsg(AboutActivity.this, getString(R.string.about_invalid_license_key));
                            return;
                        }

                        Intent intent = new Intent(AboutActivity.this, OfflineActivationActivity.class);
                        intent.putExtra(OfflineActivationActivity.EXTRA_DEVICE_ID, _activeLd._ld._deviceId);
                        intent.putExtra(OfflineActivationActivity.EXTRA_LICENSE_KEY, _etLicenseKey.getText().toString());
                        startActivityForResult(intent, Constants.OFFLINE_ACTIVATION_REQUEST_CODE);
                    }
                });

                AlertDialog dlg = alertDialogBuilder.create();
                dlg.show();
            }
        });
    }

    private void promptToClearEnterpriseId()
    {
        final TextView message = new TextView(this);
        final SpannableString ss = new SpannableString(getString(R.string.clearing_ent_id_will_delete_mission));

        message.setText(ss);
        message.setMovementMethod(LinkMovementMethod.getInstance());
        message.setPadding(32, 32, 32, 32);

        AlertDialog dlg = new AlertDialog.Builder(this)
                .setTitle(R.string.clear_enterprise_id_title)
                .setCancelable(false)
                .setPositiveButton(getString(R.string.button_yes), new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                        clearEnterpriseId();
                    }
                }).setNegativeButton(getString(R.string.button_no), new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                    }
                }).setView(message).create();

        dlg.show();
    }

    private void promptForEnterpriseId()
    {
        LayoutInflater layoutInflater = LayoutInflater.from(AboutActivity.this);
        View promptView = layoutInflater.inflate(R.layout.enterprise_id_entry_dialog, null);
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(AboutActivity.this);
        alertDialogBuilder.setView(promptView);

        final EditText etEi = promptView.findViewById(R.id.etEnterpriseId);

        alertDialogBuilder.setCancelable(false)
                .setPositiveButton(R.string.button_ok, new DialogInterface.OnClickListener()
                {
                    public void onClick(DialogInterface dialog, int id)
                    {
                        try
                        {
                            String ei = etEi.getText().toString();
                            if(!Utils.isEmptyString(ei))
                            {
                                updateToNewEnterpriseId(ei);
                            }
                        }
                        catch (Exception e)
                        {
                            e.printStackTrace();
                        }
                    }
                })
                .setNegativeButton(R.string.cancel,
                        new DialogInterface.OnClickListener()
                        {
                            public void onClick(DialogInterface dialog, int id)
                            {
                                dialog.cancel();
                            }
                        });

        AlertDialog alert = alertDialogBuilder.create();
        alert.show();
    }

    private void clearEnterpriseId()
    {
        _etEnterpriseId.setText(null);
        updateUi();

        Globals.getEngageApplication().deleteFixedIdSampleMission();
    }

    private void updateToNewEnterpriseId(String ei)
    {
        _etEnterpriseId.setText(ei);
        String newId = Globals.getEngageApplication().createOrReplaceFixedIdSampleConfiguration(true);
        if(!Utils.isEmptyString(newId))
        {
            String newName = ActiveConfiguration.getMissionNameForId(newId);
            if(Utils.isEmptyString(newName))
            {
                newName = getString(R.string.no_mission_name);
            }

            String message = String.format(getString(R.string.new_sample_mission_msg_fmt), newName);
            Utils.showMessageInDialog(AboutActivity.this, getString(R.string.new_sample_mission_title), message);
        }

        updateUi();
    }
}
