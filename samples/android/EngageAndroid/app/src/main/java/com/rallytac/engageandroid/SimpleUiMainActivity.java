//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.Manifest;
import android.app.KeyguardManager;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.media.MediaPlayer;

import androidx.annotation.ColorInt;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.FileProvider;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import androidx.core.content.ContextCompat;
import androidx.core.content.res.ResourcesCompat;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.appcompat.widget.PopupMenu;
import androidx.recyclerview.widget.RecyclerView;

import android.os.Environment;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ArrayAdapter;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.gms.maps.CameraUpdate;
import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.OnMapReadyCallback;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.model.BitmapDescriptor;
import com.google.android.gms.maps.model.BitmapDescriptorFactory;
import com.google.android.gms.maps.model.CameraPosition;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.LatLngBounds;
import com.google.android.gms.maps.model.MarkerOptions;
import com.rallytac.engage.engine.Engine;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

public class SimpleUiMainActivity
                            extends
                                AppCompatActivity

                            implements
                                EngageApplication.IUiUpdateListener,
                                EngageApplication.IAssetChangeListener,
                                EngageApplication.IConfigurationChangeListener,
                                EngageApplication.ILicenseChangeListener,
                                EngageApplication.IGroupTimelineListener,
                                EngageApplication.IPresenceChangeListener,
                                EngageApplication.IGroupTextMessageListener,
                                OnMapReadyCallback,
                                GroupSelectorAdapter.SelectionClickListener
{
    private static String TAG = SimpleUiMainActivity.class.getSimpleName();

    private ActiveConfiguration _ac = null;
    private Timer _waitForEngineStartedTimer = null;
    private boolean _anyTxActive = false;
    private boolean _anyTxPending = false;
    private Animation _notificationBarAnimation = null;
    private Runnable _actionOnNotificationBarClick = null;
    private boolean _pttRequested = false;
    private boolean _pttHardwareButtonDown = false;

    private Animation _licensingBarAnimation = null;
    private Runnable _actionOnLicensingBarClick = null;

    private Animation _humanBiometricsAnimation = null;
    private Runnable _actionOnHumanBiometricsClick = null;

    private long _lastHeadsetKeyhookDown = 0;

    private VolumeLevels _vlSaved;
    private VolumeLevels _vlInProgress;
    private boolean _volumeSynced = true;
    private SeekBar _volumeSliderLastMoved = null;

    private boolean _optAllowMultipleChannelView = true;

    private GoogleMap _map;
    private boolean _firstCameraPositioningDone = false;
    private HashMap<String, MapTracker> _mapTrackers = new HashMap<>();

    private RecyclerView _groupSelectorView = null;
    private GroupSelectorAdapter _groupSelectorAdapter = null;

    private int _keycodePtt = 0;

    private HashSet<String> _currentlySelectedGroups = null;

    private ProgressDialog _transmittingAlertProgressDialog = null;

    private class TimelineEventPlayerTracker
    {
        private boolean _isPlayingEventAudio = false;
        private View _currentExpandedEventDetail = null;
        private TimelineEventListAdapter _eventListAdapter = null;
        private MediaPlayer _mediaPlayer = null;
        //private ImageView _ivPlayPause = null;
        //private Timer _eventAudioPlayTimer = null;
        //private boolean _timeLineEventPlayerSeekIsTouched = false;
        //private SeekBar _timelineEventPlayerSeekbar = null;
    }

    static private final int TIMELINE_EVENT_AUDIO_SCALE = 100;

    TimelineEventPlayerTracker _timelineEventPlayerTracker = new TimelineEventPlayerTracker();


    @Override
    public void onMapReady(final GoogleMap googleMap)
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                _map = googleMap;
                applySavedMapSettings();
                positionCameraToAllNodes();
                //onAnyPresenceModificationWhichIsVeryUnoptimizedAndNeedsFixing();
            }
        });
    }

    private void applySavedMapSettings()
    {
        _map.getUiSettings().setMyLocationButtonEnabled(true);
        _map.getUiSettings().setAllGesturesEnabled(true);
        _map.getUiSettings().setCompassEnabled(true);
        _map.getUiSettings().setScrollGesturesEnabled(true);
        _map.getUiSettings().setZoomControlsEnabled(true);
        _map.getUiSettings().setZoomGesturesEnabled(true);
        _map.getUiSettings().setTiltGesturesEnabled(true);
        _map.getUiSettings().setIndoorLevelPickerEnabled(true);
        _map.getUiSettings().setRotateGesturesEnabled(true);
        _map.getUiSettings().setAllGesturesEnabled(true);
        _map.getUiSettings().setMapToolbarEnabled(true);

        _map.setIndoorEnabled(true);
        _map.setBuildingsEnabled(true);
        _map.setTrafficEnabled(true);

        try
        {
            _map.setMyLocationEnabled(true);
        }
        catch (SecurityException se)
        {
            se.printStackTrace();
        }

        _map.setMapType(GoogleMap.MAP_TYPE_NORMAL);

        float lat = Globals.getSharedPreferences().getFloat(PreferenceKeys.MAP_OPTION_CAM_LAT, Float.NaN);
        float lon = Globals.getSharedPreferences().getFloat(PreferenceKeys.MAP_OPTION_CAM_LON, Float.NaN);

        if(!Float.isNaN(lat) && !Float.isNaN(lon))
        {
            CameraPosition.Builder builder = new CameraPosition.Builder();
            builder.target(new LatLng((double)lat, (double)lon));

            /*
            float v;

            v = Globals.getSharedPreferences().getFloat(PreferenceKeys.MAP_OPTION_CAM_TILT, Float.NaN);
            if(!Float.isNaN(v))
            {
                builder.tilt(v);
            }

            v = Globals.getSharedPreferences().getFloat(PreferenceKeys.MAP_OPTION_CAM_BEARING, Float.NaN);
            if(!Float.isNaN(v))
            {
                builder.bearing(v);
            }

            v = Globals.getSharedPreferences().getFloat(PreferenceKeys.MAP_OPTION_CAM_ZOOM, Float.NaN);
            if(!Float.isNaN(v))
            {
                builder.zoom(v);
            }
            */

            CameraPosition cp = builder.build();
            _map.animateCamera(CameraUpdateFactory.newCameraPosition(cp));

            // We're positioning from here so make sure it doesn't get overriden
            _firstCameraPositioningDone = true;
        }
    }

    private void positionCameraToAllNodes()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_map != null)
                {
                    try
                    {
                        if (_mapTrackers.size() > 0 && _map != null)
                        {
                            boolean found = false;
                            LatLngBounds.Builder bld = new LatLngBounds.Builder();

                            for (MapTracker t : _mapTrackers.values())
                            {
                                if (t._marker != null)
                                {
                                    bld.include(t._marker.getPosition());
                                    found = true;
                                }
                            }

                            if (found)
                            {
                                CameraUpdate cu = CameraUpdateFactory.newLatLngBounds(bld.build(), 100);
                                _map.animateCamera(cu);
                            }

                            _firstCameraPositioningDone = true;
                        }
                    }
                    catch (Exception e)
                    {
                        e.printStackTrace();
                    }
                }
            }
        });
    }

    @Override
    public void onPresenceAdded(PresenceDescriptor pd)
    {
        //Log.e(TAG, "onPresenceAdded: " + pd.nodeId + ", " + pd.displayName);
        updateMap();
    }

    @Override
    public void onPresenceChange(PresenceDescriptor pd)
    {
        //Log.e(TAG, "onPresenceChange: " + pd.nodeId + ", " + pd.displayName);
        updateMap();
    }

    @Override
    public void onPresenceRemoved(PresenceDescriptor pd)
    {
        //Log.e(TAG, "onPresenceRemoved: " + pd.nodeId + ", " + pd.displayName);
        updateMap();
    }

    private void updateTrackerTitle(MapTracker t)
    {
        // See what we can use as a title
        String title = "";

        if(!Utils.isEmptyString(t._pd.displayName))
        {
            title = t._pd.displayName;
        }
        else if(!Utils.isEmptyString(t._pd.userId))
        {
            title = t._pd.userId;
        }
        else
        {
            title = t._pd.nodeId;
        }

        if(Utils.isEmptyString(t._title))
        {
            t._title = title;
            t._locationChanged = true;
        }
        else
        {
            if(t._marker != null)
            {
                String existingTitle = t._marker.getTitle();
                if(existingTitle.compareTo(t._title) != 0)
                {
                    t._title = title;
                    t._locationChanged = true;
                }
            }
        }
    }

    public static BitmapDescriptor getBitmapFromVector(@NonNull Context context,
                                                       @DrawableRes int vectorResourceId,
                                                       @ColorInt int tintColor)
    {

        Drawable vectorDrawable = ResourcesCompat.getDrawable(context.getResources(), vectorResourceId, null);

        if (vectorDrawable == null)
        {
            Log.e(TAG, "Requested vector resource was not found");
            return BitmapDescriptorFactory.defaultMarker();
        }

        Bitmap bitmap = Bitmap.createBitmap(vectorDrawable.getIntrinsicWidth(), vectorDrawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
        //Bitmap bitmap = Bitmap.createBitmap(60, 100, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        vectorDrawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
        //DrawableCompat.setTint(vectorDrawable, tintColor);
        vectorDrawable.draw(canvas);

        return BitmapDescriptorFactory.fromBitmap(bitmap);
    }

    private void updateMap()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_map == null)
                {
                    return;
                }

                ArrayList<PresenceDescriptor> nodes = Globals.getEngageApplication().getMissionNodes(null);

                if(nodes != null)
                {
                    // Let's assume they're all gone
                    for(MapTracker t : _mapTrackers.values())
                    {
                        t._gone = true;
                        t._removeFromMap = false;
                    }

                    for (PresenceDescriptor pd : nodes)
                    {
                        MapTracker t = _mapTrackers.get(pd.nodeId);

                        // We found him
                        if(t != null)
                        {
                            t._pd = pd;
                            t._gone = false;

                            // If he has no location, clear out our positioning for him
                            if(pd.location == null)
                            {
                                if(t._marker != null)
                                {
                                    t._removeFromMap = true;
                                }

                                t._lastLatLng = null;
                                t._latLng = null;
                                t._locationChanged = false;
                            }
                            else
                            {
                                // He has a location, do our updates
                                t._latLng = new LatLng(pd.location.getLatitude(), pd.location.getLongitude());

                                // If we don't have a last position, then make it
                                if(t._lastLatLng == null)
                                {
                                    t._lastLatLng = new LatLng(pd.location.getLatitude(), pd.location.getLongitude());
                                    t._locationChanged = true;
                                }
                                else
                                {
                                    if (t._lastLatLng.latitude == t._latLng.latitude &&
                                            t._lastLatLng.longitude == t._latLng.longitude)
                                    {
                                        t._locationChanged = false;
                                    }
                                    else
                                    {
                                        t._locationChanged = true;
                                        t._lastLatLng = new LatLng(t._latLng.latitude, t._latLng.longitude);
                                    }
                                }
                            }
                        }
                        else
                        {
                            // This is a new guy
                            t = new MapTracker();
                            t._pd = pd;

                            // Setup location goodies for him
                            if (pd.location != null)
                            {
                                // Update the title
                                updateTrackerTitle(t);

                                t._latLng = new LatLng(pd.location.getLatitude(), pd.location.getLongitude());
                                t._lastLatLng = new LatLng(t._latLng.latitude, t._latLng.longitude);
                                t._locationChanged = true;
                            }

                            _mapTrackers.put(pd.nodeId, t);
                        }
                    }

                    // Now, let's process our trackers
                    ArrayList<MapTracker> trash = new ArrayList<>();
                    for(MapTracker t : _mapTrackers.values())
                    {
                        if(t._gone)
                        {
                            if(_map != null && t._marker != null)
                            {
                                t._marker.remove();
                            }

                            trash.add(t);
                        }
                        else
                        {
                            // First, see if he needs to be removed from the map
                            if(t._removeFromMap)
                            {
                                t._marker.remove();
                                t._marker = null;
                            }
                            else
                            {
                                if(t._locationChanged)
                                {
                                    // We don't yet have a marker for
                                    if(t._marker == null)
                                    {
                                        MarkerOptions opt = new MarkerOptions();
                                        opt.position(t._latLng);
                                        opt.title(t._title);

                                        // TODO: custom map markers based on node type

                                        // Our marker will come up in red, others in violet
                                        if(t._pd.self)
                                        {
                                            opt.icon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_RED));
                                        }
                                        else
                                        {
                                            opt.icon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_VIOLET));
                                        }

                                        t._marker = _map.addMarker(opt);
                                    }
                                    else
                                    {
                                        // We need to update the marker position
                                        t._marker.setPosition(t._latLng);
                                    }
                                }

                                if(t._marker != null)
                                {
                                    if(t._pd.self)
                                    {
                                        if(_anyTxActive)
                                        {
                                            //t._marker.setIcon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_ORANGE));
                                            t._marker.setIcon(getBitmapFromVector(SimpleUiMainActivity.this, R.drawable.ic_map_marker_generic, 0));
                                        }
                                        else
                                        {
                                            t._marker.setIcon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_RED));
                                        }
                                    }
                                }
                            }
                        }
                    }


                    // Take out the trash
                    for(MapTracker t : trash)
                    {
                        _mapTrackers.remove(t);
                    }
                }
                else
                {
                    // No nodes but we have trackers, take 'em out
                    if (_mapTrackers.size() > 0 && _map != null)
                    {
                        for(MapTracker t : _mapTrackers.values())
                        {
                            if(t._marker != null)
                            {
                                t._marker.remove();
                            }
                        }
                    }

                    _mapTrackers.clear();
                }
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        Log.d(TAG, "onCreate");

        AppCompatDelegate.setCompatVectorFromResourcesEnabled(true);

        _ac = Globals.getEngageApplication().getActiveConfiguration();

        _optAllowMultipleChannelView = Utils.boolOpt(getString(R.string.opt_allow_multiple_channel_view), true);
        //setRequestedOrientation(Utils.intOpt(getString(R.string.opt_lock_orientation), ActivityInfo.SCREEN_ORIENTATION_PORTRAIT));

        _keycodePtt = Utils.intOpt(getString(R.string.app_keycode_ptt), 0);

        super.onCreate(savedInstanceState);

        if(!_optAllowMultipleChannelView)
        {
            _ac.setUiMode(Constants.UiMode.vSingle);
        }

        if (_ac.getUiMode() == Constants.UiMode.vSingle)
        {
            if(_ac.showTextMessaging())
            {
                setContentView(R.layout.activity_main_single_with_text_messaging);
            }
            else
            {
                setContentView(R.layout.activity_main_single);
            }
        }
        else if (_ac.getUiMode() == Constants.UiMode.vMulti)
        {
            setContentView(R.layout.activity_main_multi);
            //setContentView(R.layout.activity_main_single_multi);
        }

        // Remember what groups we're showing in this view
        _currentlySelectedGroups = _ac.getIdsOfSelectedGroups();

        // Hide things we don't necessarily need right now
        hideNotificationBar();
        hideLicensingBar();

        String title;
        String fmt = getString(R.string.replacement_main_screen_title_bar);
        if(Utils.isEmptyString(fmt))
        {
            title = _ac.getMissionName();
            title = title.toUpperCase();
        }
        else
        {
            title = fmt;
            title = title.replace("${appName}", getString(R.string.app_name));
            title = title.replace("${missionName}", _ac.getMissionName().toUpperCase());
        }

        TextView tvTitle = findViewById(R.id.tvTitleBar);
        if(tvTitle != null)
        {
            tvTitle.setText(title);
        }
        else
        {
            setTitle(title);
        }

        restoreSavedState(savedInstanceState);

        assignGroupsToFragments();
        setupMainScreen();
        redrawPttButton();

        _firstCameraPositioningDone = true;

        SupportMapFragment mapFragment = (SupportMapFragment) getSupportFragmentManager().findFragmentById(R.id.map);
        if(mapFragment != null)
        {
            String googleMapsApiKey = Utils.getMetaData("com.google.android.geo.API_KEY");//NON-NLS
            if(!Utils.isEmptyString(googleMapsApiKey))
            {
                mapFragment.getMapAsync(this);
            }
        }

        // Use the group selector if we have it
        _groupSelectorAdapter = null;
        /*
        _groupSelectorView = findViewById(R.id.rvGroupSelector);
        if(_groupSelectorView != null)
        {
            LinearLayoutManager horizontalLayoutManager = new LinearLayoutManager(this, LinearLayoutManager.HORIZONTAL, false);
            _groupSelectorView.setLayoutManager(horizontalLayoutManager);
            _groupSelectorAdapter = new GroupSelectorAdapter(this, _ac.getMissionGroups());
            _groupSelectorAdapter.setClickListener(this);
            _groupSelectorView.setAdapter(_groupSelectorAdapter);
        }
        */

        redrawCardFragments();

        checkForLicenseInstallation();
    }

    @Override
    protected void onSaveInstanceState(Bundle outState)
    {
        Log.d(TAG, "onSaveInstanceState");//NON-NLS
        saveState(outState);
        super.onSaveInstanceState(outState);
    }

    @Override
    protected void onStart()
    {
        Log.d(TAG, "onStart");//NON-NLS
        super.onStart();
    }

    @Override
    protected void onResume()
    {
        Log.d(TAG, "onResume");//NON-NLS
        super.onResume();
        Globals.getEngageApplication().ensureAllIsGood();
        setLockScreenSettings();
        registerWithApp();
        updateLicensingBar();
        updateBiometricsIconDisplay();
        fixPttSize();
    }

    @Override
    protected void onPause()
    {
        Log.d(TAG, "onPause");//NON-NLS
        super.onPause();
        stopTimelineAudioPlayer();
        stopAllTx();
        cancelTimers();
        unregisterFromApp();
    }

    @Override
    protected void onStop()
    {
        Log.d(TAG, "onStop");//NON-NLS
        stopTimelineAudioPlayer();
        stopAllTx();
        cancelTimers();
        super.onStop();
    }

    @Override
    protected void onDestroy()
    {
        Log.d(TAG, "onDestroy");//NON-NLS
        stopTimelineAudioPlayer();
        stopAllTx();
        cancelTimers();
        super.onDestroy();
    }

    @Override
    public void onBackPressed()
    {
        if(_optAllowMultipleChannelView)
        {
            stopTimelineAudioPlayer();
            stopAllTx();
            toggleViewMode();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent)
    {
        if (requestCode == Constants.SETTINGS_REQUEST_CODE)
        {
            if(resultCode == SettingsActivity.MISSION_CHANGED_RESULT || Globals.getEngageApplication().getMissionChangedStatus())
            {
                Log.i(TAG, "============= mission has changed, recreating =======================");//NON-NLS
                onMissionChanged();
            }
        }
        else if(requestCode == Constants.ENGINE_POLICY_EDIT_REQUEST_CODE)
        {
            if(resultCode == RESULT_OK)
            {
                if (intent != null && intent.hasExtra(JsonEditorActivity.JSON_DATA))
                {
                    String json = intent.getStringExtra(JsonEditorActivity.JSON_DATA);
                    SharedPreferences.Editor ed = Globals.getSharedPreferencesEditor();
                    ed.putString(PreferenceKeys.ENGINE_POLICY_JSON, json);
                    ed.apply();
                    Utils.showShortPopupMsg(this, getString(R.string.applying_policy));
                    onMissionChanged();
                }
            }
        }
        else if(requestCode == Constants.MISSION_LISTING_REQUEST_CODE)
        {
            if(intent != null && intent.hasExtra(Constants.MISSION_ACTIVATED_ID))
            {
                String activatedId = intent.getStringExtra(Constants.MISSION_ACTIVATED_ID);
                if(Globals.getEngageApplication().switchToMission(activatedId))
                {
                    ActiveConfiguration ac = Globals.getEngageApplication().updateActiveConfiguration();
                    Toast.makeText(this, String.format(getString(R.string.activated_mission_fmt), ac.getMissionName()), Toast.LENGTH_SHORT).show();
                    onMissionChanged();
                }
                else
                {
                    Toast.makeText(this, Globals.getEngageApplication().getLastSwitchToMissionErrorMsg(), Toast.LENGTH_SHORT).show();
                }
            }
        }
        else if(requestCode == Constants.CERTIFICATE_MANAGER_REQUEST_CODE)
        {
            if(intent != null && intent.hasExtra(Constants.CERTSTORE_CHANGED_TO_FN))
            {
                String newFn = intent.getStringExtra(Constants.CERTSTORE_CHANGED_TO_FN);

                ActiveConfiguration ac = Globals.getEngageApplication().updateActiveConfiguration();
                Toast.makeText(this, String.format(getString(R.string.activated_mission_fmt), ac.getMissionName()), Toast.LENGTH_SHORT).show();
                onMissionChanged();
            }
        }
    }

    private void setLockScreenSettings()
    {
        boolean showIt = Globals.getSharedPreferences().getBoolean(PreferenceKeys.UI_SHOW_ON_LOCK_SCREEN, false);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O_MR1)
        {
            setShowWhenLocked(showIt);
            setTurnScreenOn(showIt);

            if(showIt)
            {
                KeyguardManager keyguardManager = (KeyguardManager) getSystemService(Context.KEYGUARD_SERVICE);
                if(keyguardManager != null)
                {
                    keyguardManager.requestDismissKeyguard(this, null);
                }
            }
        }
        else
        {
            int flags = WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD |
                        WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED |
                        WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON;

            Window window = getWindow();

            if(window != null)
            {
                if(showIt)
                {
                    getWindow().addFlags(flags);
                }
                else
                {
                    getWindow().clearFlags(flags);
                }
            }
        }
    }

    @Override
    public void onAttachedToWindow()
    {
        setLockScreenSettings();
    }

    private long _lastKeydown = 0;
    private boolean _pttRequestIsLatched = false;

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        //Log.d(TAG, "---onKeyDown keyCode=" + keyCode + ", repeat=" + event.getRepeatCount() + ", event=" + event.toString() + ", _lastHeadsetKeyhookDown=" + _lastHeadsetKeyhookDown);

        if(_keycodePtt != 0 && keyCode == _keycodePtt)
        {
            if(event.getRepeatCount() == 0)
            {
                long diff = (event.getDownTime() - _lastKeydown);

                if (diff <= Constants.PTT_KEY_DOUBLE_CLICK_LATCH_THRESHOLD_MS)
                {
                    _pttRequested = !_pttRequested;

                    if (_pttRequested)
                    {
                        Log.d(TAG, "---onKeyDown requesting startTx (latched)");//NON-NLS
                        _pttRequestIsLatched = true;
                        Globals.getEngageApplication().startTx("");
                    }
                    else
                    {
                        Log.d(TAG, "---onKeyDown requesting endTx");//NON-NLS
                        _pttRequestIsLatched = false;
                        Globals.getEngageApplication().endTx();
                    }
                }
                else
                {
                    if(_pttRequested || _anyTxActive || _anyTxPending)
                    {
                        _pttRequested = false;
                        _pttRequestIsLatched = false;
                        Log.d(TAG, "---onKeyDown requesting endTx");//NON-NLS
                        Globals.getEngageApplication().endTx();
                    }
                }

                _lastKeydown = event.getDownTime();
            }
            else
            {
                if(!_pttRequested)
                {
                    _pttRequested = true;
                    _pttRequestIsLatched = false;
                    Log.d(TAG, "---onKeyDown requesting startTx (ptt hold)");//NON-NLS
                    Globals.getEngageApplication().startTx("");
                }
            }
        }
        else
        {
            if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK)
            {
                if (_lastHeadsetKeyhookDown == 0)
                {
                    _lastHeadsetKeyhookDown = event.getDownTime();
                }
                else
                {
                    long diffTime = (event.getDownTime() - _lastHeadsetKeyhookDown);
                    if (diffTime <= 500)
                    {
                        _lastHeadsetKeyhookDown = 0;

                        _pttRequested = !_pttRequested;

                        if (_pttRequested)
                        {
                            Log.d(TAG, "---onKeyDown requesting startTx due to media button double-push");//NON-NLS
                            Globals.getEngageApplication().startTx("");
                        }
                        else
                        {
                            Log.d(TAG, "---onKeyDown requesting endTx due to media button double-push");//NON-NLS
                            Globals.getEngageApplication().endTx();
                        }
                    }
                }
            }
        }

        /*
        // Handle repeat counts - for those headsets that can generate a long-push
        if(keyCode == KeyEvent.KEYCODE_HEADSETHOOK && event.getRepeatCount() > 2)
        {
            if(!_pttRequested)
            {
                Log.d(TAG, "---onKeyDown requesting startTx due to media button push");
                _pttRequested = true;
                Globals.getEngageApplication().startTx(0, 0);
            }
        }
        */

        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        long diffTime = (event.getEventTime() - event.getDownTime());
        //Log.d(TAG, "---onKeyUp keyCode=" + keyCode + ", repeat=" + event.getRepeatCount() + ", event=" + event.toString() + ", diff=" + diffTime);

        if(_keycodePtt != 0 && keyCode == _keycodePtt)
        {
            if((_pttRequested || _anyTxPending || _anyTxActive) && !_pttRequestIsLatched)
            {
                _pttRequested = false;
                _pttRequestIsLatched = false;
                Log.d(TAG, "---onKeyUp requesting endTx");//NON-NLS
                Globals.getEngageApplication().endTx();
            }
        }
        else
        {
            /*
            // Handle PTT indication via specialized key
            if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK)
            {
                if (diffTime > 240 && diffTime < 260)
                {
                    if (!_pttRequested)
                    {
                        Log.d(TAG, "---onKeyUp requesting startTx due to media button PTT");
                        Globals.getEngageApplication().startTx(0, 0);
                    }
                    else
                    {
                        Log.d(TAG, "---onKeyUp requesting endTx due to media button PTT");
                        Globals.getEngageApplication().endTx();
                    }

                    _pttRequested = !_pttRequested;
                }
                else
                {
                    if (_pttRequested)
                    {
                        Log.d(TAG, "---onKeyUp requesting endTx due to media button release");
                        _pttRequested = false;
                        Globals.getEngageApplication().endTx();
                    }
                }
            }
            */
        }

        return super.onKeyUp(keyCode, event);
    }

    private void stopAllTx()
    {
        Globals.getEngageApplication().endTx();
    }

    private void showNotificationBar(final String msg)
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                View v = findViewById(R.id.tvNotificationBar);
                if(v != null)
                {
                    ((TextView)v).setText(msg);
                    if(_notificationBarAnimation == null)
                    {
                        _notificationBarAnimation = AnimationUtils.loadAnimation(SimpleUiMainActivity.this, R.anim.notification_bar_pulse);
                        v.startAnimation(_notificationBarAnimation);
                        v.setVisibility(View.VISIBLE);
                    }
                }
            }
        });
    }

    private void hideNotificationBar()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_notificationBarAnimation != null)
                {
                    _notificationBarAnimation.cancel();
                    _notificationBarAnimation.reset();
                    _notificationBarAnimation = null;
                }

                View v = findViewById(R.id.tvNotificationBar);
                if(v != null)
                {
                    v.setVisibility(View.GONE);
                    v.clearAnimation();
                }
            }
        });
    }

    private void executeActionOnNotificationBarClick()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_actionOnNotificationBarClick != null)
                {
                    _actionOnNotificationBarClick.run();
                }
            }
        });
    }


    private void showLicensingBar(final String msg)
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                View v = findViewById(R.id.tvLicensingBar);
                if(v != null)
                {
                    ((TextView)v).setText(msg);
                    if(_licensingBarAnimation == null)
                    {
                        _actionOnLicensingBarClick = new Runnable()
                        {
                            @Override
                            public void run()
                            {
                                startAboutActivity();
                            }
                        };

                        _licensingBarAnimation = AnimationUtils.loadAnimation(SimpleUiMainActivity.this, R.anim.licensing_bar_pulse);
                        v.startAnimation(_licensingBarAnimation);
                        v.setVisibility(View.VISIBLE);
                    }
                }
            }
        });
    }

    private void hideLicensingBar()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_licensingBarAnimation != null)
                {
                    _licensingBarAnimation.cancel();
                    _licensingBarAnimation.reset();
                    _licensingBarAnimation = null;
                }

                View v = findViewById(R.id.tvLicensingBar);
                if(v != null)
                {
                    v.setVisibility(View.GONE);
                    v.clearAnimation();
                }
            }
        });
    }

    private void executeActionOnLicensingBarClick()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_actionOnLicensingBarClick != null)
                {
                    _actionOnLicensingBarClick.run();
                }
            }
        });
    }

    private void showBiometricsReporting()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                View v = findViewById(R.id.ivHeart);
                if(v != null)
                {
                    if(_humanBiometricsAnimation == null)
                    {
                        _actionOnHumanBiometricsClick = new Runnable()
                        {
                            @Override
                            public void run()
                            {
                                startSettingsActivity();
                            }
                        };

                        _humanBiometricsAnimation = AnimationUtils.loadAnimation(SimpleUiMainActivity.this, R.anim.heart_pulse);
                        v.startAnimation(_humanBiometricsAnimation);
                        v.setVisibility(View.VISIBLE);
                    }
                }
            }
        });
    }

    private void hideBiometricsReporting()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_humanBiometricsAnimation != null)
                {
                    _humanBiometricsAnimation.cancel();
                    _humanBiometricsAnimation.reset();
                    _humanBiometricsAnimation = null;
                }

                View v = findViewById(R.id.ivHeart);
                if(v != null)
                {
                    v.setVisibility(View.GONE);
                    v.clearAnimation();
                }
            }
        });
    }

    private void executeActionOnBiometricsClick()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_actionOnHumanBiometricsClick != null)
                {
                    _actionOnHumanBiometricsClick.run();
                }
            }
        });
    }

    @Override
    public void onMissionChanged()
    {
        Globals.getEngageApplication().logEvent(Analytics.MISSION_CHANGED);
        runOnUiThread(new Runnable()
            {
                @Override
                public void run()
                {
                    recreateWhenEngineIsRestarted();
                }
            });
    }

    @Override
    public void onCriticalConfigurationChange()
    {
        _actionOnNotificationBarClick = new Runnable()
        {
            @Override
            public void run()
            {
                doRecreate();
            }
        };

        showNotificationBar(getString(R.string.config_change_tap_to_activate));
    }

    @Override
    public void onAssetDiscovered(String id, String json)
    {
        try
        {
            JSONObject j = new JSONObject(json);
            String msg = "Talk group '" + j.getString(Engine.JsonFields.Group.name) + "' discovered!";
            Toast.makeText(this, msg, Toast.LENGTH_LONG).show();
        }
        catch (Exception e)
        {
        }
    }

    @Override
    public void onAssetRediscovered(String id, String json)
    {
        // TODO: Do we want to do anything here?
    }

    @Override
    public void onAssetUndiscovered(String id, String json)
    {
        try
        {
            JSONObject j = new JSONObject(json);
            String msg = "Talk group '" + j.getString(Engine.JsonFields.Group.name) + "' lost!";
            Toast.makeText(this, msg, Toast.LENGTH_LONG).show();
        }
        catch (Exception e)
        {
        }
    }

    @Override
    public void onAnyTxPending()
    {
        Log.d(TAG, "onAnyTxPending");//NON-NLS
        _anyTxPending = true;
        redrawPttButton();
        redrawCardFragments();
    }

    @Override
    public void onAnyTxActive()
    {
        Log.d(TAG, "onAnyTxActive");//NON-NLS
        _anyTxActive = true;
        _anyTxPending = true;
        redrawPttButton();
        redrawCardFragments();
        updateMap();
    }

    @Override
    public void onAnyTxEnding()
    {
        Log.d(TAG, "onAnyTxEnding");//NON-NLS
        // Nothing to do here
    }

    @Override
    public void onAllTxEnded()
    {
        Log.d(TAG, "onAllTxEnded");//NON-NLS
        _anyTxActive = false;
        _anyTxPending = false;
        _pttRequested = false;
        _pttHardwareButtonDown = false;
        _lastHeadsetKeyhookDown = 0;

        _ac.setPriorityTxLevel(0);

        redrawPttButton();
        redrawCardFragments();
        updateMap();
    }

    @Override
    public void onGroupUiRefreshNeeded(GroupDescriptor gd)
    {
        updateFragmentForGroup(gd.id);
    }

    @Override
    public void onGroupTxUsurped(GroupDescriptor gd, final String eventExtra)
    {
        if(Utils.isEmptyString(eventExtra))
        {
            return;
        }

        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                Toast.makeText(SimpleUiMainActivity.this, txMsgFromExtra(eventExtra), Toast.LENGTH_SHORT).show();
            }
        });
    }

    @Override
    public void onGroupMaxTxTimeExceeded(GroupDescriptor gd)
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                Toast.makeText(SimpleUiMainActivity.this, R.string.max_tx_time_exceeded, Toast.LENGTH_SHORT).show();
            }
        });
    }

    @Override
    public void onGroupTxFailed(GroupDescriptor gd, final String eventExtra)
    {
        if(Utils.isEmptyString(eventExtra))
        {
            return;
        }

        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                Toast.makeText(SimpleUiMainActivity.this, txMsgFromExtra(eventExtra), Toast.LENGTH_LONG).show();
            }
        });
    }

    @Override
    public void onLicenseChanged()
    {
        updateLicensingBar();
    }

    @Override
    public void onLicenseExpired()
    {
        updateLicensingBar();
    }

    @Override
    public void onLicenseExpiring(double secondsLeft)
    {
        updateLicensingBar();
    }

    private void updateLicensingBar()
    {
        if(Globals.getEngageApplication().getLicenseExpired())
        {
            showLicensingBar(getString(R.string.license_has_expired));
        }
        else
        {
            double secondsLeft = Globals.getEngageApplication().getLicenseSecondsLeft();

            if(secondsLeft > 0)
            {
                // Only show this if our license is going to expire within 10 days
                if (secondsLeft <= (86400 * 10))
                {
                    Calendar now = Calendar.getInstance();
                    Calendar exp = Calendar.getInstance();
                    exp.add(Calendar.SECOND, (int) secondsLeft);
                    String expMsg = Utils.formattedTimespan(now.getTimeInMillis(), exp.getTimeInMillis());
                    showLicensingBar("License expires " + expMsg);
                }
                else
                {
                    hideLicensingBar();
                }
            }
            else
            {
                showLicensingBar(getString(R.string.license_has_expired));
            }
        }
    }

    private void fixPttSize()
    {
        ImageView iv = findViewById(R.id.ivPtt);

        if(Globals.getSharedPreferences().getBoolean(PreferenceKeys.UI_LARGE_PTT_BUTTON, false))
        {
            iv.getLayoutParams().width = (int)getResources().getDimension(R.dimen.ptt_width_large);
            iv.getLayoutParams().height = (int)getResources().getDimension(R.dimen.ptt_height_large);
        }
        else
        {
            iv.getLayoutParams().width = (int)getResources().getDimension(R.dimen.ptt_width_standard);
            iv.getLayoutParams().height = (int)getResources().getDimension(R.dimen.ptt_height_standard);
        }

        iv.requestLayout();
    }

    private void updateBiometricsIconDisplay()
    {
        if(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_EXPERIMENT_ENABLE_HBM, false))
        {
            showBiometricsReporting();
        }
        else
        {
            hideBiometricsReporting();
        }
    }

    private void registerWithApp()
    {
        Globals.getEngageApplication().addUiUpdateListener(this);
        Globals.getEngageApplication().addAssetChangeListener(this);
        Globals.getEngageApplication().addConfigurationChangeListener(this);
        Globals.getEngageApplication().addLicenseChangeListener(this);
        Globals.getEngageApplication().addGroupTimelineListener(this);
        Globals.getEngageApplication().addPresenceChangeListener(this);
        Globals.getEngageApplication().addGroupTextMessageListener(this);
    }

    private void unregisterFromApp()
    {
        Globals.getEngageApplication().removeUiUpdateListener(this);
        Globals.getEngageApplication().removeAssetChangeListener(this);
        Globals.getEngageApplication().removeConfigurationChangeListener(this);
        Globals.getEngageApplication().removeLicenseChangeListener(this);
        Globals.getEngageApplication().removeGroupTimelineListener(this);
        Globals.getEngageApplication().removePresenceChangeListener(this);
        Globals.getEngageApplication().removeGroupTextMessageListener(this);
    }

    private void saveState(Bundle bundle)
    {
        Log.d(TAG, "saveState");//NON-NLS
    }

    private void restoreSavedState(Bundle bundle)
    {
        Log.d(TAG, "restoreSavedState");//NON-NLS
    }

    private void performDevSimulation()
    {
        Toast.makeText(this, "DEVELOPER!! - __devOnly__RunTest", Toast.LENGTH_LONG).show();//NON-NLS
        Globals.getEngageApplication().__devOnly__RunTest();
    }

    @Override
    public void onGroupTimelineEventStarted(final GroupDescriptor gd, final String eventJson)
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                //Toast.makeText(SimpleUiMainActivity.this, "TODO: Event started for " + gd.name, Toast.LENGTH_SHORT).show();
            }
        });
    }

    @Override
    public void onGroupTimelineEventUpdated(final GroupDescriptor gd, final String eventJson)
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                //Toast.makeText(SimpleUiMainActivity.this, "TODO: Event updated for " + gd.name, Toast.LENGTH_SHORT).show();
            }
        });
    }

    @Override
    public void onGroupTimelineEventEnded(final GroupDescriptor gd, final String eventJson)
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                //Toast.makeText(SimpleUiMainActivity.this, "TODO: Event ended for " + gd.name, Toast.LENGTH_SHORT).show();
            }
        });
    }

    @Override
    public void onGroupTextMessageRx(final PresenceDescriptor sourcePd, final String message)
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                FragmentManager fragMan = getSupportFragmentManager();
                List<Fragment> fragments = fragMan.getFragments();

                for(Fragment f : fragments)
                {
                    if(f instanceof TextMessagingFragment)
                    {
                        ((TextMessagingFragment)f).onTextMessageReceived(sourcePd, message);
                    }
                }

                /*
                StringBuilder sb = new StringBuilder();

                if(!Utils.isEmptyString(sourcePd.displayName))
                {
                    sb.append(sourcePd.displayName);
                }
                else if(!Utils.isEmptyString(sourcePd.userId))
                {
                    sb.append(sourcePd.userId);
                }
                else
                {
                    sb.append(sourcePd.nodeId);
                }

                sb.append(":\n\n");
                sb.append(message);

                Toast.makeText(SimpleUiMainActivity.this, sb.toString(), Toast.LENGTH_LONG).show();
                 */
            }
        });
    }


    @Override
    public void onGroupSelectorClick(String id)
    {
        String currentId = getIdOfSingleViewGroup();

        if(currentId.compareTo(id) != 0)
        {
            showSingleView(id);
        }
    }


    private class TimelineEvent
    {
        public int typeIcon;
        public String sourceEntity;
        public String audioUri;
        public long started;
        public long ended;
        public int audioId = -1;
        public long audioLengthMs = 0;
        public View view = null;
        public boolean expanded = false;
    }

    void stopTimelineAudioPlayer()
    {
        /*
        if(_eventAudioPlayTimer != null)
        {
            _eventAudioPlayTimer.cancel();
            _eventAudioPlayTimer = null;
        }
        */

        if(_timelineEventPlayerTracker._mediaPlayer != null)
        {
            _timelineEventPlayerTracker._mediaPlayer.stop();
            _timelineEventPlayerTracker._mediaPlayer.release();
            _timelineEventPlayerTracker._mediaPlayer = null;
        }

        _timelineEventPlayerTracker._isPlayingEventAudio = false;
        //_timeLineEventPlayerSeekIsTouched = false;
    }

    void toggleTimelineEventDetail(View v)
    {
        stopTimelineAudioPlayer();

        TimelineEvent currentTimelineEvent = null;
        TimelineEvent newTimelineEvent = null;

        if(_timelineEventPlayerTracker._currentExpandedEventDetail != null)
        {
            currentTimelineEvent = (TimelineEvent)_timelineEventPlayerTracker._currentExpandedEventDetail.getTag();
            currentTimelineEvent.expanded = false;
            _timelineEventPlayerTracker._currentExpandedEventDetail.setVisibility(View.GONE);
            _timelineEventPlayerTracker._currentExpandedEventDetail = null;
            //_timelineEventPlayerSeekbar = null;
            //_timeLineEventPlayerSeekIsTouched = false;
        }

        v.setBackgroundColor(Color.RED);

        newTimelineEvent = (TimelineEvent)v.getTag();
        if(newTimelineEvent != currentTimelineEvent)
        {
            newTimelineEvent.expanded = true;
            View expansion = v.findViewById(R.id.layEventExpansion);
            //expansion.setVisibility(View.VISIBLE);
            expansion.setTag(newTimelineEvent);
            _timelineEventPlayerTracker._currentExpandedEventDetail = expansion;

            //_timelineEventPlayerTracker._ivPlayPause = _timelineEventPlayerTracker._currentExpandedEventDetail.findViewById(R.id.ivPlayOrPause);

            //_timelineEventPlayerTracker._ivPlayPause.setImageDrawable(ContextCompat.getDrawable(SimpleUiMainActivity.this, R.drawable.ic_pause_media));

            /*
            _timelineEventPlayerSeekbar = _currentExpandedEventDetail.findViewById(R.id.eventAudioSeekbar);
            _timelineEventPlayerSeekbar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
            {
                @Override
                public void onProgressChanged(SeekBar seekBar, int i, boolean b)
                {
                    Log.w(TAG, "onProgressChanged i=" + i);
                    if(_timeLineEventPlayerSeekIsTouched)
                    {
                        _timelineEventMediaPlayer.seekTo(i * TIMELINE_EVENT_AUDIO_SCALE);
                        if (!_timelineEventMediaPlayer.isPlaying())
                        {
                            _timelineEventMediaPlayer.start();
                        }
                    }
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar)
                {
                    Log.w(TAG, "onStartTrackingTouch");
                    _timeLineEventPlayerSeekIsTouched = true;
                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar)
                {
                    Log.w(TAG, "onStopTrackingTouch");
                    _timeLineEventPlayerSeekIsTouched = false;
                }
            });
            */

            try
            {
                _timelineEventPlayerTracker._mediaPlayer = new MediaPlayer();
                String path = newTimelineEvent.audioUri.substring(7);
                _timelineEventPlayerTracker._mediaPlayer.setDataSource(this, Uri.parse(path));

                _timelineEventPlayerTracker._mediaPlayer.setOnPreparedListener(new MediaPlayer.OnPreparedListener()
                {
                    @Override
                    public void onPrepared(MediaPlayer mediaPlayer)
                    {
                        final int durationSecs = (mediaPlayer.getDuration() / 1000);
                        Log.w(TAG, "onPrepared, durationSecs=" + durationSecs);

                        /*
                        _timelineEventPlayerSeekbar.post(new Runnable()
                        {
                            @Override
                            public void run()
                            {
                                _timelineEventPlayerSeekbar.setMax(durationSecs);
                                _timelineEventPlayerSeekbar.setProgress(0);
                            }
                        });
                        */

                        /*
                        _eventAudioPlayTimer = new Timer();
                        _eventAudioPlayTimer.scheduleAtFixedRate(new TimerTask()
                        {
                            @Override
                            public void run()
                            {
                                if(!_timeLineEventPlayerSeekIsTouched)
                                {
                                    if(_timelineEventMediaPlayer.isPlaying())
                                    {
                                        int positionSecs = (_timelineEventMediaPlayer.getCurrentPosition() / 1000);
                                        Log.d(TAG, "updating seekbar, position=" + positionSecs);
                                        _timelineEventPlayerSeekbar.setProgress(positionSecs);
                                    }
                                }
                            }
                        }, 0, (TIMELINE_EVENT_AUDIO_SCALE / 2));
                        */

                        _timelineEventPlayerTracker._mediaPlayer.start();
                    }
                });

                _timelineEventPlayerTracker._mediaPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener()
                {
                    @Override
                    public void onCompletion(MediaPlayer mediaPlayer)
                    {
                        // TODO: Maybe stop the timer?
                        if(_timelineEventPlayerTracker != null)
                        {
                            //_timelineEventPlayerTracker._ivPlayPause.setImageDrawable(ContextCompat.getDrawable(SimpleUiMainActivity.this, R.drawable.ic_play_media));
                            _timelineEventPlayerTracker._eventListAdapter.notifyDataSetChanged();
                        }
                    }
                });

                _timelineEventPlayerTracker._mediaPlayer.prepareAsync();
            }
            catch (Exception e)
            {
                stopTimelineAudioPlayer();
                Toast.makeText(this, getString(R.string.err_cannot_play_event_audio), Toast.LENGTH_SHORT).show();
            }

        }

        _timelineEventPlayerTracker._eventListAdapter.notifyDataSetChanged();
    }

    private class TimelineEventListAdapter extends ArrayAdapter<TimelineEvent>
    {
        private Context _ctx;
        private int _resId;

        public TimelineEventListAdapter(Context ctx, int resId, ArrayList<TimelineEvent> list)
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

            final TimelineEvent item = getItem(position);

            convertView.setTag(item);

            ((ImageView)convertView.findViewById(R.id.ivEventType)).setImageDrawable(ContextCompat.getDrawable(_ctx, item.typeIcon));
            ((TextView)convertView.findViewById(R.id.tvSourceEntity)).setText(item.sourceEntity);
            ((TextView)convertView.findViewById(R.id.tvAudioLengthMs)).setText((item.audioLengthMs / 1000) + " secs");

            View expansion = convertView.findViewById(R.id.layEventExpansion);

            /*
            if(item.expanded)
            {
                expansion.setVisibility(View.VISIBLE);
            }
            else
            {
                expansion.setVisibility(View.GONE);
            }
            */

            expansion.setVisibility(View.GONE);

            String extraInfo;

            if(item.started > 0)
            {
                extraInfo = Utils.javaDateFromUnixMilliseconds(item.started).toString();
            }
            else
            {
                extraInfo = "";
            }

            ((TextView)convertView.findViewById(R.id.tvExtraInformation)).setText(extraInfo);

            convertView.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    toggleTimelineEventDetail(v);
                }
            });

            return convertView;
        }
    }


    @Override
    public void onGroupTimelineReport(final GroupDescriptor gd, final String reportJson)
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                ArrayList<TimelineEvent> events = null;

                if(!Utils.isEmptyString(reportJson))
                {
                    try
                    {
                        JSONObject root = new JSONObject(reportJson);
                        JSONArray list = root.getJSONArray(Engine.JsonFields.TimelineReport.events);
                        if(list != null && list.length() > 0)
                        {
                            for(int x = 0; x < list.length(); x++)
                            {
                                JSONObject obj = list.getJSONObject(x);
                                TimelineEvent te = new TimelineEvent();

                                int dir = obj.getInt(Engine.JsonFields.TimelineEvent.direction);

                                if(dir == 1)
                                {
                                    te.typeIcon = R.drawable.ic_event_receive;
                                }
                                else if(dir == 2)
                                {
                                    te.typeIcon = R.drawable.ic_event_transmit;
                                }
                                else
                                {
                                    te.typeIcon = R.drawable.ic_event_type_error;
                                }

                                te.started = obj.getLong(Engine.JsonFields.TimelineEvent.started);
                                te.ended = obj.optLong(Engine.JsonFields.TimelineEvent.ended, 0);
                                te.sourceEntity = obj.optString(Engine.JsonFields.TimelineEvent.alias);
                                te.audioUri = obj.optString(Engine.JsonFields.TimelineEvent.uri);

                                JSONObject audio = obj.optJSONObject(Engine.JsonFields.TimelineEvent.Audio.objectName);
                                if(audio != null)
                                {
                                    te.audioLengthMs = audio.getLong(Engine.JsonFields.TimelineEvent.Audio.ms);
                                }

                                if(events == null)
                                {
                                    events = new ArrayList<>();
                                }

                                events.add(te);
                            }
                        }
                    }
                    catch(Exception e)
                    {
                        if(events != null)
                        {
                            events.clear();
                            events = null;
                        }

                        e.printStackTrace();
                    }
                }

                if(events == null)
                {
                    Toast.makeText(SimpleUiMainActivity.this, R.string.no_events_in_timeline_report, Toast.LENGTH_SHORT).show();
                }
                else
                {
                    AlertDialog.Builder builder = new AlertDialog.Builder(SimpleUiMainActivity.this);

                    _timelineEventPlayerTracker._eventListAdapter = new TimelineEventListAdapter(SimpleUiMainActivity.this, R.layout.timeline_event_list_entry, events);

                    builder.setAdapter(_timelineEventPlayerTracker._eventListAdapter, null);
                    builder.setPositiveButton(R.string.button_close, new DialogInterface.OnClickListener()
                    {
                        @Override
                        public void onClick(DialogInterface dialog, int which)
                        {
                            SimpleUiMainActivity.this.stopTimelineAudioPlayer();
                        }
                    });

                    builder.show();
                }
            }
        });
    }

    @Override
    public void onGroupTimelineReportFailed(final GroupDescriptor gd)
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                Toast.makeText(SimpleUiMainActivity.this, "Failed to obtain a timeline report", Toast.LENGTH_SHORT).show();
            }
        });
    }

    @Override
    public void onGroupTimelineGroomed(final GroupDescriptor gd, final String eventListJson)
    {
        // TODO: Nothing to do for now in the UI when we're notified that an event was groomed
    }

    @Override
    public void onGroupHealthReport(final GroupDescriptor gd, final String reportJson)
    {
        // TODO: Handle group health report
    }

    @Override
    public void onGroupHealthReportFailed(final GroupDescriptor gd)
    {
        // TODO: Nothing to do for now in the UI when we're notified that a health report failed
    }

    @Override
    public void onGroupStatsReport(final GroupDescriptor gd, final String reportJson)
    {
        // TODO: Handle group stats report
    }

    @Override
    public void onGroupStatsReportFailed(final GroupDescriptor gd)
    {
        // TODO: Nothing to do for now in the UI when we're notified that a stats report failed
    }

    private class TeamListAdapter extends ArrayAdapter<EngageEntity>
    {
        private Context _ctx;
        private int _resId;

        public TeamListAdapter(Context ctx, int resId, ArrayList<EngageEntity> list)
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

            final EngageEntity item = getItem(position);

            ImageView iv = convertView.findViewById(R.id.ivType);
            iv.setImageDrawable(ContextCompat.getDrawable(_ctx, R.drawable.ic_app_logo));

            ((TextView)convertView.findViewById(R.id.tvDisplayName)).setText(item.friendlyName);

            convertView.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    Intent intent = new Intent(_ctx, UserNodeViewActivity.class);
                    intent.putExtra(UserNodeViewActivity.EXTRA_NODE_ID, item.nodeId);
                    startActivity(intent);
                }
            });

            return convertView;
        }
    }

    private AlertDialog _activeGroupListDialog = null;
    private class GroupListAdapter extends ArrayAdapter<GroupDescriptor>
    {
        private Context _ctx;
        private int _resId;

        public GroupListAdapter(Context ctx, int resId, ArrayList<GroupDescriptor> list)
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

            final GroupDescriptor item = getItem(position);

            ImageView iv = convertView.findViewById(R.id.ivGroupEncrypted);

            if(item.isEncrypted)
            {
                iv.setImageDrawable(ContextCompat.getDrawable(_ctx, R.drawable.ic_single_channel_background_secure_idle));
            }
            else
            {
                iv.setImageDrawable(ContextCompat.getDrawable(_ctx, R.drawable.ic_single_channel_background_clear_idle));
            }

            TextView tv = convertView.findViewById(R.id.tvGroupName);
            tv.setText(item.name);

            convertView.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    _activeGroupListDialog.dismiss();
                    showSingleView(item.id);
                }
            });

            return convertView;
        }
    }

    public void showGroupList()
    {
        Globals.getEngageApplication().logEvent(Analytics.VIEW_GROUP_LIST);

        if(_ac == null)
        {
            return;
        }

        final ArrayList<GroupDescriptor> theList = new ArrayList<>();

        for(GroupDescriptor gd : _ac.getMissionGroups())
        {
            if(gd.type == GroupDescriptor.Type.gtAudio)
            {
                theList.add(gd);
            }
        }

        if(!theList.isEmpty())
        {
            final GroupListAdapter arrayAdapter = new GroupListAdapter(this, R.layout.group_list_row_item, theList);

            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setAdapter(arrayAdapter, null);
            builder.setPositiveButton(R.string.button_cancel, null);
            _activeGroupListDialog = builder.show();
        }
    }

    public void showTeamList(String forGroupId)
    {
        Globals.getEngageApplication().logEvent(Analytics.VIEW_TEAM);

        String title = null;

        if(_ac == null || Globals.getEngageApplication().getMissionNodeCount(forGroupId) == 0)
        {
            Utils.showShortPopupMsg(this, R.string.no_team_members_present);
            return;
        }

        if(!Utils.isEmptyString(forGroupId))
        {
            GroupDescriptor gd = Globals.getEngageApplication().getActiveConfiguration().getGroupDescriptor(forGroupId);
            if(gd != null)
            {
                title = gd.name;
            }
            else
            {
                title = getString(R.string.unknown_group);
            }
        }
        else
        {
            title = getString(R.string.all_groups);
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);

        int[] flags = {Constants.GMT_STATUS_FLAG_CONNECTED};
        final ArrayList<EngageEntity> theList = Globals.getEngageApplication().getEntities(forGroupId, flags);

        final TeamListAdapter arrayAdapter = new TeamListAdapter(this, R.layout.team_list_row_item, theList);

        builder.setTitle(title);
        builder.setAdapter(arrayAdapter, null);
        builder.setPositiveButton(R.string.button_close, null);

        builder.show();
    }

    private void startJsonPolicyEditorActivity()
    {
        String json = Globals.getSharedPreferences().getString(PreferenceKeys.ENGINE_POLICY_JSON, "");
        if(Utils.isEmptyString(json))
        {
            json = Utils.getStringResource(this, R.raw.default_engine_policy_template);
        }

        Intent intent = new Intent(this, JsonEditorActivity.class);
        intent.putExtra(JsonEditorActivity.JSON_DATA, json);
        startActivityForResult(intent, Constants.ENGINE_POLICY_EDIT_REQUEST_CODE);
    }

    private void startDevTestActivity()
    {
        Intent intent = new Intent(this, DeveloperTestActivity.class);
        startActivity(intent);
    }

    private void startAboutActivity()
    {
        Globals.getEngageApplication().logEvent(Analytics.VIEW_ABOUT);
        Intent intent = new Intent(this, AboutActivity.class);
        startActivity(intent);
    }

    private void startMissionListActivity()
    {
        Globals.getEngageApplication().logEvent(Analytics.VIEW_MISSION_LIST);
        Intent intent = new Intent(this, MissionListActivity.class);
        startActivityForResult(intent, Constants.MISSION_LISTING_REQUEST_CODE);
    }

    private void startSettingsActivity()
    {
        Globals.getEngageApplication().logEvent(Analytics.VIEW_SETTINGS);
        Intent intent = new Intent(this, SettingsActivity.class);
        startActivityForResult(intent, Constants.SETTINGS_REQUEST_CODE);
    }

    private void startMapActivity()
    {
        Globals.getEngageApplication().logEvent(Analytics.VIEW_MAP);
        Intent intent = new Intent(this, MapActivity.class);
        startActivity(intent);
    }

    private void startCertificateStoresListActivity()
    {
        Globals.getEngageApplication().logEvent(Analytics.VIEW_CERTIFICATES);
        Intent intent = new Intent(this, CertStoreListActivity.class);
        startActivityForResult(intent, Constants.CERTIFICATE_MANAGER_REQUEST_CODE);
    }

    private void requestGroupTimeline(String groupId)
    {
        Globals.getEngageApplication().logEvent(Analytics.VIEW_TIMELINE);
        if(!Utils.isEmptyString(groupId))
        {
            try
            {
                JSONObject obj = new JSONObject();

                obj.put(Engine.JsonFields.TimelineQuery.maxCount, 10);
                obj.put(Engine.JsonFields.TimelineQuery.mostRecentFirst, true);

                Globals.getEngageApplication().getEngine().engageQueryGroupTimeline(groupId, obj.toString());
            }
            catch(Exception e)
            {
                e.printStackTrace();
                Toast.makeText(this, R.string.error_constructing_timeline_query, Toast.LENGTH_SHORT).show();
            }
        }
    }

    private void cancelTimers()
    {
        if(_waitForEngineStartedTimer != null)
        {
            _waitForEngineStartedTimer.cancel();
            _waitForEngineStartedTimer = null;
        }
    }

    private void recreateWhenEngineIsRestarted()
    {
        Globals.getEngageApplication().restartEngine();

        _waitForEngineStartedTimer = new Timer();
        _waitForEngineStartedTimer.scheduleAtFixedRate(new TimerTask()
        {
            @Override
            public void run()
            {
                if(Globals.getEngageApplication().isEngineRunning())
                {
                    Log.i(TAG, "engine is running, proceeding");//NON-NLS
                    _waitForEngineStartedTimer.cancel();

                    runOnUiThread(new Runnable()
                    {
                        @Override
                        public void run()
                        {
                            doRecreate();
                        }
                    });
                }
                else
                {
                    Log.i(TAG, "waiting for engage engine to restart");//NON-NLS
                }
            }
        }, 0, 100);
    }

    private void doRecreate()
    {
        removeAllFragments();

        Globals.getEngageApplication().updateActiveConfiguration();
        HashSet<String> newlySelectedGroups = Globals.getEngageApplication().getActiveConfiguration().getIdsOfSelectedGroups();

        // Figure out which groups to leave
        HashSet<String> groupsToLeave = new HashSet<>();
        for(String p : _currentlySelectedGroups)
        {
            boolean foundInNewlySelected = false;

            for(String n : newlySelectedGroups)
            {
                if(p.compareToIgnoreCase(n) == 0)
                {
                    foundInNewlySelected = true;
                    break;
                }
            }

            if(!foundInNewlySelected)
            {
                groupsToLeave.add(p);
            }
        }

        // Figure out which groups to join
        HashSet<String> groupsToJoin = new HashSet<>();
        for(String n : newlySelectedGroups)
        {
            boolean foundInPreviouslySelected = false;

            for(String p : _currentlySelectedGroups)
            {
                if(n.compareToIgnoreCase(p) == 0)
                {
                    foundInPreviouslySelected = true;
                    break;
                }
            }

            if(!foundInPreviouslySelected)
            {
                groupsToJoin.add(n);
            }
        }

        for(String id : groupsToLeave)
        {
            Globals.getEngageApplication().leaveGroup(id);
        }

        for(String id : groupsToJoin)
        {
            Globals.getEngageApplication().joinGroup(id);
        }

        recreate();
    }

    public void toggleViewMode()
    {
        if(_ac.getUiMode() == Constants.UiMode.vSingle)
        {
            showMultiView();
        }
        else
        {
            showSingleView(null);
        }
    }

    public void setSingleMultiPrimaryGroup(String groupId)
    {

    }

    public void showSingleView(String groupId)
    {
        if(groupId == null)
        {
            groupId = "";
        }

        String currentId = getIdOfSingleViewGroup();

        if(currentId == null)
        {
            currentId = "";
        }

        if(currentId.compareTo(groupId) != 0)
        {
            Globals.getEngageApplication().logEvent(Analytics.SINGLE_VIEW_GROUP_CHANGED);
        }

        if(!Utils.isEmptyString(groupId))
        {
            Globals.getSharedPreferencesEditor().putString(PreferenceKeys.ACTIVE_MISSION_CONFIGURATION_SELECTED_GROUPS_SINGLE, groupId);
            Globals.getSharedPreferencesEditor().apply();
        }

        Globals.getEngageApplication().logEvent(Analytics.VIEW_SINGLE_MODE);

        _ac.setUiMode(Constants.UiMode.vSingle);
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                doRecreate();
            }
        });
    }

    public void showMultiView()
    {
        Globals.getEngageApplication().logEvent(Analytics.VIEW_MULTI_MODE);

        _ac.setUiMode(Constants.UiMode.vMulti);
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                doRecreate();
            }
        });
    }

    private void enableTxForAllGroups()
    {
        for(GroupDescriptor gd : _ac.getMissionGroups())
        {
            gd.txSelected = true;
        }
    }

    private void disableTxForAllGroups()
    {
        for(GroupDescriptor gd : _ac.getMissionGroups())
        {
            gd.txSelected = false;
        }
    }

    private void assignGroupsToFragments()
    {
        FragmentManager fragMan = getSupportFragmentManager();
        List<Fragment> fragments = fragMan.getFragments();

        // Default all to no TX
        disableTxForAllGroups();

        if(_ac.getUiMode() == Constants.UiMode.vSingle)
        {
            boolean gotOne = false;
            for(GroupDescriptor gd : _ac.getMissionGroups())
            {
                if(gd.selectedForSingleView && gd.type == GroupDescriptor.Type.gtAudio )
                {
                    gotOne = true;
                    break;
                }
            }

            if(!gotOne)
            {
                for(GroupDescriptor gd : _ac.getMissionGroups())
                {
                    if(gd.type == GroupDescriptor.Type.gtAudio )
                    {
                        gd.selectedForSingleView = true;
                        break;
                    }
                }
            }
        }

        if(_ac.getUiMode() == Constants.UiMode.vSingle)
        {
            for(GroupDescriptor gd : _ac.getMissionGroups())
            {
                if(gd.selectedForSingleView && gd.type == GroupDescriptor.Type.gtAudio )
                {
                    for(Fragment f : fragments)
                    {
                        if(f instanceof CardFragment)
                        {
                            // Select TX on this group
                            gd.txSelected = true;

                            ((CardFragment)f).setGroupDescriptor(gd);

                            Globals.getSharedPreferencesEditor().putString(PreferenceKeys.ACTIVE_MISSION_CONFIGURATION_SELECTED_GROUPS_SINGLE, gd.id);
                            Globals.getSharedPreferencesEditor().apply();
                        }
                        else if(f instanceof TextMessagingFragment)
                        {
                            ((TextMessagingFragment)f).setGroupDescriptor(gd);
                        }
                    }

                    break;
                }
            }
        }
        else if(_ac.getUiMode() == Constants.UiMode.vMulti)
        {
            for(GroupDescriptor gd : _ac.getMissionGroups())
            {
                if(gd.selectedForMultiView && gd.type == GroupDescriptor.Type.gtAudio)
                {
                    for(Fragment f : fragments)
                    {
                        if(f instanceof CardFragment)
                        {
                            if(((CardFragment)f).getGroupDescriptor() == null)
                            {
                                ((CardFragment)f).setGroupDescriptor(gd);
                                break;
                            }
                        }
                    }
                }
            }
        }

        ArrayList<Fragment> trash = new ArrayList<>();

        // Now, go through the fragments and hide those that have no group
        for(Fragment f : fragments)
        {
            if(f instanceof CardFragment)
            {
                if(((CardFragment)f).getGroupDescriptor() == null)
                {
                    trash.add(f);
                }
            }
        }

        for(Fragment f : trash)
        {
            int id = f.getId();
            FragmentTransaction ft = fragMan.beginTransaction();
            ft.remove(f);
            ft.commit();
            findViewById(id).setVisibility(View.GONE);
        }
    }

    private void switchNetworking(boolean useRp)
    {
        // Do nothing if we're already in this state
        if(_ac.getUseRp() == useRp)
        {
            return;
        }

        Globals.getEngageApplication().logEvent(Analytics.TOGGLE_NETWORKING);

        _ac.setUseRp(useRp);

        SharedPreferences.Editor ed = Globals.getSharedPreferencesEditor();
        ed.putString(PreferenceKeys.ACTIVE_MISSION_CONFIGURATION_JSON, _ac.makeTemplate().toString());
        ed.apply();

        onMissionChanged();
    }

    private void handleClickOnNetworkIcon()
    {
        if(_ac.getCanUseRp())
        {
            //switchNetworking(!_ac.getUseRp());

            String msg;

            /*
            if(!_ac.couldAllGroupsWorkWithoutRallypoint())
            {
                msg = String.format(getString(R.string.currently_connected_globally_fmt), _ac.getRpAddress(), _ac.getRpPort());
                Utils.showLongPopupMsg(SimpleUiMainActivity.this, msg);
            }
            else
            */
            {
                AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);

                alertDialogBuilder.setTitle(getString(R.string.networking));

                if (_ac.getUseRp())
                {
                    msg = String.format(getString(R.string.currently_connected_globally_fmt), _ac.getRpAddress(), _ac.getRpPort());
                    alertDialogBuilder.setPositiveButton(getString(R.string.go_local), new DialogInterface.OnClickListener()
                    {
                        @Override
                        public void onClick(DialogInterface dialog, int which)
                        {
                            switchNetworking(false);
                        }
                    });
                }
                else
                {
                    msg = getString(R.string.currently_connected_via_ip_multicast);
                    alertDialogBuilder.setPositiveButton(getString(R.string.go_global), new DialogInterface.OnClickListener()
                    {
                        @Override
                        public void onClick(DialogInterface dialog, int which)
                        {
                            switchNetworking(true);
                        }
                    });
                }

                alertDialogBuilder.setMessage(msg);
                alertDialogBuilder.setCancelable(true);
                alertDialogBuilder.setNegativeButton(getString(R.string.button_cancel), new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        dialog.cancel();
                    }
                });

                AlertDialog dlg = alertDialogBuilder.create();
                dlg.show();
            }
        }
        else
        {
            Utils.showLongPopupMsg(SimpleUiMainActivity.this, getString(R.string.local_connection_via_ip_multicast));
        }
    }

    public void onClickPlayOrPauseEventAudio(View view)
    {
        Log.e(TAG, "onClickPlayOrPauseEventAudio todo");
        ImageView iv = (ImageView)view;

        if(_timelineEventPlayerTracker._mediaPlayer.isPlaying())
        {
            iv.setImageDrawable(ContextCompat.getDrawable(SimpleUiMainActivity.this, R.drawable.ic_play_media));
            _timelineEventPlayerTracker._mediaPlayer.pause();
        }
        else
        {
            iv.setImageDrawable(ContextCompat.getDrawable(SimpleUiMainActivity.this, R.drawable.ic_pause_media));
            _timelineEventPlayerTracker._mediaPlayer.start();
        }
    }

    public void onClickTeamIcon(View view)
    {
        showTeamList(null);
    }

    public void onClickGroupsIcon(View view)
    {
        showGroupList();
    }

    public void onSwitchToTextMessagingView(View view)
    {
        _ac.setShowTextMessaging(true);
        doRecreate();
    }

    public void onSwitchToVoiceView(View view)
    {
        _ac.setShowTextMessaging(false);
        doRecreate();
    }

    public void onClickShareIcon(View view)
    {
        try
        {
            /*
            JSONObject obj = _ac.makeTemplate();
            if(obj == null)
            {
                throw new Exception("No template available");
            }

            String json = obj.toString();
            String textRecord = (Constants.QR_CODE_HEADER + Constants.QR_VERSION) + json;
            byte[] dataBytes = textRecord.getBytes(Utils.getEngageCharSet());

            String compressedDataString = new String(Utils.compress(dataBytes, 0, dataBytes.length));
            String qrDataString = "@EbQr:" + compressedDataString;
            Bitmap bm = Utils.stringToQrCodeBitmap(qrDataString, Constants.QR_CODE_WIDTH, Constants.QR_CODE_HEIGHT);
            ImageView iv = findViewById(R.id.ivMissionQrCode);
            iv.setImageBitmap(bm);
            */
        }
        catch (Exception e)
        {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    public void onClickMenuIcon(View view)
    {
        showPopupMenu();
    }

    public void onClickNetworkIcon(View view)
    {
        handleClickOnNetworkIcon();
    }

    public void onClickMapIcon(View view)
    {
        startMapActivity();
    }

    public void onClickTimelineIcon(View view)
    {
        String gid = Globals.getSharedPreferences().getString(PreferenceKeys.ACTIVE_MISSION_CONFIGURATION_SELECTED_GROUPS_SINGLE, "");
        if(!Utils.isEmptyString(gid))
        {
            requestGroupTimeline(gid);
        }
    }

    public void onClickSendTextMessageIcon(View view)
    {
        FragmentManager fragMan = getSupportFragmentManager();
        List<Fragment> fragments = fragMan.getFragments();

        for(Fragment f : fragments)
        {
            if(f instanceof TextMessagingFragment)
            {
                ((TextMessagingFragment)f).sendEnteredTextIfAny();
            }
        }
    }


    public void onClickTitleBar(View view)
    {
    }

    public void onClickNotificationBar(View view)
    {
        hideNotificationBar();
        executeActionOnNotificationBarClick();
    }

    public void onClickLicensingBar(View view)
    {
        hideLicensingBar();
        executeActionOnLicensingBarClick();
    }

    public void onClickBiometricsIcon(View view)
    {
        executeActionOnBiometricsClick();
    }

    private void setupMainScreen()
    {
        ImageView iv;

        // Network
        iv = findViewById(R.id.ivNetwork);
        if(iv != null)
        {
            if(_ac.getUseRp())
            {
                iv.setImageDrawable(ContextCompat.getDrawable(this, R.drawable.ic_net_global));
            }
            else
            {
                iv.setImageDrawable(ContextCompat.getDrawable(this, R.drawable.ic_net_local));
            }
        }

        // PTT
        final ImageView ivPtt = findViewById(R.id.ivPtt);

        if(_ac.getPttLatching())
        {
            if(_ac.getPttVoiceControl())
            {
                ivPtt.setContentDescription(getString(R.string.app_ptt_voice_control_on));
            }

            ivPtt.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    _pttRequested = !_pttRequested;

                    if(_pttRequested)
                    {
                        Globals.getEngageApplication().startTx("");
                    }
                    else
                    {
                        Globals.getEngageApplication().endTx();
                    }
                }
            });
        }
        else
        {
            ivPtt.setOnTouchListener(new View.OnTouchListener()
            {
                @Override
                public boolean onTouch(View v, MotionEvent event)
                {
                    if (event.getAction() == MotionEvent.ACTION_DOWN)
                    {
                        _pttRequested = true;
                        Globals.getEngageApplication().startTx("");
                    }
                    else if (event.getAction() == MotionEvent.ACTION_UP)
                    {
                        _pttRequested = false;
                        Globals.getEngageApplication().endTx();
                    }

                    return true;
                }
            });
        }

        // Priority TX
        final ImageView ivAppPriority = findViewById(R.id.ivAppPriority);
        if(ivAppPriority != null)
        {
            ivAppPriority.setLongClickable(true);
            ivAppPriority.setOnLongClickListener(new View.OnLongClickListener()
            {
                @Override
                public boolean onLongClick(View v)
                {
                    int currentLevel = _ac.getPriorityTxLevel();
                    if(currentLevel == 0)
                    {
                        _ac.setPriorityTxLevel(Utils.intOpt(getString(R.string.opt_app_tx_priority_level), 0));
                    }
                    else
                    {
                        _ac.setPriorityTxLevel(0);
                    }

                    redrawPttButton();

                    return true;
                }
            });
        }

        // Alert TX
        final ImageView ivTxAlert = findViewById(R.id.ivTxAlert);
        if(ivTxAlert != null)
        {
            ivTxAlert.setLongClickable(true);
            ivTxAlert.setOnLongClickListener(new View.OnLongClickListener()
            {
                @Override
                public boolean onLongClick(View v)
                {
                    _transmittingAlertProgressDialog = Utils.showProgressMessage(SimpleUiMainActivity.this, getString(R.string.transmitting_alert), _transmittingAlertProgressDialog);

                    // This is kinda nasty - hardcoded audio file.  But this is demonstration only so ok for now.
                    Globals.getEngageApplication().startTx(Globals.getEngageApplication().getRawFilesCacheDir() + "/" + "engage_hail_1.wav");
                    return true;
                }
            });
        }
    }

    public void redrawPttButton()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                ImageView ivPtt = findViewById(R.id.ivPtt);
                ImageView ivAppPriority = findViewById(R.id.ivAppPriority);
                ImageView ivTxAlert = findViewById(R.id.ivTxAlert);

                if(_anyTxActive)
                {
                    if(_ac.getPttVoiceControl())
                    {
                        ivPtt.setImageDrawable(ContextCompat.getDrawable(SimpleUiMainActivity.this, R.drawable.ic_ptt_active_voice_control));
                        ivPtt.setContentDescription(getString(R.string.app_ptt_voice_control_off));
                    }
                    else
                    {
                        ivPtt.setImageDrawable(ContextCompat.getDrawable(SimpleUiMainActivity.this, R.drawable.ic_ptt_active));
                        ivPtt.setContentDescription(null);
                    }
                }
                else if(_anyTxPending)
                {
                    if(_ac.getPttVoiceControl())
                    {
                        ivPtt.setImageDrawable(ContextCompat.getDrawable(SimpleUiMainActivity.this, R.drawable.ic_ptt_transition_voice_control));
                        ivPtt.setContentDescription(getString(R.string.app_ptt_voice_control_off));
                    }
                    else
                    {
                        ivPtt.setImageDrawable(ContextCompat.getDrawable(SimpleUiMainActivity.this, R.drawable.ic_ptt_transition));
                        ivPtt.setContentDescription(null);
                    }
                }
                else
                {
                    if(_ac.getPttVoiceControl())
                    {
                        ivPtt.setImageDrawable(ContextCompat.getDrawable(SimpleUiMainActivity.this, R.drawable.ic_ptt_idle_voice_control));
                        ivPtt.setContentDescription(getString(R.string.app_ptt_voice_control_on));
                    }
                    else
                    {
                        ivPtt.setImageDrawable(ContextCompat.getDrawable(SimpleUiMainActivity.this, R.drawable.ic_ptt_idle));
                        ivPtt.setContentDescription(null);
                    }

                    // See whether app priority icon should be touched at all
                    boolean changeAppPriorityIconVisibility;
                    String visVal = Globals.getContext().getResources().getString(R.string.visibility_visible);
                    String setVal;

                    if(_ac.getUiMode() == Constants.UiMode.vSingle)
                    {
                        setVal = Globals.getContext().getResources().getString(R.string.show_priority_icon_on_app_single_view);
                    }
                    else
                    {
                        setVal = Globals.getContext().getResources().getString(R.string.show_priority_icon_on_app_multi_view);
                    }

                    changeAppPriorityIconVisibility = (setVal == visVal);

                    // If we're idle and in multi-group view, then show/hide the PTT button based on whether any groups have been selected for TX
                    if(Globals.getEngageApplication().getActiveConfiguration().anyGroupsSelectedForTx())
                    {
                        if(Globals.getEngageApplication().hasPermissionToRecordAudio())
                        {
                            ivPtt.setVisibility(View.VISIBLE);

                            if(changeAppPriorityIconVisibility)
                            {
                                ivAppPriority.setVisibility(View.VISIBLE);
                            }
                        }
                        else
                        {
                            ivPtt.setVisibility(View.GONE);
                            if(changeAppPriorityIconVisibility)
                            {
                                ivAppPriority.setVisibility(View.GONE);
                            }
                        }
                    }
                    else
                    {
                        ivPtt.setVisibility(View.GONE);
                        if(changeAppPriorityIconVisibility)
                        {
                            ivAppPriority.setVisibility(View.GONE);
                        }
                    }

                    // Close the TX alert progress dialog if its showing
                    if(_transmittingAlertProgressDialog != null)
                    {
                        _transmittingAlertProgressDialog = Utils.hideProgressMessage(_transmittingAlertProgressDialog);
                    }
                }

                Drawable dw;

                if(_ac.getPriorityTxLevel() == 0)
                {
                    dw = ContextCompat.getDrawable(SimpleUiMainActivity.this, R.drawable.ic_no_priority);
                }
                else
                {
                    dw = ContextCompat.getDrawable(SimpleUiMainActivity.this, R.drawable.ic_high_priority);
                }

                ivAppPriority.setImageDrawable(dw);
            }
        });
    }

    private void redrawCardFragments()
    {
        FragmentManager fragMan = getSupportFragmentManager();
        List<Fragment> fragments = fragMan.getFragments();
        for(Fragment f : fragments)
        {
            if(f instanceof CardFragment)
            {
                ((CardFragment)f).draw();
            }
        }
    }

    private void removeAllFragments()
    {
        FragmentManager fm = getSupportFragmentManager();
        List<Fragment> fragments = fm.getFragments();
        ArrayList<Fragment> trash = new ArrayList<>();
        for (Fragment f : fragments)
        {
            trash.add(f);
        }

        if (!trash.isEmpty())
        {
            FragmentTransaction ft = fm.beginTransaction();
            for (Fragment f : trash)
            {
                ft.remove(f);
            }
            ft.commit();
        }
    }

    private CardFragment getCardForGroup(String id)
    {
        FragmentManager fragMan = getSupportFragmentManager();
        List<Fragment> fragments = fragMan.getFragments();

        for(Fragment f : fragments)
        {
            if(f instanceof CardFragment)
            {
                String groupId = ((CardFragment)f).getGroupId();

                if(groupId != null && groupId.compareTo(id) == 0)
                {
                    return (CardFragment)f;
                }
            }
        }

        return null;
    }

    private void updateFragmentForGroup(String id)
    {
        CardFragment card = getCardForGroup(id);
        if(card == null)
        {
            return;
        }

        card.draw();
    }

    public void showPopupMenu()
    {
        PopupMenu popup = new PopupMenu(this, findViewById(R.id.ivMainScreenMenu));

        MenuInflater inflater = popup.getMenuInflater();
        inflater.inflate(R.menu.main_activity_menu, popup.getMenu());

        // Display the advanced menu options if necessary
        popup.getMenu().findItem(R.id.action_dev_json_policy_editor)
                .setVisible(Globals.getSharedPreferences().getBoolean(PreferenceKeys.ADVANCED_MODE_ACTIVE, false));

        // Display the developer menu options if necessary
        popup.getMenu().findItem(R.id.action_dev_test)
                .setVisible(Globals.getSharedPreferences().getBoolean(PreferenceKeys.DEVELOPER_MODE_ACTIVE, false));

        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener()
        {
            @Override
            public boolean onMenuItemClick(MenuItem item)
            {
                int id = item.getItemId();

                if (id == R.id.action_settings)
                {
                    startSettingsActivity();
                    return true;
                }
                else if (id == R.id.action_missions)
                {
                    startMissionListActivity();
                    return true;
                }
                else if (id == R.id.action_about)
                {
                    startAboutActivity();
                    //performDevSimulation();
                    return true;
                }
                else if (id == R.id.action_dev_test)
                {
                    startDevTestActivity();
                    return true;
                }
                else if (id == R.id.action_dev_json_policy_editor)
                {
                    startJsonPolicyEditorActivity();
                    return true;
                }
                else if (id == R.id.action_disconnect)
                {
                    verifyDisconnect();
                    return true;
                }
                else if (id == R.id.action_security)
                {
                    startCertificateStoresListActivity();
                    return true;
                }

                return false;
            }
        });

        popup.show();
    }

    private void verifyDisconnect()
    {
        String s;

        s = String.format(getString(R.string.confirm_disconnect), getString(R.string.app_name));

        final TextView message = new TextView(this);
        final SpannableString ss = new SpannableString(s);

        message.setText(ss);
        message.setMovementMethod(LinkMovementMethod.getInstance());
        message.setPadding(32, 32, 32, 32);

        AlertDialog dlg = new AlertDialog.Builder(this)
                .setTitle(R.string.title_disconnect)
                .setCancelable(false)
                .setPositiveButton(R.string.button_yes, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                        Globals.getEngageApplication().terminateApplicationAndReturnToAndroid(SimpleUiMainActivity.this);
                        //Globals.getEngageApplication().stopEngine();
                        //moveTaskToBack(true);
                        //finishAndRemoveTask();
                    }
                }).setNegativeButton(R.string.button_no, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                    }
                }).setView(message).create();

        dlg.show();
    }

    private void setVolumeLevels(String groupId, VolumeLevels vl)
    {
        Globals.getEngageApplication().getEngine().engageSetGroupRxVolume(groupId, vl.left, vl.right);
    }

    private int getSelectedVolumeLeft()
    {
        return _vlInProgress.left;
    }

    private int getSelectedVolumeRight()
    {
        return _vlInProgress.right;
    }

    public void showVolumeSliders(final String groupId)
    {
        if(Utils.isEmptyString(groupId))
        {
            return;
        }

        _vlSaved = Globals.getEngageApplication().loadVolumeLevels(groupId);

        _vlInProgress = new VolumeLevels();
        _vlInProgress.left = _vlSaved.left;
        _vlInProgress.right = _vlSaved.right;

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        LayoutInflater inflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View v = inflater.inflate(R.layout.volume_dialog, null);

        builder
                .setView(v)
                .setCancelable(false)
                .setTitle(getString(R.string.adjust_group_volume))
                .setPositiveButton(getString(R.string.button_save), new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        setVolumeLevels(groupId, _vlInProgress);
                        Globals.getEngageApplication().saveVolumeLevels(groupId, _vlInProgress);
                    }
                })
                .setNegativeButton(getString(R.string.button_cancel), new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        _vlInProgress.left = _vlSaved.left;
                        _vlInProgress.right = _vlSaved.right;
                        setVolumeLevels(groupId, _vlInProgress);
                    }
                })
                .setNeutralButton(getString(R.string.button_reset), new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        _vlInProgress.left = 100;
                        _vlInProgress.right = 100;
                        setVolumeLevels(groupId, _vlInProgress);
                        Globals.getEngageApplication().saveVolumeLevels(groupId, _vlInProgress);
                    }
                });

        final SeekBar sbLeft = v.findViewById(R.id.volumeSeekBarLeft);
        final SeekBar sbRight = v.findViewById(R.id.volumeSeekBarRight);
        final Switch swSync = v.findViewById(R.id.swSync);

        swSync.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener()
        {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                _volumeSynced = isChecked;

                if(_volumeSliderLastMoved != null)
                {
                    if(_volumeSliderLastMoved == sbLeft)
                    {
                        sbRight.setProgress(_vlInProgress.left);
                    }
                    else
                    {
                        sbLeft.setProgress(_vlInProgress.right);
                    }
                }
            }
        });

        swSync.setChecked(_volumeSynced);

        sbLeft.setMax(200);
        sbLeft.setProgress(getSelectedVolumeLeft());
        sbLeft.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
        {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
            {
                _vlInProgress.left = progress;
                if(_volumeSynced)
                {
                    sbRight.setProgress(progress);
                }

                setVolumeLevels(groupId, _vlInProgress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar)
            {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar)
            {
                _volumeSliderLastMoved = seekBar;
            }
        });


        sbRight.setMax(200);
        sbRight.setProgress(getSelectedVolumeRight());
        sbRight.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
        {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
            {
                _vlInProgress.right = progress;
                if(_volumeSynced)
                {
                    sbLeft.setProgress(progress);
                }

                setVolumeLevels(groupId, _vlInProgress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar)
            {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar)
            {
                _volumeSliderLastMoved = seekBar;
            }
        });

        builder.create().show();
    }

    private String getIdOfSingleViewGroup()
    {
        String rc = null;

        for(GroupDescriptor gd : _ac.getMissionGroups())
        {
            if(gd.selectedForSingleView && gd.type == GroupDescriptor.Type.gtAudio )
            {
                rc = gd.id;
                break;
            }
        }

        return rc;
    }


    private String getIdOfFirstAudioGroup()
    {
        String rc = null;

        for(GroupDescriptor gd : _ac.getMissionGroups())
        {
            if(gd.type == GroupDescriptor.Type.gtAudio )
            {
                rc = gd.id;
                break;
            }
        }

        return rc;
    }

    private void switchToNextIdInList(boolean reverse)
    {
        String currentId = getIdOfSingleViewGroup();
        String newId = null;

        if(Utils.isEmptyString(currentId))
        {
            newId = getIdOfFirstAudioGroup();
        }
        else
        {
            ArrayList<String> ids = new ArrayList<>();
            for(GroupDescriptor gd : _ac.getMissionGroups())
            {
                if (gd.type == GroupDescriptor.Type.gtAudio)
                {
                    ids.add(gd.id);
                }
            }

            if(!ids.isEmpty())
            {
                if(reverse)
                {
                    Collections.reverse(ids);
                }
            }

            boolean foundExisting = false;
            for(String s : ids)
            {
                if(s.compareTo(currentId) == 0)
                {
                    foundExisting = true;
                }
                else
                {
                    if(foundExisting)
                    {
                        newId = s;
                        break;
                    }
                }
            }

            if(Utils.isEmptyString(newId) && !ids.isEmpty())
            {
                newId = ids.get(0);
            }
        }

        if(!Utils.isEmptyString(newId))
        {
            if(currentId.compareTo(newId) != 0)
            {
                showSingleView(newId);
            }
        }
    }

    public void onClickPreviousGroup(View view)
    {
        switchToNextIdInList(true);
    }

    public void onClickNextGroup(View view)
    {
        switchToNextIdInList(false);
    }

    private String txMsgFromExtra(String eventExtraJson)
    {
        String rc;

        try
        {
            JSONObject j = new JSONObject(eventExtraJson);
            JSONObject txd = j.getJSONObject(Engine.JsonFields.GroupTxDetail.objectName);
            Engine.TxStatus status = Engine.TxStatus.fromInt(txd.getInt(Engine.JsonFields.GroupTxDetail.status));

            switch(status)
            {
                case started:
                    rc = getString(R.string.tx_status_reason_started);
                    break;

                case ended:
                    rc = getString(R.string.tx_status_reason_ended);
                    break;

                case notAnAudioGroup:
                    rc = getString(R.string.tx_status_reason_tx_on_non_audio);
                    break;

                case notJoined:
                    rc = getString(R.string.tx_status_reason_tx_on_non_joined);
                    break;

                case notConnected:
                    rc = getString(R.string.tx_status_reason_tx_on_non_connected);
                    break;

                case alreadyTransmitting:
                    rc = getString(R.string.tx_status_reason_akready_tx);
                    break;

                case invalidParams:
                    rc = "Invalid parameters";
                    break;

                case priorityTooLow:
                    int localPriority = txd.optInt(Engine.JsonFields.GroupTxDetail.localPriority, 0);
                    int remotePriority = txd.optInt(Engine.JsonFields.GroupTxDetail.remotePriority, 0);

                    if(localPriority == remotePriority)
                    {
                        rc = getString(R.string.tx_status_reason_another_already_tx);
                    }
                    else
                    {
                        rc = getString(R.string.tx_status_reason_another_already_tx_higher_priority);
                    }
                    break;

                case rxActiveOnNonFdx:
                    rc = getString(R.string.tx_status_reason_another_already_tx);
                    break;

                case cannotSubscribeToMic:
                    rc = getString(R.string.tx_status_reason_cannot_open_mic);
                    break;

                case invalidId:
                    rc = getString(R.string.tx_status_reason_invalid_group_id);
                    break;

                case undefined:
                default:
                    rc = getString(R.string.tx_status_reason_unknown);
                    break;
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            rc = "?";
        }

        return rc;
    }

    private void dev_setRtpFactor(int f)
    {
        String json = Globals.getSharedPreferences().getString(PreferenceKeys.ENGINE_POLICY_JSON, "");
        if(Utils.isEmptyString(json))
        {
            json = Utils.getStringResource(this, R.raw.default_engine_policy_template);
        }

        try
        {
            JSONObject jo = new JSONObject(json);
            JSONObject networking = jo.getJSONObject("networking");
            networking.put("rtpJitterMaxFactor", f);
            json = jo.toString();

            SharedPreferences.Editor ed = Globals.getSharedPreferencesEditor();
            ed.putString(PreferenceKeys.ENGINE_POLICY_JSON, json);
            ed.apply();

            onMissionChanged();
        }
        catch (Exception e)
        {
            Utils.showLongPopupMsg(this, e.getMessage());
        }
    }

    private void checkForLicenseInstallation()
    {
        if(Globals.getSharedPreferences().getBoolean(PreferenceKeys.CHECKED_FOR_LICENSING_DONE, false))
        {
            return;
        }

        SharedPreferences.Editor ed = Globals.getSharedPreferencesEditor();
        ed.putBoolean(PreferenceKeys.CHECKED_FOR_LICENSING_DONE, true);
        ed.apply();

        String key = Globals.getSharedPreferences().getString(PreferenceKeys.USER_LICENSING_KEY, null);
        if(!Utils.isEmptyString(key))
        {
            return;
        }

        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);

        alertDialogBuilder.setTitle(getString(R.string.no_license_found_title));
        alertDialogBuilder.setMessage(getString(R.string.no_license_found_question_enter_now));
        alertDialogBuilder.setPositiveButton(getString(R.string.button_yes), new DialogInterface.OnClickListener()
        {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
                startAboutActivity();
            }
        });
        alertDialogBuilder.setNegativeButton(getString(R.string.button_no), new DialogInterface.OnClickListener()
        {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
                dialog.cancel();
            }
        });

        alertDialogBuilder.setCancelable(false);

        AlertDialog dlg = alertDialogBuilder.create();
        dlg.show();
    }
}
