//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.List;

public class TalkerViewRecyclerAdapter extends RecyclerView.Adapter<TalkerViewRecyclerAdapter.ViewHolder>
{
    private int _layoutResourceId = -1;
    private final List<TalkerDescriptor> _items = new ArrayList<>();

    public TalkerViewRecyclerAdapter(int layoutResourceId)
    {
        _layoutResourceId = layoutResourceId;
    }

    public void setItems(ArrayList<TalkerDescriptor> lst)
    {
        _items.clear();
        if(lst != null && lst.size() > 0)
        {
            for(TalkerDescriptor td : lst)
            {
                _items.add(td);
            }
        }
        notifyDataSetChanged();
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
    {
        View view = LayoutInflater.from(parent.getContext()).inflate(_layoutResourceId, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(final ViewHolder holder, int position)
    {
        holder._item = _items.get(position);

        holder._tvTalkerName.setText(holder._item.alias);
        if((holder._item.rxFlags & 1) == 1)
        {
            holder._tvTalkerName.setBackgroundResource(R.drawable.ic_emergency_talker);
        }
        else if(holder._item.txPriority > 0)
        {
            holder._tvTalkerName.setBackgroundResource(R.drawable.ic_priority_talker);
        }
        else
        {
            holder._tvTalkerName.setBackgroundResource(R.drawable.ic_normal_talker);
        }
    }

    @Override
    public int getItemCount()
    {
        return _items.size();
    }

    public class ViewHolder extends RecyclerView.ViewHolder
    {
        public final View _view;
        public final TextView _tvTalkerName;
        public TalkerDescriptor _item;

        public ViewHolder(View view)
        {
            super(view);
            _view = view;
            _tvTalkerName = (TextView) view.findViewById(R.id.talkerName);
        }

        @Override
        public String toString()
        {
            return super.toString() + " '" + _tvTalkerName.getText() + "'";
        }
    }
}
