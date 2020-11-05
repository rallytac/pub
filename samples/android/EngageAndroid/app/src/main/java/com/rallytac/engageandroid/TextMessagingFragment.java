//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.text.InputFilter;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

import androidx.fragment.app.Fragment;

import com.rallytac.engage.engine.Engine;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Calendar;

public class TextMessagingFragment extends Fragment
{
    private static String TAG = TextMessagingFragment.class.getSimpleName();

    protected GroupDescriptor _gd = null;

    private ListView _lvMessages;
    private MessageAdapter _adapter;
    private EditText _etTextMessage = null;
    private ArrayList<TextMessage> _messageList = new ArrayList<>();

    public void setGroupDescriptor(GroupDescriptor gd)
    {
        _gd = gd;
        _messageList = Globals.getEngageApplication().getTextMessagesForGroup(_gd.id);

        if(_messageList == null)
        {
            _messageList = new ArrayList<>();
        }

        draw();

        _lvMessages.setSelection(_lvMessages.getCount() - 1);
    }

    private class MessageViewHolder
    {
        public View avatar;
        public TextView timestamp;
        public TextView name;
        public TextView messageBody;
    }

    private class MessageAdapter extends BaseAdapter
    {
        Context context;

        public MessageAdapter(Context context) {
            this.context = context;
        }

        /*
        public void add(TextMessage message)
        {
            this.messages.add(message);
            notifyDataSetChanged();
        }
        */

        @Override
        public int getCount()
        {
            return _messageList.size();
        }

        @Override
        public Object getItem(int i)
        {
            return _messageList.get(i);
        }

        @Override
        public long getItemId(int i)
        {
            return i;
        }

        @Override
        public View getView(int i, View convertView, ViewGroup viewGroup)
        {
            MessageViewHolder holder = new MessageViewHolder();
            LayoutInflater messageInflater = (LayoutInflater) context.getSystemService(Activity.LAYOUT_INFLATER_SERVICE);
            TextMessage message = _messageList.get(i);

            if (message._direction == TextMessage.Direction.sent)
            {
                convertView = messageInflater.inflate(R.layout.sent_message, null);

                holder.timestamp = convertView.findViewById(R.id.timestamp);
                holder.messageBody = convertView.findViewById(R.id.message_body);

                convertView.setTag(holder);
                holder.messageBody.setText(message._messageText);
                holder.timestamp.setText(message._ts.toString());
            }
            else
            {
                convertView = messageInflater.inflate(R.layout.received_message, null);

                holder.timestamp = convertView.findViewById(R.id.timestamp);
                holder.avatar = convertView.findViewById(R.id.avatar);
                holder.name = convertView.findViewById(R.id.name);
                holder.messageBody = convertView.findViewById(R.id.message_body);

                convertView.setTag(holder);

                holder.name.setText(message._sourceDisplayName);
                holder.messageBody.setText(message._messageText);
                holder.timestamp.setText(message._ts.toString());
            }

            return convertView;
        }

    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
    {
        View view = inflater.inflate(R.layout.fragment_text_messaging, container, false);

        _adapter = new MessageAdapter(view.getContext());

        _etTextMessage = view.findViewById(R.id.etTextMessage);

        int maxLength = Constants.MAX_TEXT_MESSAGE_INPUT_SIZE;
        _etTextMessage.setFilters(new InputFilter[] {new InputFilter.LengthFilter(maxLength)});

        _lvMessages = view.findViewById(R.id.lvTextMessages);
        _lvMessages.setAdapter(_adapter);

        return view;
    }

    @Override
    public void onResume()
    {
        super.onResume();

        // Force a redraw on resume
        draw();
    }

    public void draw()
    {
        getActivity().runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_gd != null)
                {
                    if(getView() != null)
                    {
                    }
                }
            }
        });
    }

    public void sendEnteredTextIfAny()
    {
        try
        {
            String msg = _etTextMessage.getText().toString();
            msg.trim();

            _etTextMessage.setText(null);
            if(!Utils.isEmptyString(msg))
            {
                byte[] msgBlobBytes = msg.getBytes();

                JSONObject bi = new JSONObject();

                bi.put(Engine.JsonFields.BlobInfo.blobSize, msgBlobBytes.length);
                bi.put(Engine.JsonFields.BlobInfo.source, "");   // Let the Engine fill in the node ID
                bi.put(Engine.JsonFields.BlobInfo.target, "");   // We're sending to everyone
                bi.put(Engine.JsonFields.BlobInfo.payloadType, Engine.BlobType.appTextUtf8.toInt());

                // If we're sending on an audio group then the blob will be wrapped in RTP, therefore
                // we need to include an RtpHeader object with, at least, a payload type defined
                if(_gd.type == GroupDescriptor.Type.gtAudio)
                {
                    JSONObject rtpHeader = new JSONObject();
                    rtpHeader.put(Engine.JsonFields.RtpHeader.pt, Constants.TEXT_MESSAGE_BLOB_RTP_PAYLOAD_TYPE);
                    bi.put(Engine.JsonFields.RtpHeader.objectName, rtpHeader);
                }

                String jsonParams = bi.toString();
                Globals.getEngageApplication().getEngine().engageSendGroupBlob(_gd.id, msgBlobBytes, msgBlobBytes.length, jsonParams);

                TextMessage tm = new TextMessage();
                tm._ts = Calendar.getInstance().getTime();
                tm._direction = TextMessage.Direction.sent;
                tm._groupId = _gd.id;
                tm._messageText = msg;
                tm._sourceNodeId = Globals.getEngageApplication().getActiveConfiguration().getNodeId();
                Globals.getEngageApplication().addTextMessage(tm);

                appendMessage(tm);
            }
        }
        catch (Exception e)
        {
        }
    }

    public void appendMessage(TextMessage tm)
    {
        _messageList.add(tm);
        _adapter.notifyDataSetChanged();
        _lvMessages.setSelection(_lvMessages.getCount() - 1);
    }

    public void onTextMessageReceived(final PresenceDescriptor sourcePd, final String msg)
    {
        TextMessage tm = new TextMessage();
        tm._ts = Calendar.getInstance().getTime();
        tm._direction = TextMessage.Direction.received;
        tm._groupId = _gd.id;
        tm._messageText = msg;
        tm._sourceNodeId = sourcePd.nodeId;
        if(!Utils.isEmptyString(sourcePd.displayName))
        {
            tm._sourceDisplayName = sourcePd.displayName;
        }
        else if(!Utils.isEmptyString(sourcePd.userId))
        {
            tm._sourceDisplayName = sourcePd.userId;
        }
        else if(!Utils.isEmptyString(sourcePd.userId))
        {
            tm._sourceDisplayName = sourcePd.nodeId;
        }
        Globals.getEngageApplication().addTextMessage(tm);

        appendMessage(tm);
    }
}
