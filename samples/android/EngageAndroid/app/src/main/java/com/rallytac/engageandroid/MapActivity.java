//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.DialogInterface;
import android.os.Build;
import android.os.Bundle;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import android.view.MenuItem;
import android.view.View;

import com.google.android.gms.maps.CameraUpdate;
import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.OnMapReadyCallback;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.model.BitmapDescriptorFactory;
import com.google.android.gms.maps.model.CameraPosition;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.LatLngBounds;
import com.google.android.gms.maps.model.MarkerOptions;

import java.util.ArrayList;
import java.util.HashMap;

public class MapActivity extends
        AppCompatActivity
                         implements
                            OnMapReadyCallback,
                            EngageApplication.IPresenceChangeListener
{
    private static String TAG = MapActivity.class.getSimpleName();

    private GoogleMap _map;
    private EngageApplication _app;
    private HashMap<String, MapTracker> _trackers = new HashMap<>();
    private boolean _firstCameraPositioningDone = false;

    private int[] _views = {GoogleMap.MAP_TYPE_NORMAL, GoogleMap.MAP_TYPE_SATELLITE, GoogleMap.MAP_TYPE_TERRAIN, GoogleMap.MAP_TYPE_HYBRID};
    private int _viewIndex = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        _app = (EngageApplication) getApplication();

        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
        {
            String googleMapsApiKey = Utils.getMetaData("com.google.android.geo.API_KEY");//NON-NLS
            if(!Utils.isEmptyString(googleMapsApiKey))
            {
                try
                {
                    ActionBar ab = getSupportActionBar();
                    if (ab != null)
                    {
                        ab.setDisplayHomeAsUpEnabled(true);
                    }

                    updateTitle();

                    setContentView(R.layout.activity_map);
                    SupportMapFragment mapFragment = (SupportMapFragment) getSupportFragmentManager()
                            .findFragmentById(R.id.map);

                    mapFragment.getMapAsync(this);
                }
                catch (Exception e)
                {
                    setContentView(R.layout.empty_layout);
                    showIssueAndFinish(getString(R.string.title_map_incompatibility_issue), getString(R.string.mapping_does_not_appear_to_be_supported));
                }
            }
            else
            {
                setContentView(R.layout.empty_layout);
                showIssueAndFinish(getString(R.string.title_no_google_maps_key), getString(R.string.need_google_maps_key));
            }
        }
        else
        {
            setContentView(R.layout.empty_layout);
            showIssueAndFinish(getString(R.string.title_android_version_too_old_for_maps), getString(R.string.maps_not_available_on_such_and_old_android_platform));
        }
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

    private void showIssueAndFinish(String title, String message)
    {
        AlertDialog dlg = new AlertDialog.Builder(this)
                .setTitle(title)
                .setMessage(message)
                .setCancelable(false)
                .setPositiveButton(getString(R.string.button_ok), new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                        MapActivity.this.finish();
                    }
                }).create();

        dlg.show();
    }

    @Override
    protected void onResume()
    {
        _app.addPresenceChangeListener(this);
        super.onResume();
    }

    @Override
    protected void onPause()
    {
        _app.removePresenceChangeListener(this);
        super.onPause();
    }

    @Override
    protected void onDestroy()
    {
        _app.removePresenceChangeListener(this);
        saveMapSettings();
        super.onDestroy();
    }

    @Override
    public void onPresenceAdded(PresenceDescriptor pd)
    {
        onAnyPresenceModificationWhichIsVeryUnoptimizedAndNeedsFixing();
    }

    @Override
    public void onPresenceChange(PresenceDescriptor pd)
    {
        onAnyPresenceModificationWhichIsVeryUnoptimizedAndNeedsFixing();
    }

    @Override
    public void onPresenceRemoved(PresenceDescriptor pd)
    {
        onAnyPresenceModificationWhichIsVeryUnoptimizedAndNeedsFixing();
    }

    // TODO: FIXME onAnyPresenceModificationWhichIsVeryUnoptimizedAndNeedsFixing
    private void onAnyPresenceModificationWhichIsVeryUnoptimizedAndNeedsFixing()
    {
        runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                updateMap();

                if(!_firstCameraPositioningDone)
                {
                    positionCameraToAllNodes();
                }
            }
        });
    }


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
                onAnyPresenceModificationWhichIsVeryUnoptimizedAndNeedsFixing();
            }
        });
    }

    private void updateTitle()
    {
        int count = 0;

        if(!_trackers.isEmpty())
        {
            count = _trackers.size();
        }

        if(count == 1)
        {
            setTitle(String.format(getString(R.string.title_map_team_1), count));
        }
        else
        {
            setTitle(String.format(getString(R.string.title_map_team), count));
        }
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

    private void updateMap()
    {
        if(_map == null)
        {
            return;
        }

        ArrayList<PresenceDescriptor> nodes = Globals.getEngageApplication().getMissionNodes(null);

        if(nodes != null)
        {
            synchronized (_trackers)
            {
                // Let's assume they're all gone
                for(MapTracker t : _trackers.values())
                {
                    t._gone = true;
                    t._removeFromMap = false;
                }

                for (PresenceDescriptor pd : nodes)
                {
                    MapTracker t = _trackers.get(pd.nodeId);

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

                            // Update the title
                            updateTrackerTitle(t);
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

                        _trackers.put(pd.nodeId, t);
                    }
                }

                // Now, let's process our trackers
                ArrayList<MapTracker> trash = new ArrayList<>();
                for(MapTracker t : _trackers.values())
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
                        else if(t._locationChanged)
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
                    }
                }

                // Take out the trash
                for(MapTracker t : trash)
                {
                    _trackers.remove(t);
                }
            }
        }
        else
        {
            // No nodes but we have trackers, take 'em out
            synchronized (_trackers)
            {
                if (_trackers.size() > 0 && _map != null)
                {
                    for(MapTracker t : _trackers.values())
                    {
                        if(t._marker != null)
                        {
                            t._marker.remove();
                        }
                    }
                }

                _trackers.clear();
            }
        }

        updateTitle();
    }

    private void positionCameraToAllNodes()
    {
        if(_map == null)
        {
            return;
        }

        synchronized (_trackers)
        {
            try
            {
                if (_trackers.size() > 0 && _map != null)
                {
                    boolean found = false;
                    LatLngBounds.Builder bld = new LatLngBounds.Builder();

                    for (MapTracker t : _trackers.values())
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
            catch(Exception e)
            {
                e.printStackTrace();
            }
        }
    }

    private void zoomTo(MapTracker t)
    {
        if(_map != null && t._marker != null)
        {
            float zoomLevel = _map.getCameraPosition().zoom;
            _map.moveCamera(CameraUpdateFactory.newLatLngZoom(t._marker.getPosition(), zoomLevel));
            t._marker.showInfoWindow();
        }
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

        _viewIndex = Globals.getSharedPreferences().getInt(PreferenceKeys.MAP_OPTION_VIEW_INDEX, 0);
        _map.setMapType(Globals.getSharedPreferences().getInt(PreferenceKeys.MAP_OPTION_TYPE, _views[_viewIndex]));

        float lat = Globals.getSharedPreferences().getFloat(PreferenceKeys.MAP_OPTION_CAM_LAT, Float.NaN);
        float lon = Globals.getSharedPreferences().getFloat(PreferenceKeys.MAP_OPTION_CAM_LON, Float.NaN);

        if(!Float.isNaN(lat) && !Float.isNaN(lon))
        {
            CameraPosition.Builder builder = new CameraPosition.Builder();
            builder.target(new LatLng((double)lat, (double)lon));

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

            CameraPosition cp = builder.build();
            _map.animateCamera(CameraUpdateFactory.newCameraPosition(cp));

            // We're positioning from here so make sure it doesn't get overriden
            _firstCameraPositioningDone = true;
        }
    }

    private void saveMapSettings()
    {
        if(_map != null)
        {
            Globals.getSharedPreferencesEditor().putInt(PreferenceKeys.MAP_OPTION_VIEW_INDEX,_viewIndex);
            Globals.getSharedPreferencesEditor().putInt(PreferenceKeys.MAP_OPTION_TYPE, _map.getMapType());
            Globals.getSharedPreferencesEditor().putFloat(PreferenceKeys.MAP_OPTION_CAM_TILT, _map.getCameraPosition().tilt);
            Globals.getSharedPreferencesEditor().putFloat(PreferenceKeys.MAP_OPTION_CAM_LAT, (float)_map.getCameraPosition().target.latitude);
            Globals.getSharedPreferencesEditor().putFloat(PreferenceKeys.MAP_OPTION_CAM_LON, (float)_map.getCameraPosition().target.longitude);
            Globals.getSharedPreferencesEditor().putFloat(PreferenceKeys.MAP_OPTION_CAM_BEARING, _map.getCameraPosition().bearing);
            Globals.getSharedPreferencesEditor().putFloat(PreferenceKeys.MAP_OPTION_CAM_ZOOM, _map.getCameraPosition().zoom);
            Globals.getSharedPreferencesEditor().apply();
        }

    }

    public void onClickShowPdList(View view)
    {
        showPdList();
    }

    private void showPdList()
    {
        if(_trackers.size() == 0)
        {
            Utils.showShortPopupMsg(this, getString(R.string.map_no_team_members_found));
            return;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);

        final ArrayList<MapTracker> theList = new ArrayList<>();
        for(MapTracker t : _trackers.values())
        {
            theList.add(t);
        }

        final MapTrackerListAdapter arrayAdapter = new MapTrackerListAdapter(this, R.layout.team_list_row_item, theList);

        builder.setAdapter(arrayAdapter, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
                MapTracker t = arrayAdapter.getItem(which);
                zoomTo(t);
            }
        });

        builder.setPositiveButton(R.string.button_map_show_all, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i)
            {
                positionCameraToAllNodes();
            }
        });

        builder.show();
    }

    public void onClickNextView(View view)
    {
        setNextView();
    }

    private void setNextView()
    {
        if(_map == null)
        {
            return;
        }

        _viewIndex++;
        if(_viewIndex == _views.length)
        {
            _viewIndex = 0;
        }

        _map.setMapType(_views[_viewIndex]);
    }
}
