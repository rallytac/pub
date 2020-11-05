//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.preference.PreferenceManager;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.view.LayoutInflater;
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

import java.util.ArrayList;

public class MissionListActivity extends AppCompatActivity
{
    private static String TAG = MissionListActivity.class.getSimpleName();

    private static int EDIT_ACTION_REQUEST_CODE = 42;

    private MissionDatabase _database;
    private MissionListAdapter _adapter;
    private Intent _resultIntent = new Intent();
    private String _activeMissionId;
    private String _activeMissionJson;

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

            convertView.findViewById(R.id.ivActiveMissionIndicator).setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    activateMission(item._id);
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

    private void activateMission(String id)
    {
        _resultIntent.putExtra(Constants.MISSION_ACTIVATED_ID, id);
        setResult(RESULT_OK, _resultIntent);
        finish();
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
                        String passphrase = etPassphrase.getText().toString();
                        int groupCount = Integer.parseInt(spnGroupCount.getSelectedItem().toString());
                        String rp = etRallypoint.getText().toString();
                        String name = etName.getText().toString();

                        if(!Utils.isEmptyString(passphrase))
                        {
                            String json;
                            json = Globals.getEngageApplication().applyFlavorSpecificGeneratedMissionModifications(Globals.getEngageApplication().getEngine().engageGenerateMission(passphrase, groupCount, rp, name));

                            if(!ActiveConfiguration.doesMissionExistInDatabase(json))
                            {
                                ActiveConfiguration.installMissionJson(MissionListActivity.this, json, false);

                                // Force a recreate to reload the database
                                recreate();
                            }
                            else
                            {
                                Toast.makeText(MissionListActivity.this, R.string.action_would_create_duplicate_mission, Toast.LENGTH_LONG).show();
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
                        startActivityForResult(intent, EDIT_ACTION_REQUEST_CODE);
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
        startActivityForResult(intent, EDIT_ACTION_REQUEST_CODE);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent)
    {
        Log.d(TAG, "onActivityResult");//NON-NLS

        if(resultCode == RESULT_OK)
        {
            if (requestCode == EDIT_ACTION_REQUEST_CODE )
            {
                if(intent != null)
                {
                    installMissionJson(intent.getStringExtra(Constants.MISSION_EDIT_EXTRA_JSON));
                }
            }
        }
    }

    private void installMissionJson(String json)
    {
        if (!Utils.isEmptyString(json))
        {
            DatabaseMission mission = DatabaseMission.parse(json);

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
        }
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
                    }).setNegativeButton(getString(R.string.button_cancel), new DialogInterface.OnClickListener()
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
}
