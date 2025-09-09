/*
 *
 * Copyright (c) 2025 Rally Tactical Systems, Inc.
 *
 */
package com.rallytac.engageandroid;

import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;

public class LogAdapter extends RecyclerView.Adapter<LogAdapter.LogViewHolder> {
    private final ArrayList<String> logList;

    public LogAdapter(ArrayList<String> logList) {
        this.logList = logList;
    }

    @NonNull
    @Override
    public LogViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        // Inflate the custom layout for each log item
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.log_list_item, parent, false);
        return new LogViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull LogViewHolder holder, int position) {
        // Bind the log string to the TextView
        holder.logTextView.setText(logList.get(position));

        // Set alternating background colors
        if (position % 2 == 0) {
            holder.itemView.setBackgroundColor(Color.WHITE); // Even rows
        } else {
            holder.itemView.setBackgroundColor(Color.LTGRAY); // Odd rows
        }
    }

    @Override
    public int getItemCount() {
        return logList.size();
    }

    // ViewHolder class for better performance
    public static class LogViewHolder extends RecyclerView.ViewHolder {
        public final TextView logTextView;

        public LogViewHolder(@NonNull View itemView) {
            super(itemView);
            logTextView = itemView.findViewById(R.id.logTextView);
        }
    }
}
