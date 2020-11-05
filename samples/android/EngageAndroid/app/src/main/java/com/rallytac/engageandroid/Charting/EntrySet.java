//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid.Charting;

import android.graphics.Color;

import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.rallytac.engageandroid.Biometrics.DataSeries;
import com.rallytac.engageandroid.Biometrics.SeriesElement;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class EntrySet
{
    private String _title;
    private int _baseTimestamp;
    private int _minValue;
    private int _maxValue;
    private int _maxEntries;
    private int _lineColor;
    private int _fillColor;
    private int _baseLevel;
    private int _okRange;
    private int _warningRange;
    private int _dangerRange;

    private List<Entry> _entries = new ArrayList<>();
    private Random _rnd = new Random();

    public EntrySet(String title, int minValue, int maxValue, int maxEntries, int baseLevel, int okRange, int warningRange, int dangerRange)
    {
        _title = title;
        _minValue = minValue;
        _maxValue = maxValue;
        _maxEntries = maxEntries;
        _baseLevel = baseLevel;
        _okRange = okRange;
        _warningRange = warningRange;
        _dangerRange = dangerRange;
        _baseTimestamp = 0;
    }

    private void doPostUpdateProcessing()
    {
        while( _entries.size() > _maxEntries )
        {
            _entries.remove(0);
        }

        float total = 0.0f;
        float index = 0f;               // NOTE: Sets X index to start at 0 and increement by 1
        for(Entry e : _entries)
        {
            total += e.getY();
            e.setX(index);
            index++;
        }

        float measurePoint = (total / _entries.size());
        float diff = (_maxValue - _minValue);

        if( (measurePoint >= (_baseLevel - _okRange)) && (measurePoint <= (_baseLevel + _okRange)) )
        {
            _lineColor = Color.parseColor("#65991c");//NON-NLS
            _fillColor = Color.parseColor("#b6e376");//NON-NLS
        }
        else if( (measurePoint >= (_baseLevel - _warningRange)) && (measurePoint <= (_baseLevel + _warningRange)) )
        {
            _lineColor = Color.parseColor("#e06900");//NON-NLS
            _fillColor = Color.parseColor("#ff9d47");//NON-NLS
        }
        else
        {
            _lineColor = Color.parseColor("#c21b00");//NON-NLS
            _fillColor = Color.parseColor("#f57864");//NON-NLS
        }
    }

    public void addRandomEntry(int tick)
    {
        int r = _rnd.nextInt(_maxValue - _minValue);
        if(r < 0)
        {
            r *= -1;
        }

        r += _minValue;

        _entries.add(new Entry(tick, r));
        doPostUpdateProcessing();
    }

    public void updateChart(LineChart chart)
    {
        LineDataSet dataSet = new LineDataSet(_entries, _title);
        dataSet.setColor(_lineColor);
        dataSet.setDrawFilled(true);
        dataSet.setFillColor(_fillColor);
        dataSet.setDrawValues(false);
        dataSet.setDrawVerticalHighlightIndicator(false);

        LineData lineData = new LineData(dataSet);

        chart.setData(lineData);
        chart.setDrawGridBackground(false);
        chart.getAxisRight().setEnabled(false);
        chart.getXAxis().setEnabled(false);
        chart.invalidate();
    }

    public boolean merge(DataSeries ds)
    {
        boolean rc = false;

        try
        {
            if(ds.getElementCount() == 0)
            {
                throw new Exception("No elements in data series");
            }

            // TODO : We're not looking at timestamps - lots of work to do there

            for(SeriesElement se : ds.getSeries())
            {
                Entry entry = new Entry(0, se.getValue());
                _entries.add(entry);
            }

            doPostUpdateProcessing();

            rc = true;
        }
        catch (Exception e)
        {
            rc = false;
        }

        return rc;
    }
}
