//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;

public class MapTrackerListAdapter extends ArrayAdapter<MapTracker>
{
    private Context _ctx;
    private int _resId;

    public MapTrackerListAdapter(Context ctx, int resId, ArrayList<MapTracker> list)
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

        MapTracker item = getItem(position);

        ImageView iv = convertView.findViewById(R.id.ivType);
        iv.setImageDrawable(ContextCompat.getDrawable(_ctx, R.drawable.ic_app_logo));

        String displayName = item._pd.displayName;
        if(Utils.isEmptyString(displayName))
        {
            displayName = item._pd.userId;
            if(Utils.isEmptyString(displayName))
            {
                displayName = item._pd.nodeId;
            }
        }

        ((TextView)convertView.findViewById(R.id.tvDisplayName)).setText(displayName);

        return convertView;
    }
}