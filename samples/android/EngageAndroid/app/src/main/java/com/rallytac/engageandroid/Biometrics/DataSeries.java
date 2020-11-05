//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid.Biometrics;

import com.rallytac.engageandroid.NetworkByteBuffer;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class DataSeries
{
    public final int IT_SECONDS = 0;
    public final int IT_MILLISECONDS = 1;
    public final int IT_MINUTES = 2;
    public final int IT_HOURS = 3;
    public final int IT_DAYS = 4;

    public final int IT_DEFAULT = IT_SECONDS;

    public final int IM_DEFAULT = 1;

    public final int VT_BYTE = 1;
    public final int VT_DEFAULT = VT_BYTE;

    private String _name;
    private int _binaryId;
    private int _timestamp;
    private int _incrementType;
    private int _incrementMultiplier;
    private int _valueType;

    private List<SeriesElement> _series = Collections.synchronizedList(new ArrayList<SeriesElement>());

    public DataSeries()
    {
        init();
    }

    public DataSeries(int binaryId)
    {
        init();
        _binaryId = binaryId;
    }

    private void init()
    {
        _binaryId = 0;
        _timestamp = (int)(System.currentTimeMillis()/1000L);
        _incrementType = IT_DEFAULT;
        _incrementMultiplier = IM_DEFAULT;
        _valueType = VT_DEFAULT;
    }

    @Override
    public String toString()
    {
        StringBuilder sb = new StringBuilder();

        sb.append("n=");//NON-NLS
            sb.append(_name);
        sb.append("ts=");//NON-NLS
            sb.append(_timestamp);
        sb.append("it=");//NON-NLS
            sb.append(_incrementType);
        sb.append("im=");//NON-NLS
            sb.append(_incrementMultiplier);
        sb.append("vt=");//NON-NLS
            sb.append(_valueType);

        sb.append(" s=[");//NON-NLS
        for(SeriesElement se : _series)
        {
            sb.append(se.getTimeoffset());
            sb.append(",");//NON-NLS
            sb.append(se.getValue());
        }
        sb.append("]");//NON-NLS

        return sb.toString();
    }

    public JSONObject toJson()
    {
        JSONObject rc = null;

        try
        {
            rc = new JSONObject();
            rc.put("bi", _binaryId);//NON-NLS
            rc.put("n", _name);//NON-NLS
            rc.put("ts", _timestamp);//NON-NLS
            rc.put("it", _incrementType);//NON-NLS
            rc.put("im", _incrementMultiplier);//NON-NLS
            rc.put("vt", _valueType);//NON-NLS

            if(!_series.isEmpty())
            {
                JSONArray s = new JSONArray();

                for(SeriesElement se : _series)
                {
                    s.put(se.getTimeoffset());
                    s.put(se.getValue());
                }

                rc.put("s", s);//NON-NLS
            }
        }
        catch (Exception e)
        {
            rc = null;
        }

        return rc;
    }

    public boolean parseJson(String j)
    {
        boolean rc = false;

        _series.clear();

        try
        {
            JSONObject root = new JSONObject(j);

            _binaryId = root.optInt("bi", _binaryId);//NON-NLS
            _name = root.optString("n", _name);//NON-NLS
            _timestamp = root.optInt("ts", _timestamp);//NON-NLS
            _incrementType = root.optInt("it", _incrementType);//NON-NLS
            _incrementMultiplier = root.optInt("im", _incrementMultiplier);//NON-NLS
            _valueType = root.optInt("vt", _valueType);//NON-NLS

            JSONArray s = root.optJSONArray("s");//NON-NLS
            if(s != null)
            {
                if(s.length() % 2 != 0)
                {
                    throw new Exception("Element count not divisible by 2");//NON-NLS
                }

                for(int idx = 0; idx < s.length(); idx += 2)
                {
                    addElement(s.getInt(idx), s.getInt(idx+1));
                }
            }

            rc = true;
        }
        catch (Exception e)
        {
            rc = false;
        }

        return rc;
    }

    public byte[] toByteArray()
    {
        // Our representation looks as follows
        //  1  4    1   1   1   1       11 11 11
        // |--|----|-  |-  |-  |-  |   ..|..|..|........................|
        //  id  ts  it  im  vt  ss     ov ov ov

        int sizeRequired = 0 ;

        sizeRequired += 1;                          // id
        sizeRequired += 4;                          // ts
        sizeRequired += 1;                          // it
        sizeRequired += 1;                          // im
        sizeRequired += 1;                          // vt
        sizeRequired += 1;                          // ss

        sizeRequired += (_series.size() * 2);       // series

        NetworkByteBuffer nbb = new NetworkByteBuffer();
        nbb.allocate(sizeRequired);

        nbb.putUnsignedByte(_binaryId);
        nbb.putUnsignedInt(_timestamp);
        nbb.putUnsignedByte(_incrementType);
        nbb.putUnsignedByte(_incrementMultiplier);
        nbb.putUnsignedByte(_valueType);
        nbb.putUnsignedByte(_series.size());

        if(!_series.isEmpty())
        {
            for(SeriesElement se : _series)
            {
                nbb.putUnsignedByte(se.getTimeoffset());
                nbb.putUnsignedByte(se.getValue());
            }
        }

        return nbb.array();
    }

    public int parseByteArray(byte[] ba, int ofs, int len)
    {
        int rc = 0;

        _series.clear();

        try
        {
            NetworkByteBuffer nbb = new NetworkByteBuffer();
            nbb.wrap(ba, ofs, len);

            _binaryId = (int)nbb.getUnsignedByte();
            _timestamp = (int)nbb.getUnsignedInt();
            _incrementType = (int)nbb.getUnsignedByte();
            _incrementMultiplier = (int)nbb.getUnsignedByte();
            _valueType = (int)nbb.getUnsignedByte();

            if(_valueType != VT_BYTE)
            {
                throw new Exception("Unsupported value type");
            }

            short ss = (nbb.getUnsignedByte());

            // The number of bytes extracted will be 9 (header size) + 2 bytes per sample.
            rc = (9 + (ss * 2));

            while(ss > 0)
            {
                addElement(nbb.getUnsignedByte(), nbb.getUnsignedByte());
                ss--;
            }
        }
        catch (Exception e)
        {
            rc = 0;
            e.printStackTrace();
        }

        return rc;
    }

    public void setBinaryId(int binaryId)
    {
        _binaryId = binaryId;
    }

    public int getBinaryId()
    {
        return _binaryId;
    }

    public void setName(String name)
    {
        _name = name;
    }

    public String getName()
    {
        return _name;
    }

    public int getTimestamp()
    {
        return _timestamp;
    }

    public void setTimestamp(int ts)
    {
        _timestamp = ts;
    }

    public boolean addElement(SeriesElement se)
    {
        if(_series.size() < 255)
        {
            _series.add(se);
            return true;
        }
        else
        {
            return false;
        }
    }

    public boolean addElement(int timeOffset, int value)
    {
        return addElement(new SeriesElement(timeOffset, value));
    }

    public int getElementCount()
    {
        return _series.size();
    }

    public void clearElements()
    {
        _series.clear();
    }

    public void restart()
    {
        clearElements();
        _timestamp = (int)(System.currentTimeMillis()/1000L);
    }

    public List<SeriesElement> getSeries()
    {
        return _series;
    }
}

