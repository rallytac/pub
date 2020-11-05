//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;
import android.view.GestureDetector;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;

public class GroupSelectorAdapter extends RecyclerView.Adapter<GroupSelectorAdapter.ViewHolder>
{
    public interface SelectionClickListener
    {
        void onGroupSelectorClick(String id);
    }

    private Context _ctx;
    private ArrayList<GroupDescriptor> _groups;
    private LayoutInflater _inflater;
    private SelectionClickListener _selectionClickListener = null;

    GroupSelectorAdapter(Context ctx, ArrayList<GroupDescriptor> groups)
    {
        _ctx = ctx;

        // We only want audio groups
        _groups = new ArrayList<>();
        for(GroupDescriptor gd : groups)
        {
            if(gd.type == GroupDescriptor.Type.gtAudio)
            {
                _groups.add(gd);
            }
        }

        _inflater = LayoutInflater.from(_ctx);
    }

    public void setClickListener(SelectionClickListener selectionClickListener)
    {
        _selectionClickListener = selectionClickListener;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int i)
    {
        View view = _inflater.inflate(R.layout.group_selector_item, viewGroup, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder viewHolder, int i)
    {
        GroupDescriptor gd = _groups.get(i);

        if(gd.isEncrypted)
        {
            viewHolder._ivIcon.setImageDrawable(ContextCompat.getDrawable(_ctx, R.drawable.ic_single_channel_background_secure_idle));
        }
        else
        {
            viewHolder._ivIcon.setImageDrawable(ContextCompat.getDrawable(_ctx, R.drawable.ic_single_channel_background_clear_idle));
        }

        viewHolder._tvName.setText(gd.name);
    }

    @Override
    public int getItemCount()
    {
        return _groups.size();
    }

    public class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener
    {
        private class DoubleTapListener extends GestureDetector.SimpleOnGestureListener
        {
            @Override
            public boolean onDoubleTap(MotionEvent e)
            {
                if(_selectionClickListener != null)
                {
                    _selectionClickListener.onGroupSelectorClick(_groups.get(getAdapterPosition()).id);
                }

                return true;
            }

            @Override
            public boolean onFling(MotionEvent e1, MotionEvent e2, float vX, float vY)
            {
                return true;
            }
        }

        private GestureDetector _gestureDetector;
        private ImageView _ivIcon;
        private TextView _tvName;

        ViewHolder(View itemView)
        {
            super(itemView);

            _gestureDetector = new GestureDetector(GroupSelectorAdapter.this._ctx, new DoubleTapListener());

            _ivIcon = itemView.findViewById(R.id.ivGroupSelectorIcon);
            _tvName = itemView.findViewById(R.id.tvGroupSelectorName);

            itemView.setOnClickListener(this);

            /*
            _ivIcon.setOnTouchListener(new View.OnTouchListener()
            {
                @Override
                public boolean onTouch(View v, MotionEvent event)
                {
                    return _gestureDetector.onTouchEvent(event);
                }
            });
            */
        }

        @Override
        public void onClick(View view)
        {
            if(_selectionClickListener != null)
            {
                _selectionClickListener.onGroupSelectorClick(_groups.get(getAdapterPosition()).id);
            }
        }
    }
}
