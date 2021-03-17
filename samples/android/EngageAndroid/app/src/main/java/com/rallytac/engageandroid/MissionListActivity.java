//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.preference.PreferenceManager;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.zxing.integration.android.IntentIntegrator;
import com.google.zxing.integration.android.IntentResult;

import androidx.appcompat.widget.PopupMenu;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.util.ArrayList;

public class MissionListActivity extends AppCompatActivity
{
    private static final String TAG = MissionListActivity.class.getSimpleName();

    private MissionDatabase _database;
    private MissionListAdapter _adapter;
    private Intent _resultIntent = new Intent();
    private String _activeMissionId;
    private String _activeMissionJson;
    //private String _missionFileLoadPassword = null;

    private String _generatePassphrase = null;
    private int _generateGroupCount = 0;
    private String _generateRp = null;
    private String _generateName = null;

    private class MissionListAdapter extends ArrayAdapter<DatabaseMission>
    {
        private Context _ctx;
        private int _resId;

        public MissionListAdapter(Context ctx, int resId, ArrayList<DatabaseMission> list)
        {
            super(ctx, resId, list);
            _ctx = ctx;
            _resId = resId;
        }

        @NonNull
        @Override
        public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent)
        {
            LayoutInflater inflator = LayoutInflater.from(_ctx);
            convertView = inflator.inflate(_resId, parent, false);

            final DatabaseMission item = getItem(position);

            if(!Utils.isEmptyString(item._name))
            {
                ((TextView)convertView.findViewById(R.id.tvMissionName)).setText(item._name);
            }
            else
            {
                ((TextView)convertView.findViewById(R.id.tvMissionName)).setText(R.string.no_mission_name);
            }

            if(!Utils.isEmptyString(item._description))
            {
                ((TextView)convertView.findViewById(R.id.tvDescription)).setText(item._description);
            }
            else
            {
                ((TextView)convertView.findViewById(R.id.tvDescription)).setText(R.string.no_mission_description);
            }

            int groupCount;

            if(item._groups == null || item._groups.size() == 0)
            {
                groupCount = 0;
            }
            else
            {
                groupCount = item._groups.size();
            }

            ((TextView)convertView.findViewById(R.id.tvGroupCount)).setText(Integer.toString(groupCount));

            convertView.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    editMission(item._id);
                }
            });

            if(_activeMissionId.compareTo(item._id) == 0)
            {
                convertView.findViewById(R.id.ivDeleteMission).setVisibility(View.INVISIBLE);
            }
            else
            {
                convertView.findViewById(R.id.ivDeleteMission).setOnClickListener(new View.OnClickListener()
                {
                    @Override
                    public void onClick(View v)
                    {
                        confirmDeleteMission(item._id);
                    }
                });

                if (_activeMissionId.compareTo(item._id) == 0)
                {
                    ((ImageView) convertView.findViewById(R.id.ivActiveMissionIndicator))
                            .setImageDrawable(ContextCompat.getDrawable(MissionListActivity.this,
                                    R.drawable.ic_radio_button_selected));
                }
                else
                {
                    ((ImageView) convertView.findViewById(R.id.ivActiveMissionIndicator))
                            .setImageDrawable(ContextCompat.getDrawable(MissionListActivity.this,
                                    R.drawable.ic_radio_button_unselected));
                }
            }

            convertView.findViewById(R.id.ivShareMission).setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    shareMission(item._id);
                }
            });

            convertView.findViewById(R.id.ivActiveMissionIndicator).setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    attemptToActivateMission(item._id);
                }
            });

            return convertView;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_mission_list);

        FloatingActionButton fabAdd = findViewById(R.id.fabAdd);
        fabAdd.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                promptToAddMission();
            }
        });

        _activeMissionId = Globals.getEngageApplication().getActiveConfiguration().getMissionId();

        _database = MissionDatabase.load(Globals.getSharedPreferences(), Constants.MISSION_DATABASE_NAME);
        if(_database == null)
        {
            _database = new MissionDatabase();
        }

        // Grab the active mission's JSON - we'll use it later
        DatabaseMission activeDatabaseMission = _database.getMissionById(_activeMissionId);
        if(activeDatabaseMission != null)
        {
            _activeMissionJson = activeDatabaseMission.toString();
        }

        _adapter = new MissionListAdapter(this, R.layout.mission_list_entry, _database._missions);
        ListView lv = findViewById(R.id.lvMissions);
        lv.setAdapter(_adapter);

        setupActionBar();
    }

    private void setupActionBar()
    {
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null)
        {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        int id = item.getItemId();
        if (id == android.R.id.home)
        {
            onBackPressed();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void attemptToActivateMission(String id)
    {
        _resultIntent.putExtra(Constants.MISSION_ACTIVATED_ID, id);
        setResult(RESULT_OK, _resultIntent);
        finish();
    }

    private void doMissionGeneration(String certStoreFn, String passwordHexString)
    {
        String json = null;

        if(Utils.isEmptyString(certStoreFn))
        {
            json = Globals.getEngageApplication()
                    .applyFlavorSpecificGeneratedMissionModifications(Globals.getEngageApplication()
                            .getEngine().engageGenerateMission(_generatePassphrase,
                                                               _generateGroupCount,
                                                               _generateRp,
                                                               _generateName), false);

            if(Utils.isEmptyString(json))
            {
                Utils.showErrorMsg(this, getString(R.string.failed_to_generate_the_mission));
                return;
            }
        }
        else
        {
            json = Globals.getEngageApplication()
                    .applyFlavorSpecificGeneratedMissionModifications(Globals.getEngageApplication()
                            .getEngine().engageGenerateMissionUsingCertStore(_generatePassphrase,
                                                                             _generateGroupCount,
                                                                             _generateRp,
                                                                             _generateName,
                                                                             certStoreFn,
                                                                             passwordHexString,
                                                                             ""), false);//NON-NLS

            if(Utils.isEmptyString(json))
            {
                Utils.showErrorMsg(this, getString(R.string.failed_to_generate_the_mission_using_the_selected_certstore));
                return;
            }
        }

        if(!ActiveConfiguration.doesMissionExistInDatabase(json))
        {
            if(!ActiveConfiguration.installMissionJson(MissionListActivity.this, json, false))
            {
                Utils.showErrorMsg(this, getString(R.string.failed_to_install_the_generated_mission));
                return;
            }

            // Force a recreate to reload the database
            recreate();
        }
        else
        {
            Toast.makeText(MissionListActivity.this, R.string.action_would_create_duplicate_mission, Toast.LENGTH_LONG).show();
        }
    }

    private AlertDialog _activeCertStoreListDialog = null;
    private class CertStoreListAdapter extends ArrayAdapter<EngageCertStore>
    {
        private Context _ctx;
        private int _resId;

        public CertStoreListAdapter(Context ctx, int resId, ArrayList<EngageCertStore> list)
        {
            super(ctx, resId, list);
            _ctx = ctx;
            _resId = resId;
        }

        @NonNull
        @Override
        public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent)
        {
            LayoutInflater inflator = LayoutInflater.from(_ctx);
            convertView = inflator.inflate(_resId, parent, false);

            final EngageCertStore item = getItem(position);

            ((TextView)convertView.findViewById(R.id.tvCertStoreName)).setText(item.getDisplayName());
            ((TextView)convertView.findViewById(R.id.tvDescription)).setText(item.getDescription());

            convertView.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    _activeCertStoreListDialog.dismiss();
                    doMissionGeneration(item.getFileName(), item.getPasswordhexString());
                }
            });

            return convertView;
        }
    }

    private void promptForCertStoreToUseForMissionGeneration(ArrayList<EngageCertStore> certStoresList)
    {
        if(!certStoresList.isEmpty())
        {
            final CertStoreListAdapter arrayAdapter = new CertStoreListAdapter(this, R.layout.certstore_list_row_item, certStoresList);
            final AlertDialog.Builder builder = new AlertDialog.Builder(this);

            View content = getLayoutInflater().inflate(R.layout.list_dialog_with_message, null);

            TextView tvMessage = content.findViewById(R.id.tvMessage);
            tvMessage.setText("Please select a certificate store to use to generate the mission");

            final ListView lvItems = content.findViewById(R.id.lvItems);
            lvItems.setAdapter(arrayAdapter);
            lvItems.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
            builder.setView(content);
            builder.setCancelable(false);
            builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener()
            {
                @Override
                public void onClick(DialogInterface dialog, int which)
                {
                    dialog.cancel();
                }
            });
            _activeCertStoreListDialog = builder.show();
        }
    }

    private void promptToGenerateMission()
    {
        LayoutInflater layoutInflater = LayoutInflater.from(this);
        View promptView = layoutInflater.inflate(R.layout.generate_mission_parameters_dialog, null);
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);
        alertDialogBuilder.setView(promptView);

        final EditText etPassphrase = promptView.findViewById(R.id.etPassphrase);
        final Spinner spnGroupCount = promptView.findViewById(R.id.spnGroupCount);
        final EditText etRallypoint = promptView.findViewById(R.id.etRallypoint);
        final EditText etName = promptView.findViewById(R.id.etName);

        alertDialogBuilder.setCancelable(false)
                .setPositiveButton(R.string.generate_mission_button, new DialogInterface.OnClickListener()
                {
                    public void onClick(DialogInterface dialog, int id)
                    {
                        _generatePassphrase = etPassphrase.getText().toString();
                        _generateGroupCount = Integer.parseInt(spnGroupCount.getSelectedItem().toString());
                        _generateRp = etRallypoint.getText().toString();
                        _generateName = etName.getText().toString();
                        
                        if(!Utils.isEmptyString(_generatePassphrase))
                        {
                            ArrayList<EngageCertStore> certStores = getCertStoreList();
                            if(certStores == null || certStores.isEmpty())
                            {
                                doMissionGeneration(null, null);
                            }
                            else
                            {
                                promptForCertStoreToUseForMissionGeneration(certStores);
                            }
                        }
                        else
                        {
                            Toast.makeText(MissionListActivity.this, R.string.a_passphrase_is_required_to_generate, Toast.LENGTH_SHORT).show();
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

    private void promptToAddMission()
    {
        PopupMenu popup = new PopupMenu(this, findViewById(R.id.fabAdd));

        MenuInflater inflater = popup.getMenuInflater();
        inflater.inflate(R.menu.mission_list_activity_menu, popup.getMenu());

        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener()
        {
            @Override
            public boolean onMenuItemClick(MenuItem item)
            {
                final int menuId = item.getItemId();

                if ( (menuId == R.id.action_mission_add_scan) ||
                     (menuId == R.id.action_mission_add_load_file) )
                {
                    // Clear any left-over password
                    Utils.setInboundMissionPassword(null);

                    LayoutInflater layoutInflater = LayoutInflater.from(MissionListActivity.this);
                    View promptView = layoutInflater.inflate(R.layout.mission_load_password_dialog, null);
                    AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(MissionListActivity.this);
                    alertDialogBuilder.setView(promptView);

                    final EditText editText = promptView.findViewById(R.id.etPassword);

                    alertDialogBuilder.setCancelable(false)
                            .setPositiveButton(R.string.mission_load_continue_button, new DialogInterface.OnClickListener()
                            {
                                public void onClick(DialogInterface dialog, int id)
                                {
                                    // Save the password for later
                                    Utils.setInboundMissionPassword(editText.getText().toString());

                                    if(menuId == R.id.action_mission_add_scan)
                                    {
                                        Globals.getEngageApplication().initiateScanOfAQrCode(MissionListActivity.this, getString(R.string.scan_qr_code));
                                    }
                                    else
                                    {
                                        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
                                        intent.setType("*/*");
                                        intent.addCategory(Intent.CATEGORY_OPENABLE);
                                        startActivityForResult(Intent.createChooser(intent, getString(R.string.select_a_file)), Constants.PICK_MISSION_FILE_REQUEST_CODE);
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

                    return true;
                }
                else
                {
                    if (menuId == R.id.action_mission_add_manual)
                    {
                        Intent intent = new Intent(MissionListActivity.this, MissionEditActivity.class);
                        startActivityForResult(intent, Constants.EDIT_ACTION_REQUEST_CODE);
                        return true;
                    }
                    else if (menuId == R.id.action_mission_add_generate)
                    {
                        promptToGenerateMission();
                        return true;
                    }
                }

                /*
                else if (id == R.id.action_mission_add_load_file)
                {
                    _missionFileLoadPassword = null;

                    LayoutInflater layoutInflater = LayoutInflater.from(MissionListActivity.this);
                    View promptView = layoutInflater.inflate(R.layout.file_load_mission_password_dialog, null);
                    AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(MissionListActivity.this);
                    alertDialogBuilder.setView(promptView);

                    final EditText etPassword = promptView.findViewById(R.id.etPassword);

                    alertDialogBuilder.setCancelable(false)
                            .setPositiveButton(R.string.mission_file_load_button, new DialogInterface.OnClickListener()
                            {
                                public void onClick(DialogInterface dialog, int id)
                                {
                                    try
                                    {
                                        // Save this for later
                                        _missionFileLoadPassword = etPassword.getText().toString();

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

                    return true;
                }
                */
                /*
                else if (id == R.id.action_mission_add_download)
                {
                    LayoutInflater layoutInflater = LayoutInflater.from(MissionListActivity.this);
                    View promptView = layoutInflater.inflate(R.layout.download_mission_url_dialog, null);
                    AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(MissionListActivity.this);
                    alertDialogBuilder.setView(promptView);

                    final EditText etPassword = promptView.findViewById(R.id.etPassword);
                    final EditText etUrl = promptView.findViewById(R.id.etUrl);

                    //etUrl.setText("https://s3.us-east-2.amazonaws.com/rts-missions/{a50f4cfb-f200-4cf0-8f88-0401585ba034}.json");

                    alertDialogBuilder.setCancelable(false)
                            .setPositiveButton(R.string.mission_download_button, new DialogInterface.OnClickListener()
                            {
                                public void onClick(DialogInterface dialog, int id)
                                {
                                    downloadAndSaveMission(etUrl.getText().toString(), etPassword.getText().toString());
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

                    return true;
                }
                */
                /*
                else if (id == R.id.action_mission_add_oem)
                {
                    Intent intent = new Intent(MissionListActivity.this, OemMissionEditActivity.class);
                    startActivityForResult(intent, Constants.EDIT_ACTION_REQUEST_CODE);
                    return true;
                }
                */

                return false;
            }
        });

        popup.show();
    }

    /*
    private void downloadAndSaveMission(final String url, final String password)
    {
        Handler handler = new Handler() {
            @Override
            public void handleMessage(Message msg)
            {
                try
                {
                    String resultMsg = msg.getData().getString(DownloadMissionTask.BUNDLE_RESULT_MSG);
                    int responseCode = msg.arg1;

                    if(responseCode >= 200 && responseCode <= 299)
                    {
                        byte[] missionData = msg.getData().getString(DownloadMissionTask.BUNDLE_RESULT_DATA).getBytes(Utils.getEngageCharSet());
                        String json;

                        if (!Utils.isEmptyString(password))
                        {
                            String pwdHexString = Utils.toHexString(_missionFileLoadPassword.getBytes(Utils.getEngageCharSet()));
                            byte[] decryptedBytes = Globals.getEngageApplication().getEngine().decryptSimple(missionData, pwdHexString);
                            json = new String(decryptedBytes, Utils.getEngageCharSet());
                        }
                        else
                        {
                            json = new String(missionData, Utils.getEngageCharSet());
                        }

                        try
                        {
                            JSONObject verifier = null;

                            try
                            {
                                verifier = new JSONObject(json);
                            }
                            catch(Exception eInner)
                            {
                                verifier = null;
                            }

                            if(verifier != null)
                            {
                                if(ActiveConfiguration.installMissionJson(MissionListActivity.this, json, true))
                                {
                                    // Force a recreate to reload the database
                                    recreate();
                                }
                                else
                                {
                                    throw new Exception(getString(R.string.invalid_mission_data));
                                }
                            }
                            else
                            {
                                throw new Exception(getString(R.string.invalid_mission_data));
                            }
                        }
                        catch (Exception e)
                        {
                            Toast.makeText(MissionListActivity.this, e.getMessage(), Toast.LENGTH_LONG).show();
                        }
                    }
                    else
                    {
                        Toast.makeText(MissionListActivity.this, "Mission download failed - " + resultMsg, Toast.LENGTH_LONG).show();
                    }
                }
                catch (Exception e)
                {
                    Toast.makeText(MissionListActivity.this, "Mission download failed with exception " + e.getMessage(), Toast.LENGTH_LONG).show();
                }
            }
        };

        DownloadMissionTask dmt = new DownloadMissionTask(handler);
        dmt.execute(url);
    }
    */

    private void promptToAddMission_original()
    {
        final TextView message = new TextView(this);

        message.setText(R.string.you_can_add_a_mission_manually_or_generate);
        message.setMovementMethod(LinkMovementMethod.getInstance());
        message.setPadding(32, 32, 32, 32);

        AlertDialog dlg = new AlertDialog.Builder(this)
                .setTitle(getString(R.string.add_mission))
                .setCancelable(false)
                .setPositiveButton(getString(R.string.button_manual), new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                        Intent intent = new Intent(MissionListActivity.this, MissionEditActivity.class);
                        startActivityForResult(intent, Constants.EDIT_ACTION_REQUEST_CODE);
                    }
                }).setNegativeButton(getString(R.string.button_generate), new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                        promptToGenerateMission();
                    }
                }).setNeutralButton(getString(R.string.button_cancel), new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                    }
                }).setView(message).create();

        dlg.show();
    }

    private void editMission(String id)
    {
        DatabaseMission mission = _database.getMissionById(id);
        Intent intent = new Intent(this, MissionEditActivity.class);
        intent.putExtra(Constants.MISSION_EDIT_EXTRA_JSON, mission.toJson().toString());
        startActivityForResult(intent, Constants.EDIT_ACTION_REQUEST_CODE);
    }

    private void processIncomingMissionData(byte[] data, String pwd, boolean isScannedQrCode)
    {
        ActiveConfiguration ac = null;
        Exception errorException = null;

        try
        {
            if(isScannedQrCode)
            {
                String scannedString = new String(data, Utils.getEngageCharSet());
                ac = ActiveConfiguration.parseEncryptedQrCodeString(scannedString, pwd);
                if(ac == null)
                {
                    throw new Exception(getString(R.string.cannot_parse_mission_data));
                }
            }
            else
            {
                Bitmap bm = BitmapFactory.decodeByteArray(data, 0, data.length);
                if(bm != null)
                {
                    byte[] missionData = null;

                    missionData = Utils.qrCodeBitmapToString(bm).getBytes(Utils.getEngageCharSet());
                    if(missionData == null || missionData.length < 1)
                    {
                        throw new Exception(getString(R.string.image_is_not_a_qr_code));
                    }

                    String stringData = new String(missionData, Utils.getEngageCharSet());

                    // Look for the "/??" to see if there's a deflection URL
                    int endOfDeflection = stringData.indexOf(Constants.QR_DEFLECTION_URL_SEP);

                    // If it's there, strip it off
                    if (endOfDeflection > 0)
                    {
                        stringData = stringData.substring(endOfDeflection + Constants.QR_DEFLECTION_URL_SEP.length());
                    }

                    // Now we have a string with is Base91 encoded, we need to decode that
                    byte[] base91DecodedBytes = Base91.decode(stringData.getBytes(Utils.getEngageCharSet()));
                    if (base91DecodedBytes == null || base91DecodedBytes.length < 1)
                    {
                        throw new Exception(getString(R.string.image_is_not_base91_encoded));
                    }

                    // It may be encrypted, so decrypt if we have a password
                    if (!Utils.isEmptyString(pwd))
                    {
                        String pwdHexString = Utils.toHexString(pwd.getBytes(Utils.getEngageCharSet()));

                        base91DecodedBytes = Globals.getEngageApplication().getEngine().decryptSimple(base91DecodedBytes, pwdHexString);
                        if (base91DecodedBytes == null)
                        {
                            throw new Exception(getString(R.string.encrypted_data_cannot_be_decrypted));
                        }
                    }

                    // Next, we decompress the data
                    byte[] decompressed = Utils.inflate(base91DecodedBytes);
                    if (decompressed == null || decompressed.length < 1)
                    {
                        throw new Exception(getString(R.string.image_compression_not_supported));
                    }

                    String qrCodeDataString = new String(decompressed, Utils.getEngageCharSet());
                    qrCodeDataString = qrCodeDataString.substring(Constants.QR_CODE_HEADER.length());

                    // Now, check the version - its "nnn"
                    int checkVersion = Integer.parseInt(Constants.QR_VERSION);
                    int qrVersion = Integer.parseInt(qrCodeDataString.substring(0, 3));
                    if (qrVersion == checkVersion)
                    {
                        // Strip the version
                        qrCodeDataString = qrCodeDataString.substring(3);

                        ac = new ActiveConfiguration();
                        if (!ac.parseTemplate(qrCodeDataString))
                        {
                            throw new Exception(getString(R.string.cannot_parse_mission_data));
                        }
                    }
                    else
                    {
                        throw new Exception(getString(R.string.image_version_is_not_supported));
                    }
                }
                else
                {
                    String stringData;

                    // It may be encrypted, so decrypt if we have a password
                    if (!Utils.isEmptyString(pwd))
                    {
                        String pwdHexString = Utils.toHexString(pwd.getBytes(Utils.getEngageCharSet()));

                        byte[] decryptedBytes = Globals.getEngageApplication().getEngine().decryptSimple(data, pwdHexString);
                        if (decryptedBytes == null)
                        {
                            throw new Exception(getString(R.string.encrypted_data_cannot_be_decrypted));
                        }

                        stringData = new String(decryptedBytes, Utils.getEngageCharSet());
                    }
                    else
                    {
                        stringData = new String(data, Utils.getEngageCharSet());
                    }

                    ac = new ActiveConfiguration();
                    if (!ac.parseTemplate(stringData))
                    {
                        throw new Exception(getString(R.string.cannot_parse_mission_data));
                    }
                }
            }
        }
        catch(Exception e)
        {
            ac = null;
            errorException = e;
        }

        if(ac != null)
        {
            try
            {
                String json = ac.getInputJson();
                if(Utils.isEmptyString(json))
                {
                    throw new Exception(getString(R.string.invalid_mission_data));
                }

                if(ActiveConfiguration.installMissionJson(MissionListActivity.this, json, true))
                {
                    // See if what was changed was the active mission, if so, we need to
                    // make sure our resultIntent is set correctly
                    if(ac.getMissionId().compareTo(_activeMissionId) == 0)
                    {
                        if(json.compareTo(_activeMissionJson) != 0)
                        {
                            _resultIntent.putExtra(Constants.MISSION_ACTIVATED_ID, _activeMissionId);
                            setResult(RESULT_OK, _resultIntent);
                        }
                    }

                    // Force a recreate to reload the database
                    recreate();
                }
                else
                {
                    throw new Exception(getString(R.string.invalid_mission_data));
                }
            }
            catch (Exception e)
            {
                errorException = e;
            }
        }

        if(errorException != null)
        {
            Utils.showShortPopupMsg(MissionListActivity.this, errorException.getMessage());
            errorException.printStackTrace();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent)
    {
        Log.d(TAG, "onActivityResult");//NON-NLS

        if(resultCode == RESULT_OK)
        {
            if (requestCode == Constants.EDIT_ACTION_REQUEST_CODE )
            {
                if(intent != null)
                {
                    installMissionJson(intent.getStringExtra(Constants.MISSION_EDIT_EXTRA_JSON));
                }
            }
            else if(requestCode == Globals.getEngageApplication().getQrCodeScannerRequestCode() ||
                    requestCode == Constants.PICK_MISSION_FILE_REQUEST_CODE ||
                    requestCode == Constants.PICK_QR_MISSION_FILE_REQUEST_CODE)
            {
                if(intent != null)
                {
                    boolean isScannedQrCode = (requestCode == Globals.getEngageApplication().getQrCodeScannerRequestCode());
                    try
                    {
                        // Get whatever password may be waiting
                        String pwd = Utils.getInboundMissionPassword();

                        Uri uri = intent.getData();
                        if(uri != null)
                        {
                            processIncomingMissionData(Utils.readBinaryFile(MissionListActivity.this, uri), pwd, isScannedQrCode);
                        }
                        else
                        {
                            IntentResult intentResult = IntentIntegrator.parseActivityResult(requestCode, resultCode, intent);
                            if(intentResult != null)
                            {
                                processIncomingMissionData(intentResult.getContents().getBytes(), pwd, isScannedQrCode);
                            }
                        }
                    }
                    catch (Exception e)
                    {
                        e.printStackTrace();
                    }
                }
            }
        }

        // Clear any waiting password
        Utils.setInboundMissionPassword(null);
    }

    private boolean installMissionJson(String json)
    {
        boolean rc = false;

        if (!Utils.isEmptyString(json))
        {
            DatabaseMission mission = DatabaseMission.parse(json);

            // Early return if we have nothing useful
            if( Utils.isEmptyString(mission._name) &&
                Utils.isEmptyString(mission._description) &&
                (mission._groups == null || mission._groups.isEmpty()) )
            {
                return false;
            }

            if(!_database.updateMissionById(mission._id, mission))
            {
                _database._missions.add(mission);
            }

            _database.save(Globals.getSharedPreferences(), Constants.MISSION_DATABASE_NAME);

            _adapter.notifyDataSetChanged();

            // See if what was changed was the active mission, if so, we need to
            // make sure our resultIntent is set correctly
            if(mission._id.compareTo(_activeMissionId) == 0)
            {
                String newJson = mission.toString();
                if(newJson.compareTo(_activeMissionJson) != 0)
                {
                    _resultIntent.putExtra(Constants.MISSION_ACTIVATED_ID, _activeMissionId);
                    setResult(RESULT_OK, _resultIntent);
                }
            }

            rc = true;
        }

        return rc;
    }

    private void shareMission(String id)
    {
        Globals.getEngageApplication().logEvent(Analytics.VIEW_SHARE_MISSION);
        Intent intent = new Intent(this, ShareMissionActivity.class);
        intent.putExtra(ShareMissionActivity.KEY_MISSION_ID, id);
        startActivity(intent);
    }

    private void confirmDeleteMission(final String id)
    {
        if(id.compareTo(_activeMissionId) == 0)
        {
            Toast.makeText(this, R.string.active_mission_cannot_be_deleted, Toast.LENGTH_SHORT).show();
        }
        else
        {
            DatabaseMission mission = _database.getMissionById(id);
            String s = String.format(getString(R.string.are_you_sure_you_want_to_delete_mission), mission._name);

            final TextView message = new TextView(this);
            final SpannableString ss = new SpannableString(s);

            message.setText(ss);
            message.setMovementMethod(LinkMovementMethod.getInstance());
            message.setPadding(32, 32, 32, 32);

            AlertDialog dlg = new AlertDialog.Builder(this)
                    .setTitle(getString(R.string.delete_mission))
                    .setCancelable(false)
                    .setPositiveButton(getString(R.string.button_yes), new DialogInterface.OnClickListener()
                    {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i)
                        {
                            deleteMission(id);
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
    }

    private void deleteMission(String id)
    {
        if(_database.deleteMissionById(id))
        {
            _adapter.notifyDataSetChanged();
        }

        _database.save(PreferenceManager.getDefaultSharedPreferences(this), Constants.MISSION_DATABASE_NAME);
    }

    private ArrayList<EngageCertStore> getCertStoreList()
    {
        ArrayList<EngageCertStore> rc = new ArrayList<>();

        try
        {
            File dir = new File(Globals.getEngageApplication().getCertStoreCacheDir());
            File[] allContents = dir.listFiles();
            if (allContents != null)
            {
                for (File file : allContents)
                {
                    EngageCertStore cs = EngageCertStore.loadStoreFrom(file.getAbsolutePath());
                    if(cs != null)
                    {
                        rc.add(cs);
                    }
                }
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

        return rc;
    }
}
