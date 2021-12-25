//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.annotation.TargetApi;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.media.AudioDeviceInfo;
import android.os.Build;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;

import androidx.appcompat.app.ActionBar;

import android.provider.Settings;
import android.util.Log;
import android.view.MenuItem;

import com.rallytac.engage.engine.Engine;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;

public class SettingsActivity extends AppCompatPreferenceActivity
{
    private static String TAG = SettingsActivity.class.getSimpleName();

    public static int MISSION_CHANGED_RESULT = (RESULT_FIRST_USER + 1);
    private static SettingsActivity _thisActivity;
    private static boolean _prefChangeIsBeingForcedByBinding = false;

    private void indicateMissionChanged()
    {
        ((EngageApplication) getApplication()).setMissionChangedStatus(true);
    }

    private static Preference.OnPreferenceChangeListener sBindPreferenceSummaryToValueListener = new Preference.OnPreferenceChangeListener()
    {
        @Override
        public boolean onPreferenceChange(Preference preference, Object value)
        {
            String stringValue = value.toString();

            //Globals.getLogger().e(TAG, ">>>>>> onPreferenceChange: " + preference.getKey());

            if(!_prefChangeIsBeingForcedByBinding)
            {
                String key = preference.getKey();
                if(key.startsWith("rallypoint_")//NON-NLS
                    || key.startsWith("user_")//NON-NLS
                    || key.startsWith("network_")//NON-NLS
                    || key.startsWith("mission_") //NON-NLS
                    || key.startsWith("ui."))//NON-NLS
                {
                    Globals.getLogger().i(TAG, "mission parameters changed");//NON-NLS
                    _thisActivity.indicateMissionChanged();
                }
            }

            if (preference instanceof ListPreference)
            {
                ListPreference listPreference = (ListPreference) preference;
                int index = listPreference.findIndexOfValue(stringValue);

                preference.setSummary(
                        index >= 0
                                ? listPreference.getEntries()[index]
                                : null);

            }
            else
            {
                if( !(preference instanceof SwitchPreference) )
                {
                    preference.setSummary(stringValue);
                }
            }
            return true;
        }
    };

    private static boolean isXLargeTablet(Context context)
    {
        return (context.getResources().getConfiguration().screenLayout
                & Configuration.SCREENLAYOUT_SIZE_MASK) >= Configuration.SCREENLAYOUT_SIZE_XLARGE;
    }

    private static void bindPreferenceSummaryToValue(Preference preference)
    {
        preference.setOnPreferenceChangeListener(sBindPreferenceSummaryToValueListener);

        _prefChangeIsBeingForcedByBinding = true;

        if(!(preference instanceof SwitchPreference))
        {
            String strVal;

            String className = preference.getClass().toString();

            if(className.contains("SeekBarPreference"))//NON-NLS
            {
                strVal = Integer.toString(PreferenceManager
                                    .getDefaultSharedPreferences(preference.getContext())
                                    .getInt(preference.getKey(), 0));
            }
            else
            {
                strVal = PreferenceManager
                            .getDefaultSharedPreferences(preference.getContext())
                            .getString(preference.getKey(), "");
            }

            strVal = Utils.trimString(strVal);

            sBindPreferenceSummaryToValueListener.onPreferenceChange(preference, strVal);
        }

        _prefChangeIsBeingForcedByBinding = false;
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

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        _thisActivity = this;
        super.onCreate(savedInstanceState);
        setupActionBar();



        getFragmentManager().beginTransaction()
                .replace(android.R.id.content, new GeneralPreferenceFragment()).commit();
    }

    @Override
    public void onBackPressed()
    {
        super.onBackPressed();
    }

    @Override
    protected void onPause()
    {
        super.onPause();
    }

