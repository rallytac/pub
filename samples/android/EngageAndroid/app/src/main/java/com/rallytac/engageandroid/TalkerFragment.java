//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;

public class TalkerFragment extends Fragment
{
    private static final String ARG_COLUMN_COUNT = "column-count";
    private int _columnCount = 0;

    private TalkerViewRecyclerAdapter _theAdapter = null;

    public TalkerFragment()
    {
    }

    @SuppressWarnings("unused")
    public static TalkerFragment newInstance(int columnCount, int layoutResourceId)
    {
        TalkerFragment fragment = new TalkerFragment();
        Bundle args = new Bundle();
        args.putInt(ARG_COLUMN_COUNT, columnCount);
        fragment.setArguments(args);

        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        if (getArguments() != null)
        {
            _columnCount = getArguments().getInt(ARG_COLUMN_COUNT);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
    {
        View view = inflater.inflate(getContainerResourceId(), container, false);

        if (view instanceof RecyclerView)
        {
            Context context = view.getContext();
            RecyclerView recyclerView = (RecyclerView) view;
            if (_columnCount <= 1)
            {
                recyclerView.setLayoutManager(new LinearLayoutManager(context));
            }
            else
            {
                recyclerView.setLayoutManager(new GridLayoutManager(context, _columnCount));
            }

            _theAdapter = new TalkerViewRecyclerAdapter(getItemResourceId());

            recyclerView.setAdapter(_theAdapter);
        }

        return view;
    }

    public void setTalkers(ArrayList<TalkerDescriptor> lst)
    {
        _theAdapter.setItems(lst);
    }

    public int getContainerResourceId()
    {
        return -1;
    }

    public int getItemResourceId()
    {
        return -1;
    }
}
