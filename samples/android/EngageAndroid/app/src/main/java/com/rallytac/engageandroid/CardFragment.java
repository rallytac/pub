//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.graphics.drawable.Drawable;
import android.os.Bundle;
import androidx.fragment.app.Fragment;
import androidx.core.content.ContextCompat;
import android.util.Log;
import android.view.GestureDetector;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.TextView;

public abstract class CardFragment extends Fragment
{
    private static String TAG = CardFragment.class.getSimpleName();

    protected GroupDescriptor _gd = null;
    private Animation _networkErrorAnimation = null;
    private Animation _networkFailoverAnimation = null;
    private Animation _speakerAnimation = null;
    private boolean _isEmergencyTxOn = false;

    public String getGroupId()
    {
        return ((_gd != null) ? _gd.id : null);
    }

    public GroupDescriptor getGroupDescriptor()
    {
        return _gd;
    }

    public void setGroupDescriptor(GroupDescriptor gd)
    {
        _gd = gd;
        draw();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
    {
        View view = inflater.inflate(getLayoutId(), container, false);

        ImageView iv;

        // Speaker
        iv = view.findViewById(R.id.ivSpeaker);
        iv.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                if(_gd != null)
                {
                    _gd.rxMuted = !_gd.rxMuted;

                    if(_gd.rxMuted)
                    {
                        Globals.getEngageApplication().getEngine().engageMuteGroupRx(_gd.id);
                    }
                    else
                    {
                        Globals.getEngageApplication().getEngine().engageUnmuteGroupRx(_gd.id);
                    }
                }
            }
        });
        iv.setOnLongClickListener(new View.OnLongClickListener()
        {
            @Override
            public boolean onLongClick(View v)
            {
                ((SimpleUiMainActivity) getActivity()).showVolumeSliders(_gd.id);
                return false;
            }
        });

        // PTT enable (maybe...?)
        iv = view.findViewById(R.id.ivSelectedForPtt);
        if(iv != null)
        {
            if(Globals.getEngageApplication().hasPermissionToRecordAudio())
            {
                iv.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if (_gd != null) {
                            _gd.txSelected = !_gd.txSelected;
                            updateTxEnabledStatus();
                        }
                    }
                });
            }
            else
            {
                iv.setVisibility(View.GONE);
            }
        }

        // Network error (maybe...?)
        iv = view.findViewById(R.id.ivNetError);
        if(iv != null)
        {
            iv.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    Utils.showLongPopupMsg(getActivity(), getString(R.string.experiencing_network_problems));
                }
            });
        }

        // Network failover (maybe...?)
        iv = view.findViewById(R.id.ivNetFailover);
        if(iv != null)
        {
            iv.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    Utils.showLongPopupMsg(getActivity(), getString(R.string.currently_operating_in_mc_failover_mode));
                }
            });
        }

        final TextView tvMemberCount = view.findViewById(R.id.tvMemberCount);
        if(tvMemberCount != null)
        {
            tvMemberCount.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    ((SimpleUiMainActivity) getActivity()).showTeamList(_gd.id);
                }
            });
        }

        setupGestures(view);

        return view;
    }

    @Override
    public void onResume()
    {
        super.onResume();

        // Force a redraw on resume
        draw();
    }

    // Has to be implemented by subclass
    protected abstract int getLayoutId();
    protected abstract int getCardResId(boolean secure);
    protected abstract int getCardTxResId(boolean secure);

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
                        ((TextView) getView().findViewById(R.id.tvGroupName)).setText(_gd.name);

                        updateNetworkStatus();
                        updateTalkers();
                        updateSpeakerStatus();
                        updateTxEnabledStatus();
                        updateRxTxUi();
                        updateMembers();
                    }
               }
            }
        });
    }

    private void startNetworkErrorAnimation()
    {
        if(_networkErrorAnimation == null)
        {
            _networkErrorAnimation = AnimationUtils.loadAnimation(getActivity(), R.anim.network_error_pulse);
            getView().findViewById(R.id.ivNetError).startAnimation(_networkErrorAnimation);
            getView().findViewById(R.id.ivNetError).setVisibility(View.VISIBLE);
        }
    }

    private void stopNetworkErrorAnimation()
    {
        if(_networkErrorAnimation != null)
        {
            _networkErrorAnimation.cancel();
            _networkErrorAnimation.reset();
            _networkErrorAnimation = null;
        }

        getView().findViewById(R.id.ivNetError).setVisibility(View.INVISIBLE);
        getView().findViewById(R.id.ivNetError).clearAnimation();
    }

    private void startNetworkFailoverAnimation()
    {
        if(_networkFailoverAnimation == null)
        {
            _networkFailoverAnimation = AnimationUtils.loadAnimation(getActivity(), R.anim.network_failover_pulse);
            getView().findViewById(R.id.ivNetFailover).startAnimation(_networkFailoverAnimation);
            getView().findViewById(R.id.ivNetFailover).setVisibility(View.VISIBLE);
        }
    }

    private void stopNetworkFailoverAnimation()
    {
        if(_networkFailoverAnimation != null)
        {
            _networkFailoverAnimation.cancel();
            _networkFailoverAnimation.reset();
            _networkFailoverAnimation = null;
        }

        getView().findViewById(R.id.ivNetFailover).setVisibility(View.INVISIBLE);
        getView().findViewById(R.id.ivNetFailover).clearAnimation();
    }

    private void startSpeakerErrorAnimation()
    {
        if(_speakerAnimation == null)
        {
            _speakerAnimation = AnimationUtils.loadAnimation(getActivity(), R.anim.speaker_rx_pulse);
            getView().findViewById(R.id.ivSpeaker).startAnimation(_speakerAnimation);
        }
    }

    private void stopSpeakerAnimation()
    {
        if(_speakerAnimation != null)
        {
            _speakerAnimation.cancel();
            _speakerAnimation.reset();
            _speakerAnimation = null;
        }

        getView().findViewById(R.id.ivSpeaker).clearAnimation();
    }


    private void updateNetworkStatus()
    {
        getActivity().runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_gd != null)
                {
                    if(_gd.isConnectedInSomeForm())
                    {
                        EngageApplication.GroupConnectionTrackerInfo gts = Globals.getEngageApplication().getGroupConnectionState(_gd.id);

                        stopNetworkErrorAnimation();

                        if(gts.operatingInMulticastFailover)
                        {
                            startNetworkFailoverAnimation();
                        }
                        else
                        {
                            stopNetworkFailoverAnimation();
                        }
                    }
                    else
                    {
                        startNetworkErrorAnimation();
                    }
                }
            }
        });
    }

    private void updateTalkers()
    {
        getActivity().runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_gd != null)
                {
                    TalkerFragment tf = (TalkerFragment)getChildFragmentManager().findFragmentById(R.id.fragTalkers);
                    tf.setTalkers(_gd.getTalkers());
                }
            }
        });
    }

    private void updateRxTxUi()
    {
        getActivity().runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_gd != null)
                {
                    if(_gd.rx)
                    {
                        startSpeakerErrorAnimation();
                    }
                    else
                    {
                        stopSpeakerAnimation();
                    }

                    if(_gd.tx)
                    {
                        ((ImageView)getView().findViewById(R.id.ivCard)).setImageResource(getCardTxResId(_gd.isEncrypted));
                    }
                    else
                    {
                        ((ImageView)getView().findViewById(R.id.ivCard)).setImageResource(getCardResId(_gd.isEncrypted));
                    }
                }
                else
                {
                    Log.e("CardFragment", "========================no group descriptor in updateRxTxUi");//NON-NLS
                }
            }
        });
    }

    private void updateMembers()
    {
        getActivity().runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_gd != null)
                {
                    int count = _gd.getMemberCountForStatus(Constants.GMT_STATUS_FLAG_CONNECTED);
                    //Log.e("CardFragment", "#DBG#: updateMembers, count = " + count);//NON-NLS
                    ((TextView) getView().findViewById(R.id.tvMemberCount)).setText(Integer.toString(count));
                }
            }
        });
    }

    private void updateSpeakerStatus()
    {
        getActivity().runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_gd != null)
                {
                    if(_gd.rxMuted)
                    {
                        ((ImageView)getView().findViewById(R.id.ivSpeaker)).setImageDrawable(ContextCompat.getDrawable(getActivity(), R.drawable.ic_speaker_off));
                    }
                    else
                    {
                        ((ImageView)getView().findViewById(R.id.ivSpeaker)).setImageDrawable(ContextCompat.getDrawable(getActivity(), R.drawable.ic_speaker_on));
                    }
                }
            }
        });
    }

    private void updateTxEnabledStatus()
    {
        getActivity().runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                if(_gd != null)
                {
                    ImageView iv = getView().findViewById(R.id.ivSelectedForPtt);

                    if(iv != null)
                    {
                        if(_gd.txSelected)
                        {
                            iv.setImageDrawable(ContextCompat.getDrawable(getActivity(), R.drawable.ic_check_box_checked));
                        }
                        else
                        {
                            iv.setImageDrawable(ContextCompat.getDrawable(getActivity(), R.drawable.ic_check_box_unchecked));
                        }

                        ((SimpleUiMainActivity) getActivity()).redrawPttButton();
                    }
                }
            }
        });
    }

    protected enum GestureDirection {gdNone, gdUp, gdDown, gdLeft, gdRight}

    // Has to be implemented by subclass
    protected abstract void onCardDoubleTap();
    protected abstract void onCardSwiped(GestureDirection direction);

    private class CardGestureListener extends GestureDetector.SimpleOnGestureListener
    {
        @Override
        public boolean onDoubleTap(MotionEvent e)
        {
            onCardDoubleTap();
            return true;
        }

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float vX, float vY)
        {
            if(e1 == null || e2 == null)
            {
                return true;
            }

            ImageView ivCard = getView().findViewById(R.id.ivCard);

            float dX = (e2.getX() - e1.getX());
            float dY = (e2.getY() - e1.getY());
            float absX = Math.abs(dX);
            float absY = Math.abs(dY);
            int w = ivCard.getWidth();
            int h = ivCard.getHeight();
            float aX = (w * (float)(30.0 / 100));
            float aY = (h * (float)(30.0 / 100));
            GestureDirection direction = GestureDirection.gdNone;

            if(absX >= aX)
            {
                if(dX < 0)
                {
                    direction = GestureDirection.gdLeft;
                }
                else
                {
                    direction = GestureDirection.gdRight;
                }
            }
            else if(absY >= aY)
            {
                if(dY < 0)
                {
                    direction = GestureDirection.gdUp;
                }
                else
                {
                    direction = GestureDirection.gdDown;
                }
            }

            if(direction == GestureDirection.gdNone)
            {
                return true;
            }

            onCardSwiped(direction);

            return true;
        }
    }

    private GestureDetector _gestureDetector;

    private void setupGestures(View view)
    {
        if(!Utils.boolOpt(getString(R.string.opt_allow_multiple_channel_view), true))
        {
            return;
        }

        _gestureDetector = new GestureDetector(getActivity(), new CardGestureListener());
        ImageView ivCard = view.findViewById(R.id.ivCard);

        ivCard.setOnTouchListener(new View.OnTouchListener()
        {
            @Override
            public boolean onTouch(View v, MotionEvent event)
            {
                _gestureDetector.onTouchEvent(event);
                return true;
            }
        });
    }
}
