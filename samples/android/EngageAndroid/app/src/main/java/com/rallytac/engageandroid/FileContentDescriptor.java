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

    FileContentDescriptor(FileContentDescriptor.Type type, Object data)
    {
        _type = type;
        _data = data;
    }

    public Object getData()
    {
        return _data;
    }

    private FileContentDescriptor.Type _type = FileContentDescriptor.Type.fctUnknown;
    private Object _data = null;
}
