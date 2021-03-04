//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Intent;
import android.os.Bundle;
import android.view.MenuItem;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

public class OemMissionEditActivity extends AppCompatActivity
{
    private static String TAG = OemMissionEditActivity.class.getSimpleName();

    private DatabaseMission _mission = null;
    private Intent _resultIntent = new Intent();

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
            _mission._rpPort = Utils.intOpt(getString(R.string.default_rallypoint_port), Constants.DEF_RP_PORT);
            _mission._mcId = Utils.generateGroupId();
            _mission._mcCryptoPassword = Utils.generateCryptoPassword();
        }

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

    private boolean grabUiElements()
    {
        return true;
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
