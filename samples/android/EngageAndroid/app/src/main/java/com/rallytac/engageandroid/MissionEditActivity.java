//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;

public class MissionEditActivity extends AppCompatActivity
{
    private static String TAG = MissionEditActivity.class.getSimpleName();

    private DatabaseMission _mission = null;
    private GroupListAdapter _adapter;
    private Intent _resultIntent = new Intent();
    private boolean _allowEdit = true;

    private class GroupListAdapter extends ArrayAdapter<DatabaseGroup>
    {
        private Context _ctx;
        private int _resId;

        public GroupListAdapter(Context ctx, int resId, ArrayList<DatabaseGroup> list)
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

            final DatabaseGroup item = getItem(position);

            if(!Utils.isEmptyString(item._name))
            {
                ((TextView)convertView.findViewById(R.id.tvGroupName)).setText(item._name);
            }
            else
            {
                ((TextView)convertView.findViewById(R.id.tvGroupName)).setText(R.string.no_group_name);
            }

            String summary = null;

            String rxAddr = item._rxAddress;
            String rxPort = Integer.toString(item._rxPort);
            String txAddr = item._txAddress;
            String txPort = Integer.toString(item._txPort);
            String cryptoPassword = item._cryptoPassword;
            boolean encrypted = (cryptoPassword != null && !cryptoPassword.isEmpty() && item._useCrypto);

            int index = 0;
            for(String s : getResources().getStringArray(R.array.lp_tx_encoder_values))
            {
                if(s.compareTo(Integer.toString(item._txCodecId)) == 0)
                {
                    summary = getResources().getStringArray(R.array.lp_tx_encoder_short_names)[index];
                    break;
                }

                index++;
            }

            if(Utils.isEmptyString(summary))
            {
                summary = getString(R.string.invalid_configuration);
            }

            if(encrypted)
            {
                ((ImageView)convertView.findViewById(R.id.ivGroupEncrypted)).setImageDrawable(ContextCompat.getDrawable(MissionEditActivity.this, R.drawable.ic_protected));
            }
            else
            {
                ((ImageView)convertView.findViewById(R.id.ivGroupEncrypted)).setImageDrawable(ContextCompat.getDrawable(MissionEditActivity.this, R.drawable.ic_unprotected));
            }

            ((TextView)convertView.findViewById(R.id.tvGroupInfo)).setText(summary);

