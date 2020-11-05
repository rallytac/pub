//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//
// With thanks to Ron Hitchens (ron@ronsoft.com)
//

package com.rallytac.engageandroid;

import java.nio.ByteBuffer;

public class NetworkByteBuffer
{
    private ByteBuffer _bb;

    public void allocate(int capacity)
    {
        _bb = ByteBuffer.allocate(capacity);
    }

    public void wrap(byte[] a)
    {
        _bb = ByteBuffer.wrap(a);
    }

    public void wrap(byte[] a, int ofs, int len)
    {
        _bb = ByteBuffer.wrap(a, ofs, len);
    }

    public byte[] array()
    {
        return _bb.array();
    }

    public short getUnsignedByte()
    {
        return ((short) (_bb.get() & 0xff));
    }

    public void putUnsignedByte(int value)
    {
        _bb.put((byte) (value & 0xff));
    }

    public short getUnsignedByte(int position)
    {
        return ((short) (_bb.get(position) & (short) 0xff));
    }

    public void putUnsignedByte(int position, int value)
    {
        _bb.put(position, (byte) (value & 0xff));
    }

    // ---------------------------------------------------------------

    public int getUnsignedShort()
    {
        return (_bb.getShort() & 0xffff);
    }

    public void putUnsignedShort(int value)
    {
        _bb.putShort((short) (value & 0xffff));
    }

    public int getUnsignedShort(int position)
    {
        return (_bb.getShort(position) & 0xffff);
    }

    public void putUnsignedShort(int position, int value)
    {
        _bb.putShort(position, (short) (value & 0xffff));
    }

    // ---------------------------------------------------------------

    public long getUnsignedInt()
    {
        return ((long) _bb.getInt() & 0xffffffffL);
    }

    public void putUnsignedInt(long value)
    {
        _bb.putInt((int) (value & 0xffffffffL));
    }

    public long getUnsignedInt(int position)
    {
        return ((long) _bb.getInt(position) & 0xffffffffL);
    }

    public void putUnsignedInt(int position, long value)
    {
        _bb.putInt(position, (int) (value & 0xffffffffL));
    }
}
