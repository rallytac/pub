//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;


import android.content.Intent;
import android.os.Bundle;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import android.view.MenuItem;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.github.mikephil.charting.charts.LineChart;
import com.rallytac.engageandroid.Charting.EntrySet;

public class UserNodeViewActivity extends
                                    AppCompatActivity
                                  implements
                                    EngageApplication.IPresenceChangeListener
{
    private static String TAG = UserNodeViewActivity.class.getSimpleName();

    public static String EXTRA_NODE_ID = "$NODEID";//NON-NLS

    private EngageApplication _app;
    private String _nodeId = null;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        _app = (EngageApplication) getApplication();

        setContentView(R.layout.activity_single_user_node_view);

        ActionBar ab = getSupportActionBar();
        if(ab != null)
        {
            ab.setDisplayHomeAsUpEnabled(true);
        }

        Intent intent = getIntent();
        if(intent != null)
        {
            _nodeId = intent.getStringExtra(EXTRA_NODE_ID);
        }

        if(Utils.isEmptyString(_nodeId))
        {
            Toast.makeText(this, "No node ID provided for this activity", Toast.LENGTH_LONG).show();//NON-NLS
            finish();
        }

        forceRebuild();
    }

    @Override
    public void onResume()
    {
        _app.addPresenceChangeListener(this);
        super.onResume();
    }

    @Override
    public void onPause()
    {
        _app.removePresenceChangeListener(this);
        super.onPause();
    }

    @Override
    public void onDestroy()
    {
        _app.removePresenceChangeListener(this);
        super.onDestroy();
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
    public void onPresenceAdded(PresenceDescriptor pd)
    {
        // We only care about this node
        if(pd.nodeId.compareTo(_nodeId) != 0)
        {
            return;
        }

        rebuildTheUiWhichIsVeryExpensiveAndShouldBeFixed(pd);
    }

    @Override
    public void onPresenceChange(PresenceDescriptor pd)
    {
        // We only care about this node
        if(pd.nodeId.compareTo(_nodeId) != 0)
        {
            return;
        }

        rebuildTheUiWhichIsVeryExpensiveAndShouldBeFixed(pd);
    }

    @Override
    public void onPresenceRemoved(PresenceDescriptor pd)
    {
        // We only care about this node
        if(pd.nodeId.compareTo(_nodeId) != 0)
        {
            return;
        }

        rebuildTheUiWhichIsVeryExpensiveAndShouldBeFixed(pd);
    }

    private void forceRebuild()
    {
        PresenceDescriptor pd = Globals.getEngageApplication().getPresenceDescriptor(_nodeId);
        if(pd == null)
        {
            Toast.makeText(this, "Could not find the node!", Toast.LENGTH_LONG).show();//NON-NLS
            finish();
        }

        rebuildTheUiWhichIsVeryExpensiveAndShouldBeFixed(pd);
    }

    private void rebuildTheUiWhichIsVeryExpensiveAndShouldBeFixed(final PresenceDescriptor pd)
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                ImageView iv;
                int imageId;

                if(!Utils.isEmptyString(pd.displayName))
                {
                    ((TextView)findViewById(R.id.tvDisplayName)).setText(pd.displayName);
                }
                else
                {
                    ((TextView)findViewById(R.id.tvDisplayName)).setText(R.string.placeholder);
                }

                if(!Utils.isEmptyString(pd.userId))
                {
                    ((TextView)findViewById(R.id.tvUserId)).setText(pd.userId);
                }
                else
                {
                    ((TextView)findViewById(R.id.tvUserId)).setText(R.string.placeholder);
                }

                iv = findViewById(R.id.ivConnectivity);
                imageId = R.drawable.ic_baseline_signal_wifi_unknown_24px;
                if(pd.connectivity != null)
                {
                    if(pd.connectivity.rating <= 1)
                    {
                        imageId = R.drawable.ic_baseline_signal_wifi_1_bar_24px;
                    }
                    else if(pd.connectivity.rating <= 2)
                    {
                        imageId = R.drawable.ic_baseline_signal_wifi_2_bar_24px;
                    }
                    else if(pd.connectivity.rating <= 3)
                    {
                        imageId = R.drawable.ic_baseline_signal_wifi_3_bar_24px;
                    }
                    else
                    {
                        imageId = R.drawable.ic_baseline_signal_wifi_4_bar_24px;
                    }
                }
                iv.setImageDrawable(ContextCompat.getDrawable(UserNodeViewActivity.this, imageId));

                iv = findViewById(R.id.ivPower);
                imageId = R.drawable.ic_baseline_battery_unknown_24px;
                if(pd.power != null)
                {
                    if(pd.power.level <= 10)
                    {
                        imageId = R.drawable.ic_baseline_battery_alert_24px;
                    }
                    else if(pd.power.level <= 20)
                    {
                        imageId = R.drawable.ic_baseline_battery_20_24px;
                    }
                    else if(pd.power.level <= 30)
                    {
                        imageId = R.drawable.ic_baseline_battery_30_24px;
                    }
                    else if(pd.power.level <= 50)
                    {
                        imageId = R.drawable.ic_baseline_battery_50_24px;
                    }
                    else if(pd.power.level <= 90)
                    {
                        imageId = R.drawable.ic_baseline_battery_90_24px;
                    }
                    else
                    {
                        imageId = R.drawable.ic_baseline_battery_full_24px;
                    }
                }
                iv.setImageDrawable(ContextCompat.getDrawable(UserNodeViewActivity.this, imageId));


                EntrySet es;

                if(pd.userBiometrics != null)
                {
                    es = pd.userBiometrics.getEsHeartrate();
                    if(es != null) es.updateChart((LineChart)findViewById(R.id.chartHeartRate));

                    es = pd.userBiometrics.getEsSkinTemp();
                    if(es != null) es.updateChart((LineChart)findViewById(R.id.chartSkinTemp));

                    es = pd.userBiometrics.getEsCoreTemp();
                    if(es != null) es.updateChart((LineChart)findViewById(R.id.chartCoreTemp));

                    es = pd.userBiometrics.getEsBloodOxygenation();
                    if(es != null) es.updateChart((LineChart)findViewById(R.id.chartBloodOxygenation));

                    es = pd.userBiometrics.getEsHydrationLevel();
                    if(es != null) es.updateChart((LineChart)findViewById(R.id.chartHydration));

                    es = pd.userBiometrics.getEsFatigueLevel();
                    if(es != null) es.updateChart((LineChart)findViewById(R.id.chartFatigueLevel));

                    es = pd.userBiometrics.getEsTaskEffectivenessLevel();
                    if(es != null) es.updateChart((LineChart)findViewById(R.id.chartTaskEffectiveness));
                }
            }
        });
    }
}
