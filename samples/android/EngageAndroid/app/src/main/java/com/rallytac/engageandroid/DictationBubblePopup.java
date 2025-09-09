//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//
package com.rallytac.engageandroid;

import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;

public class DictationBubblePopup {

    private Context context;
    private PopupWindow popupWindow;
    private View popupView;
    private TextView messageTextView;
    private LinearLayout bubbleLayout;

    public DictationBubblePopup(Context context) {
        this.context = context;
        init();
    }

    private void init() {
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        popupView = inflater.inflate(R.layout.dictation_bubble_popup, null);

        messageTextView = popupView.findViewById(R.id.messageTextView);
        bubbleLayout = popupView.findViewById(R.id.bubbleLayout);

        popupWindow = new PopupWindow(popupView, ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, true);
        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        popupWindow.setOutsideTouchable(true);
    }

    public void setMessage(String message) {
        messageTextView.setText(message);
    }

    public void setBubbleColor(int color) {
        /*
        GradientDrawable drawable = (GradientDrawable) bubbleLayout.getBackground();
        drawable.setColor(color);
         */
    }

    public void show(View anchorView, int xOffset, int yOffset) {
        int[] location = new int[2];
        anchorView.getLocationOnScreen(location);

        int screenHeight = this.context.getResources().getDisplayMetrics().heightPixels;
        int bubbleHeight = (int) (screenHeight * 0.5);

        int screenWidth = this.context.getResources().getDisplayMetrics().widthPixels;
        int bubbleWidth = (int) (screenWidth * 0.8);

        popupWindow.setHeight(bubbleHeight);
        popupWindow.setWidth(bubbleWidth);

        int popupX = (screenWidth - bubbleWidth) / 2;
        int popupY = (screenHeight - bubbleHeight) / 2;

        popupWindow.showAtLocation(anchorView, Gravity.NO_GRAVITY, popupX, popupY);
    }

    public void dismiss() {
        popupWindow.dismiss();
    }
}