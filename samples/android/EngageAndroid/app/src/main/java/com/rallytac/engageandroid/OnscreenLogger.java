package com.rallytac.engageandroid;

import android.content.Context;
import android.os.Environment;
import android.view.View;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Date;

public class OnscreenLogger {
    private Context _ctx;
    private int _maxLines = 1000;
    private ArrayList<String> _lines = new ArrayList<String>();

    private LogAdapter _adapter;
    private LinearLayoutManager _layoutManager;
    private RecyclerView _recyclerView;

    OnscreenLogger(Context ctx, RecyclerView recyclerView) {
        _ctx = ctx;
        _adapter = new LogAdapter(_lines);
        _layoutManager = new LinearLayoutManager(ctx);
        _recyclerView = recyclerView;
        _recyclerView.setAdapter(_adapter);
        _recyclerView.setLayoutManager(_layoutManager);
    }

    public void addLine(String line) {
        if(_lines.size() >= _maxLines) {
            _lines.remove(0);
            _adapter.notifyItemRemoved(0);
        }

        _lines.add(Utils.getShortDateTime(new Date()) + ":" + line);
        _adapter.notifyItemInserted(_lines.size() - 1);

        _recyclerView.post(new Runnable() {
            @Override
            public void run() {
                _recyclerView.scrollToPosition(_adapter.getItemCount() - 1);
            }
        });
    }

    public void clear() {
        _lines.clear();
        _adapter.notifyDataSetChanged();
    }

    public void saveToFile() {
        if (_lines == null || _lines.isEmpty()) {
            Toast.makeText(_ctx, "Nothing to save", Toast.LENGTH_LONG).show();
            return;
        }

        File downloadsDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
        File file = new File(downloadsDir, "engagelog.log");

        try (BufferedWriter writer = new BufferedWriter(new FileWriter(file))) {
            for (String line : _lines) {
                writer.write(line);
                writer.newLine(); // Write each string on a new line
            }
            //Log.d("FileUtils", "File saved to: " + file.getAbsolutePath());
        } catch (IOException e) {
            //Log.e("FileUtils", "Error writing file", e);
        }

        Toast.makeText(_ctx, "Save to file", Toast.LENGTH_LONG).show();
    }
}