            convertView.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    editGroup(item._id);
                }
            });

            if(_allowEdit)
            {
                convertView.findViewById(R.id.ivDeleteGroup).setOnClickListener(new View.OnClickListener()
                {
                    @Override
                    public void onClick(View v)
                    {
                        confirmDeleteGroup(item._id);
                    }
                });
            }

            return convertView;
        }
    }

    private void addGroup()
    {
        if(_mission._groups.size() < Globals.getEngageApplication().getMaxGroupsAllowed())
        {
            editGroup(null);
        }
        else
        {
            Utils.showLongPopupMsg(this, R.string.max_num_groups_reached);
        }
    }

    private void editGroup(final String theIdToEdit)
    {
        final DatabaseGroup group;

        if(Utils.isEmptyString(theIdToEdit))
        {
            group = new DatabaseGroup();
            group._id = Utils.generateGroupId();
            group._txFramingMs = Constants.DEFAULT_TX_FRAMING_MS;
            group._txCodecId = Constants.DEFAULT_ENCODER;
            group._maxTxSecs = Constants.DEFAULT_TX_SECS;
        }
        else
        {
            group = _mission.getGroupById(theIdToEdit);
        }

        LayoutInflater layoutInflater = LayoutInflater.from(this);
        View view = layoutInflater.inflate(R.layout.layout_group_edit, null);
        AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(this);
        dialogBuilder.setView(view);

        final ImageView ivToggleGroupIdVisibility = view.findViewById(R.id.ivToggleGroupIdVisibility);
        final EditText etGroupId = view.findViewById(R.id.etGroupId);
        final EditText etGroupName = view.findViewById(R.id.etGroupName);
        final EditText etRxAddress = view.findViewById(R.id.etRxAddress);
        final EditText etRxPort = view.findViewById(R.id.etRxPort);
        final EditText etTxAddress = view.findViewById(R.id.etTxAddress);
        final EditText etTxPort = view.findViewById(R.id.etTxPort);
        final Spinner spnCodec = view.findViewById(R.id.spnCodec);
        final Spinner spnFraming = view.findViewById(R.id.spnFraming);
        final Switch swEncrypted = view.findViewById(R.id.swEncrypted);
        final Button btnRegen = view.findViewById(R.id.btnRegen);
        final TextView tvCryptoSignature = view.findViewById(R.id.tvCryptoSignature);
        final Switch swDisableHeaderExtensions = view.findViewById(R.id.swDisableHeaderExtensions);
        final Switch swFullDuplex = view.findViewById(R.id.swFullDuplex);
        final Switch swUnlimitedTx = view.findViewById(R.id.swUnlimitedTx);
        final Switch swEpt = view.findViewById(R.id.swEpt);
        final Switch swAnonymousAlias = view.findViewById(R.id.swAnonymousAlias);

        ivToggleGroupIdVisibility.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                int vs = etGroupId.getVisibility();
                if(vs == View.GONE)
                {
                    etGroupId.setVisibility(View.VISIBLE);
                }
                else
                {
                    etGroupId.setVisibility(View.GONE);
                }
            }
        });

        if(!Globals.getContext().getResources().getBoolean(R.bool.opt_supports_fdx))
        {
            swFullDuplex.setVisibility(View.GONE);
        }

        if(!Globals.getContext().getResources().getBoolean(R.bool.opt_supports_ept))
        {
            swEpt.setVisibility(View.GONE);
        }

        if(!Globals.getContext().getResources().getBoolean(R.bool.opt_supports_anonymous_alias))
        {
            swAnonymousAlias.setVisibility(View.GONE);
        }

        if(!Globals.getContext().getResources().getBoolean(R.bool.opt_supports_unlimited_tx))
        {
            swUnlimitedTx.setVisibility(View.GONE);
        }

        dialogBuilder.setCancelable(true)
                .setTitle(getString(R.string.title_group))
                .setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        dialog.cancel();
                    }
                });

        if(_allowEdit)
        {
            dialogBuilder.setPositiveButton(R.string.button_save, new DialogInterface.OnClickListener()
            {
                @Override
                public void onClick(DialogInterface dialog, int which)
                {
                    group._id = etGroupId.getText().toString();
                    group._name = etGroupName.getText().toString();
                    group._rxAddress = etRxAddress.getText().toString();
                    group._rxPort = Utils.parseIntSafe(etRxPort.getText().toString());
                    group._txAddress = etTxAddress.getText().toString();
                    group._txPort = Utils.parseIntSafe(etTxPort.getText().toString());
                    group._txCodecId = Utils.parseIntSafe(getResources().getStringArray(R.array.lp_tx_encoder_values)[spnCodec.getSelectedItemPosition()]);
                    group._txFramingMs = Utils.parseIntSafe(getResources().getStringArray(R.array.lp_tx_encoder_framing_values)[spnFraming.getSelectedItemPosition()]);
                    group._noHdrExt = swDisableHeaderExtensions.isChecked();
                    group._fdx = swFullDuplex.isChecked();
                    group._ept = (swEpt.isChecked() ? Constants.EPT_STATIC_VALUE : 0);
                    group._maxTxSecs = (swUnlimitedTx.isChecked() ? Constants.UNLIMITED_TX_SECS : Constants.DEFAULT_TX_SECS);
                    group._anonymousAlias = (swAnonymousAlias.isChecked());

                    if(Utils.isEmptyString(group._txAddress))
                    {
                        group._txAddress = group._rxAddress;
                    }

                    if(group._txPort == 0)
                    {
                        group._txPort = group._rxPort;
                    }

                    if(!_mission.updateGroupById(group._id, group))
                    {
                        _mission._groups.add(group);
                    }

                    _adapter.notifyDataSetChanged();
                }
            });
        }

        AlertDialog alertDialog = dialogBuilder.create();

        etGroupId.setText(group._id);
        etGroupName.setText(group._name);

        etRxAddress.setText(group._rxAddress);
        etRxPort.setText(Integer.toString(group._rxPort));

        etTxAddress.setText(group._txAddress);
        etTxPort.setText(Integer.toString(group._txPort));

        String[] ra;
        int index;

        index = 0;
        spnCodec.setSelection(index);
        ra = getResources().getStringArray(R.array.lp_tx_encoder_values);
        for(String s : ra)
        {
            if(s.compareTo(Integer.toString(group._txCodecId)) == 0)
            {
                spnCodec.setSelection(index);
                break;
            }

            index++;
        }

        index = 0;
        spnFraming.setSelection(index);
        ra = getResources().getStringArray(R.array.lp_tx_encoder_framing_values);
        for(String s : ra)
        {
            if(s.compareTo(Integer.toString(group._txFramingMs)) == 0)
            {
                spnFraming.setSelection(index);
                break;
            }

            index++;
        }

        swEncrypted.setChecked(group._useCrypto);
        swEncrypted.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                group._useCrypto = swEncrypted.isChecked();

                if(group._useCrypto && Utils.isEmptyString(group._cryptoPassword))
                {
                    group._cryptoPassword = Utils.generateCryptoPassword();
                    tvCryptoSignature.setText(Utils.md5HashOfString(group._cryptoPassword));
                }

                btnRegen.setEnabled(group._useCrypto);
            }
        });

        btnRegen.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                group._cryptoPassword = Utils.generateCryptoPassword();
                tvCryptoSignature.setText(Utils.md5HashOfString(group._cryptoPassword));
            }
        });

        if(!Utils.isEmptyString(group._cryptoPassword))
        {
            tvCryptoSignature.setText(Utils.md5HashOfString(group._cryptoPassword));
        }
        else
        {
            tvCryptoSignature.setText(null);
        }

        swDisableHeaderExtensions.setChecked(group._noHdrExt);
        swFullDuplex.setChecked(group._fdx);
        swEpt.setChecked(group._ept > 0);
        swAnonymousAlias.setChecked(group._anonymousAlias);
        swUnlimitedTx.setChecked(group._maxTxSecs == Constants.UNLIMITED_TX_SECS);

        etGroupId.setEnabled(_allowEdit);
        etGroupName.setEnabled(_allowEdit);
        etRxAddress.setEnabled(_allowEdit);
        etRxPort.setEnabled(_allowEdit);
        etTxAddress.setEnabled(_allowEdit);
        etTxPort.setEnabled(_allowEdit);
        spnCodec.setEnabled(_allowEdit);
        spnFraming.setEnabled(_allowEdit);
        swEncrypted.setEnabled(_allowEdit);
        btnRegen.setEnabled(_allowEdit && group._useCrypto);
        swDisableHeaderExtensions.setEnabled(_allowEdit);

        if(Globals.getContext().getResources().getBoolean(R.bool.opt_supports_fdx))
        {
            swFullDuplex.setEnabled(_allowEdit);
        }

        if(Globals.getContext().getResources().getBoolean(R.bool.opt_supports_ept))
        {
            swEpt.setEnabled(_allowEdit);
        }

        if(Globals.getContext().getResources().getBoolean(R.bool.opt_supports_anonymous_alias))
        {
            swAnonymousAlias.setEnabled(_allowEdit);
        }

        if(Globals.getContext().getResources().getBoolean(R.bool.opt_supports_unlimited_tx))
        {
            swUnlimitedTx.setEnabled(_allowEdit);
        }

        alertDialog.show();
    }

    private void confirmDeleteGroup(final String id)
    {
        DatabaseGroup group = _mission.getGroupById(id);
        String s;

        s = String.format(getString(R.string.are_you_sure_delete_group), group._name);

        final TextView message = new TextView(this);
        final SpannableString ss = new SpannableString(s);

        message.setText(ss);
        message.setMovementMethod(LinkMovementMethod.getInstance());
        message.setPadding(32, 32, 32, 32);

        AlertDialog dlg = new AlertDialog.Builder(this)
                .setTitle(getString(R.string.title_delete_group))
                .setCancelable(false)
                .setPositiveButton(R.string.button_yes, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                        deleteGroup(id);
                    }
                }).setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                    }
                }).setView(message).create();

        dlg.show();
    }

    private void deleteGroup(String id)
    {
        if(_mission.deleteGroupById(id))
        {
            _adapter.notifyDataSetChanged();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_mission_edit);

        Intent intent = getIntent();
        if(intent != null)
        {
            String json = intent.getStringExtra(Constants.MISSION_EDIT_EXTRA_JSON);
            if(json != null)
            {
                _mission = DatabaseMission.parse(json);
            }
        }

        if(_mission == null)
        {
            _mission = new DatabaseMission();
            _mission._id = Utils.generateMissionId();
            _mission._rpAddress = getString(R.string.default_rallypoint);
            _mission._rpPort = Globals.getContext().getResources().getInteger(R.integer.opt_default_rallypoint_port);
            _mission._mcId = Utils.generateGroupId();
            _mission._mcCryptoPassword = Utils.generateCryptoPassword();

            toggleAdvancedView();
        }

        _allowEdit = Utils.isEmptyString(_mission._modPin);

        populateUi();

        _adapter = new GroupListAdapter(this, R.layout.group_list_entry, _mission._groups);
        ListView lv = findViewById(R.id.lvGroups);
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

    private void populateUi()
    {
        if(_mission != null)
        {
            if(_allowEdit)
            {
                ((ImageView) findViewById(R.id.ivPinLock)).setImageDrawable(ContextCompat.getDrawable(this, R.drawable.ic_unlocked));
            }
            else
            {
                ((ImageView) findViewById(R.id.ivPinLock)).setImageDrawable(ContextCompat.getDrawable(this, R.drawable.ic_locked));
            }

            ((TextView)findViewById(R.id.etMissionName)).setText(_mission._name);
            findViewById(R.id.etMissionName).setEnabled(_allowEdit);

            ((TextView)findViewById(R.id.etMissionDescription)).setText(_mission._description);
            findViewById(R.id.etMissionDescription).setEnabled(_allowEdit);

            findViewById(R.id.swUseRallypoint).setEnabled(_allowEdit);
            ((Switch)findViewById(R.id.swUseRallypoint)).setChecked(_mission._useRp);

            ((TextView)findViewById(R.id.etRpAddress)).setText(_mission._rpAddress);
            findViewById(R.id.etRpAddress).setEnabled(_allowEdit);

            ((TextView)findViewById(R.id.etRpPort)).setText(Integer.toString(_mission._rpPort));
            findViewById(R.id.etRpPort).setEnabled(_allowEdit);

            ((TextView)findViewById(R.id.etMcAddress)).setText(_mission._mcAddress);
            findViewById(R.id.etMcAddress).setEnabled(_allowEdit);

            ((TextView)findViewById(R.id.etMcPort)).setText(Integer.toString(_mission._mcPort));
            findViewById(R.id.etMcPort).setEnabled(_allowEdit);

            ((Spinner)findViewById(R.id.spnMcFailoverPolicy)).setSelection(_mission._multicastFailoverPolicy);
            findViewById(R.id.spnMcFailoverPolicy).setEnabled(_allowEdit);

            findViewById(R.id.ivAddGroup).setEnabled(_allowEdit);
        }
    }

    private boolean grabUiElements()
    {
        boolean rc;
        DatabaseMission tmp = _mission;

        try
        {
            tmp._name = Utils.trimString(((TextView) findViewById(R.id.etMissionName)).getText().toString());
            tmp._description = Utils.trimString(((TextView) findViewById(R.id.etMissionDescription)).getText().toString());
            tmp._useRp = ((Switch) findViewById(R.id.swUseRallypoint)).isChecked();
            tmp._rpAddress = Utils.trimString(((TextView) findViewById(R.id.etRpAddress)).getText().toString());
            tmp._rpPort = Utils.parseIntSafe(Utils.trimString(((TextView) findViewById(R.id.etRpPort)).getText().toString()));
            tmp._multicastFailoverPolicy = ((Spinner)findViewById(R.id.spnMcFailoverPolicy)).getSelectedItemPosition();

            tmp._mcAddress = Utils.trimString(((TextView) findViewById(R.id.etMcAddress)).getText().toString());
            tmp._mcPort = Utils.parseIntSafe(Utils.trimString(((TextView) findViewById(R.id.etMcPort)).getText().toString()));

            _mission = tmp;

            rc = true;
        }
        catch(Exception e)
        {
            // TODO display info about invalid data
            rc = false;
        }

        return rc;
    }

    public void onClickPinLock(View view)
    {
        final boolean locking = (Utils.isEmptyString(_mission._modPin) || (!Utils.isEmptyString(_mission._modPin) && _allowEdit));

        LayoutInflater layoutInflater = LayoutInflater.from(this);
        View promptView = layoutInflater.inflate(R.layout.pin_code_dialog, null);
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);
        alertDialogBuilder.setView(promptView);

        final EditText editText = promptView.findViewById(R.id.etPinCode);

        alertDialogBuilder.setNegativeButton(R.string.cancel,
                new DialogInterface.OnClickListener()
                {
                    public void onClick(DialogInterface dialog, int id)
                    {
                        dialog.cancel();
                    }
                });

        if(!Utils.isEmptyString(_mission._modPin))
        {
            alertDialogBuilder.setNeutralButton(getString(R.string.button_clear), new DialogInterface.OnClickListener()
            {
                @Override
                public void onClick(DialogInterface dialog, int which)
                {
                    String pin = editText.getText().toString();
                    if(!Utils.isEmptyString(pin))
                    {
                        if(pin.compareTo(_mission._modPin) == 0)
                        {
                            _allowEdit = true;
                            _mission._modPin = "";
                            populateUi();
                            _adapter.notifyDataSetChanged();
                        }
                    }
                    else
                    {
                        Toast.makeText(MissionEditActivity.this, R.string.no_pin_entered, Toast.LENGTH_SHORT).show();
                    }
                }
            });
        }

        alertDialogBuilder.setPositiveButton(locking ? getString(R.string.button_lock) : getString(R.string.button_unlock), new DialogInterface.OnClickListener()
                {
                    public void onClick(DialogInterface dialog, int id)
                    {
                        String pin = editText.getText().toString();

                        if(!Utils.isEmptyString(pin))
                        {
                            if(locking)
                            {
                                _mission._modPin = pin;
                                _allowEdit = false;
                            }
                            else
                            {
                                if(pin.compareTo(_mission._modPin) == 0)
                                {
                                    _allowEdit = true;
                                }
                                else
                                {
                                    Toast.makeText(MissionEditActivity.this, R.string.invalid_pin, Toast.LENGTH_SHORT).show();
                                }
                            }

                            populateUi();
                            _adapter.notifyDataSetChanged();
                        }
                        else
                        {
                            Toast.makeText(MissionEditActivity.this, R.string.no_pin_entered, Toast.LENGTH_SHORT).show();
                        }

                    }
                });



        AlertDialog alert = alertDialogBuilder.create();
        alert.show();
    }

    public void onClickToggleAdvanced(View view)
    {
        toggleAdvancedView();
    }

    private void toggleAdvancedView()
    {
        ConstraintLayout lay = findViewById(R.id.layAdvanced);

        if(lay.getVisibility() == View.VISIBLE)
        {
            lay.setVisibility(View.GONE);
        }
        else
        {
            lay.setVisibility(View.VISIBLE);
        }
    }

    public void onClickAddGroup(View view)
    {
        addGroup();
    }

    private void updateActivityResult()
    {
        if(grabUiElements())
        {
            String theJson = _mission.toString();
            _resultIntent.putExtra(Constants.MISSION_EDIT_EXTRA_JSON, theJson);
            setResult(RESULT_OK, _resultIntent);
        }
    }

    @Override
    public void onBackPressed()
    {
        updateActivityResult();
        super.onBackPressed();
    }
}
