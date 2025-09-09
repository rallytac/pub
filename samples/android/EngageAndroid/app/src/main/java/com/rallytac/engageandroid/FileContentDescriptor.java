//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

public class FileContentDescriptor
{
    public enum Type
    {
        fctUnknown,
        fctJson,
        fctJsonEncrypted,
        fctText,
        fctPicture
    };

    FileContentDescriptor(Type type, Object data)
    {
        _type = type;
        _data = data;
    }

    public Object getData()
    {
        return _data;
    }

    private Type _type = Type.fctUnknown;
    private Object _data = null;
}
