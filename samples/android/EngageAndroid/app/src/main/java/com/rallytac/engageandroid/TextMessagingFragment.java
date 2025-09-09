//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.text.InputFilter;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.fragment.app.Fragment;

import com.rallytac.engage.engine.Engine;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Locale;

public class TextMessagingFragment extends Fragment
{
    private static String TAG = TextMessagingFragment.class.getSimpleName();

    protected GroupDescriptor _gd = null;

    private ListView _lvMessages;
    private MessageAdapter _adapter;
    private EditText _etTextMessage = null;
    private ArrayList<TextMessage> _messageList = new ArrayList<>();
    private ImageView _ivDictateTextMessage = null;

    private SpeechRecognizer _speechRecognizer;
    private boolean _isListening = false;
    private String _provisionalText = "";
    private DictationBubblePopup _dictationBubblePopup = null;


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

    private String getSpeechRecognizerErrorMessage(int errorCode)
    {
        switch (errorCode)
        {
            case SpeechRecognizer.ERROR_NETWORK_TIMEOUT:
                return "Network timeout";
            case SpeechRecognizer.ERROR_NETWORK:
                return "Network error";
            case SpeechRecognizer.ERROR_AUDIO:
                return "Audio recording error";
            case SpeechRecognizer.ERROR_SERVER:
                return "Server error";
            case SpeechRecognizer.ERROR_CLIENT:
                return "Client-side error";
            case SpeechRecognizer.ERROR_SPEECH_TIMEOUT:
                return "No speech input detected";
            case SpeechRecognizer.ERROR_NO_MATCH:
                return "No recognition match";
            case SpeechRecognizer.ERROR_RECOGNIZER_BUSY:
                return "Speech recognizer is busy";
            case SpeechRecognizer.ERROR_INSUFFICIENT_PERMISSIONS:
                return "Insufficient permissions";
            case SpeechRecognizer.ERROR_TOO_MANY_REQUESTS:
                return "Too many requests";
            case SpeechRecognizer.ERROR_SERVER_DISCONNECTED:
                return "Server disconnected";
            case SpeechRecognizer.ERROR_LANGUAGE_NOT_SUPPORTED:
                return "Language not supported";
            case SpeechRecognizer.ERROR_LANGUAGE_UNAVAILABLE:
                return "Language unavailable";
            case SpeechRecognizer.ERROR_CANNOT_CHECK_SUPPORT:
                return "Cannot check support";
            case SpeechRecognizer.ERROR_CANNOT_LISTEN_TO_DOWNLOAD_EVENTS:
                return "Cannot listen to download events";
            default:
                return "Unknown error " + errorCode;
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

        _speechRecognizer = SpeechRecognizer.createSpeechRecognizer(requireContext());
        final Intent speechIntent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        speechIntent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
        speechIntent.putExtra(RecognizerIntent.EXTRA_LANGUAGE, Locale.getDefault());
        speechIntent.putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, true);

        _speechRecognizer.setRecognitionListener(new RecognitionListener() {
            @Override
            public void onReadyForSpeech(Bundle params) {
                showSpeechBubble(_ivDictateTextMessage);
                _provisionalText = "";
                updateSpeechBubble("Listening...");
            }

            @Override
            public void onBeginningOfSpeech() {
                _provisionalText = "";
            }

            @Override
            public void onRmsChanged(float rmsdB) {}

            @Override
            public void onBufferReceived(byte[] buffer) {}

            @Override
            public void onEndOfSpeech() {
                dismissSpeechBubble();
                _etTextMessage.setText(_provisionalText);
            }

            @Override
            public void onError(int error) {
                dismissSpeechBubble();
                Toast.makeText(requireContext(), getSpeechRecognizerErrorMessage(error), Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onResults(Bundle results) {
                ArrayList<String> matches = results.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
                if (matches != null && !matches.isEmpty()) {
                    _etTextMessage.setText(matches.get(0));
                }
            }

            @Override
            public void onPartialResults(Bundle partialResults) {
                ArrayList<String> partialData = partialResults.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
                if (partialData != null && !partialData.isEmpty()) {
                    _provisionalText = partialData.get(0);
                    updateSpeechBubble(_provisionalText + "...");
                }
            }

            @Override
            public void onEvent(int eventType, Bundle params) {}
        });

        _ivDictateTextMessage = view.findViewById(R.id.ivDictateTextMessage);
        _ivDictateTextMessage.setOnTouchListener((v, event) -> {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    if (!_isListening) {
                        _speechRecognizer.startListening(speechIntent);
                        _isListening = true;
                    }
                    return true;
                case MotionEvent.ACTION_UP:
                    _speechRecognizer.stopListening();
                    _isListening = false;
                    dismissSpeechBubble();
                    return true;
            }
            return false;

        });

        return view;
    }

    private void showSpeechBubble(View anchorView) {
        if (_dictationBubblePopup == null) {
            _dictationBubblePopup = new DictationBubblePopup(requireContext());
            _dictationBubblePopup.setMessage("...");
            _dictationBubblePopup.setBubbleColor(Color.GREEN);

            _dictationBubblePopup.show(anchorView, 150, -20);
        }
    }

    private void updateSpeechBubble(String text) {
        if (_dictationBubblePopup != null) {
            _dictationBubblePopup.setMessage(text);
        }
    }

    private void dismissSpeechBubble() {
        if (_dictationBubblePopup != null) {
            _dictationBubblePopup.dismiss();
            _dictationBubblePopup = null;
        }
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