    @Override
    protected void onStop()
    {
        super.onStop();
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
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
    public boolean onIsMultiPane()
    {
        return isXLargeTablet(this);
    }

    protected boolean isValidFragment(String fragmentName)
    {
        return PreferenceFragment.class.getName().equals(fragmentName)
                || GeneralPreferenceFragment.class.getName().equals(fragmentName);
    }


    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    public static class BasePreferenceFragment extends PreferenceFragment
    {
        @Override
        public void onCreate(Bundle savedInstanceState)
        {
            super.onCreate(savedInstanceState);
            setHasOptionsMenu(true);
        }

        @Override
        public boolean onOptionsItemSelected(MenuItem item)
        {
            int id = item.getItemId();
            if (id == android.R.id.home)
            {
                getActivity().onBackPressed();
                return true;
            }
            return super.onOptionsItemSelected(item);
        }
    }

    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    public static class GeneralPreferenceFragment extends BasePreferenceFragment
    {
        private void hidePreferenceCategory(String nm)
        {
            Preference pc = this.findPreference(nm);
            if(pc != null)
            {
                PreferenceScreen ps = this.getPreferenceScreen();
                if(ps != null)
                {
                    ps.removePreference(pc);
                }
            }
        }

        private void hidePreferenceInCategory(String nm, String cat)
        {
            PreferenceCategory pc = (PreferenceCategory) this.findPreference(cat);
            if(pc != null)
            {
                Preference p = this.findPreference(PreferenceKeys.USER_UI_PTT_LATCHING);
                if(p != null)
                {
                    pc.removePreference(p);
                }
            }
        }

        @Override
        public void onCreate(Bundle savedInstanceState)
        {
            super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.pref_general);

            // Handle whether developer mode is active
            if(Globals.getSharedPreferences().getBoolean(PreferenceKeys.DEVELOPER_MODE_ACTIVE, false))
            {
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.DEVELOPER_USE_DEV_LICENSING_SYSTEM));
            }
            else
            {
                hidePreferenceCategory("prefcat_developer_options");//NON-NLS
            }

            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_ID));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_DISPLAY_NAME));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_ALIAS_ID));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_TONE_LEVEL_PTT));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_TONE_LEVEL_NOTIFICATION));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_TONE_LEVEL_ERROR));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_LOCATION_SHARED));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_LOCATION_ACCURACY));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_LOCATION_INTERVAL_SECS));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_LOCATION_MIN_DISPLACEMENT));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_NOTIFY_NODE_JOIN));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_NOTIFY_NODE_LEAVE));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_NOTIFY_NEW_AUDIO_RX));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_NOTIFY_NETWORK_ERROR));
            //bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_THEME_DARK_MODE));

            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_MICROPHONE_DENOISE));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_MICROPHONE_AGC_LEVEL));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_SPEAKER_AGC_LEVEL));

            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_AEC_ENABLED));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_AEC_MODE));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_AEC_CNG));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_AEC_SPEAKER_TAIL_MS));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_AEC_DISABLE_STEREO));

            //bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_JITTER_LOW_LATENCY_ENABLED));

            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_ANDROID_AUDIO_API));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_ANDROID_AUDIO_SHARING_MODE));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_ANDROID_AUDIO_PERFORMANCE_MODE));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_ANDROID_AUDIO_USAGE));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_ANDROID_AUDIO_CONTENT_TYPE));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_ANDROID_AUDIO_INPUT_PRESET));

            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_ENGINE_INTERNAL_AUDIO));

            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_NOTIFY_VIBRATIONS));
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_NOTIFY_PTT_EVERY_TIME));

            // Show/hide PTT latching depending on flavor option
            if(Globals.getContext().getResources().getBoolean(R.bool.opt_ptt_latching))
            {
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_UI_PTT_LATCHING));
            }
            else
            {
                hidePreferenceInCategory(PreferenceKeys.USER_UI_PTT_LATCHING, "prefcat_user_interface");
            }

            // Show/hide text messaging option depending on flavor option
            if(Globals.getContext().getResources().getBoolean(R.bool.opt_supports_text_messaging))
            {
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.UI_SHOW_TEXT_MESSAGING));
            }
            else
            {
                hidePreferenceInCategory(PreferenceKeys.UI_SHOW_TEXT_MESSAGING, "prefcat_user_interface");
            }


            /*
            bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_UI_PTT_VOICE_CONTROL));
            */

            // Show/hide the ability to set the screen orientation
            if(Globals.getContext().getResources().getInteger(R.integer.opt_lock_orientation) == ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED)
            {
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.UI_ORIENTATION));
            }
            else
            {
                hidePreferenceInCategory(PreferenceKeys.UI_ORIENTATION, "prefcat_user_interface");
            }

            // NICs
            {
                HashMap<String, String> uniqueNics = new HashMap<>();
                String nicArrayJsonString = Globals.getEngageApplication().getEngine().engageGetNetworkInterfaceDevices();
                JSONArray ar;

                try
                {
                    JSONObject container = new JSONObject(nicArrayJsonString);
                    ar = container.getJSONArray(Engine.JsonFields.ListOfNetworkInterfaceDevice.objectName);

                    for(int idx = 0; idx < ar.length(); idx++)
                    {
                        JSONObject nic = ar.getJSONObject(idx);

                        String name = nic.optString(Engine.JsonFields.NetworkInterfaceDevice.name, null);
                        if(!Utils.isEmptyString(name))
                        {
                            if(!uniqueNics.containsKey(name))
                            {
                                String friendlyName = nic.optString(Engine.JsonFields.NetworkInterfaceDevice.friendlyName, null);
                                if(Utils.isEmptyString(friendlyName))
                                {
                                    friendlyName = name;
                                }

                                uniqueNics.put(name, friendlyName);
                            }
                        }
                    }
                }
                catch (Exception e)
                {
                    uniqueNics.clear();
                    e.printStackTrace();
                }
                
                if(uniqueNics.size() > 0)
                {
                    final CharSequence[] entries = new CharSequence[uniqueNics.size()];
                    final CharSequence[] entryValues = new CharSequence[uniqueNics.size()];

                    int index = 0;
                    for (String s : uniqueNics.keySet())
                    {
                        entryValues[index] = s;
                        entries[index] = uniqueNics.get(s);
                        index++;
                    }

                    final ListPreference listPreference = (ListPreference) findPreference(PreferenceKeys.NETWORK_BINDING_NIC_NAME);
                    listPreference.setEntries(entries);
                    listPreference.setEntryValues(entryValues);
                    bindPreferenceSummaryToValue(findPreference(PreferenceKeys.NETWORK_BINDING_NIC_NAME));
                }

                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.NETWORK_MULTICAST_FAILOVER_ENABLED));
                //bindPreferenceSummaryToValue(findPreference(PreferenceKeys.NETWORK_MULTICAST_FAILOVER_SECS));
            }



            // Audio devices
            {

                String audioDeviceArrayJsonString = Globals.getEngageApplication().getEngine().engageGetAudioDevices();

                JSONArray ar;
                ArrayList<JSONObject> inputs = new ArrayList<>();
                ArrayList<JSONObject> outputs = new ArrayList<>();

                // Split into inputs and outputs
                try
                {
                    JSONObject container = new JSONObject(audioDeviceArrayJsonString);
                    ar = container.getJSONArray(Engine.JsonFields.ListOfAudioDevice.objectName);

                    for(int idx = 0; idx < ar.length(); idx++)
                    {
                        JSONObject ad = ar.getJSONObject(idx);

                        int direction = ad.getInt(Engine.JsonFields.AudioDevice.direction);
                        int deviceId = ad.getInt(Engine.JsonFields.AudioDevice.deviceId);
                        String name = ad.optString(Engine.JsonFields.AudioDevice.name, null);
                        String hardwareId = ad.optString(Engine.JsonFields.AudioDevice.hardwareId, null);

                        if(deviceId >= 0 && !Utils.isEmptyString(name) && !Utils.isEmptyString(hardwareId))
                        {
                            if(direction == 1)      // dirInput
                            {
                                inputs.add(ad);
                            }
                            else if(direction == 2) // dirOutput
                            {
                                outputs.add(ad);
                            }
                        }
                    }
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                }

                // Fill the input selector
                {
                    final ListPreference listPreference = (ListPreference) findPreference(PreferenceKeys.USER_AUDIO_INPUT_DEVICE);
                    final CharSequence[] entries = new CharSequence[inputs.size()];
                    final CharSequence[] entryValues = new CharSequence[inputs.size()];
                    int index = 0;
                    for (JSONObject ad : inputs)
                    {
                        entries[index] = ad.optString("name", "?") + " (" + Globals.getEngageApplication().androidAudioDeviceName(Integer.parseInt(ad.optString("type", "-1"))) + ") " + ad.optString("extra", "");
                        entryValues[index] = Integer.toString(ad.optInt("deviceId", 0));
                        index++;
                    }

                    listPreference.setEntries(entries);
                    listPreference.setEntryValues(entryValues);
                    bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_INPUT_DEVICE));
                }

                // Fill the output selector
                {
                    final ListPreference listPreference = (ListPreference) findPreference(PreferenceKeys.USER_AUDIO_OUTPUT_DEVICE);
                    final CharSequence[] entries = new CharSequence[outputs.size()];
                    final CharSequence[] entryValues = new CharSequence[outputs.size()];
                    int index = 0;
                    for (JSONObject ad : outputs)
                    {
                        entries[index] = ad.optString("name", "?") + " (" + Globals.getEngageApplication().androidAudioDeviceName(Integer.parseInt(ad.optString("type", "-1"))) + ") " + ad.optString("extra", "");
                        entryValues[index] = Integer.toString(ad.optInt("deviceId", 0));
                        index++;
                    }

                    listPreference.setEntries(entries);
                    listPreference.setEntryValues(entryValues);
                    bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_AUDIO_OUTPUT_DEVICE));
                }
            }


            // Bluetooth audio device
            {
                final ListPreference listPreference = (ListPreference) findPreference(PreferenceKeys.USER_BT_DEVICE_ADDRESS);
                ArrayList<BluetoothDevice> btDevs = BluetoothManager.getDevices();
                final CharSequence[] entries = new CharSequence[btDevs.size()];
                final CharSequence[] entryValues = new CharSequence[btDevs.size()];
                int index = 0;
                for (BluetoothDevice dev : btDevs)
                {
                    entries[index] = dev.getName();
                    entryValues[index] = dev.getAddress();
                    index++;
                }
                listPreference.setEntries(entries);
                listPreference.setEntryValues(entryValues);
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_BT_DEVICE_ADDRESS));
            }

            if(!Globals.getContext().getResources().getBoolean(R.bool.opt_experimental_general_enabled))
            {
                hidePreferenceCategory("prefcat_experimental_general");//NON-NLS
            }
            else
            {
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_ENABLE_TX_SMOOTHING));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_ALLOW_DTX));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_ENABLE_SSDP_DISCOVERY));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_ENABLE_CISTECH_GV1_DISCOVERY));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_CISTECH_GV1_DISCOVERY_ADDRESS));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_CISTECH_GV1_DISCOVERY_PORT));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_CISTECH_GV1_DISCOVERY_TIMEOUT_SECS));

                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_ENABLE_TRELLISWARE_DISCOVERY));

                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_ENABLE_DEVICE_REPORT_CONNECTIVITY));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_ENABLE_DEVICE_REPORT_POWER));
            }


            if(!Globals.getContext().getResources().getBoolean(R.bool.opt_experimental_human_biometrics_enabled))
            {
                hidePreferenceCategory("prefcat_experimental_human_biometrics");//NON-NLS
            }
            else
            {
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_ENABLE_HBM));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_HBM_INTERVAL_SECS));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_HBM_ENABLE_HEART_RATE));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_HBM_ENABLE_SKIN_TEMP));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_HBM_ENABLE_CORE_TEMP));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_HBM_ENABLE_BLOOD_OXY));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_HBM_ENABLE_BLOOD_HYDRO));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_HBM_ENABLE_FATIGUE_LEVEL));
                bindPreferenceSummaryToValue(findPreference(PreferenceKeys.USER_EXPERIMENT_HBM_ENABLE_TASK_EFFECTIVENESS_LEVEL));
            }
        }
    }
}
