//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//
//  With thanks to Antonio Gutierrez <agutierrez88s@gmail.com>
//

package com.rallytac.engageandroid;

import android.net.Uri;

import java.util.ArrayList;

public class ShareableData
{
    private String subject;
    private String html;
    private String text;
    private String url;
    private String twitterAccount;
    private ArrayList<Uri> uris = new ArrayList<>();

    public ShareableData() { }

    public String getSubject() {
        return subject;
    }

    public void setSubject(final String subject) {
        this.subject = subject;
    }

    public String getHtml() {
        return html;
    }

    public void setHtml(final String html) {
        this.html = html;
    }

    public String getText() {
        return text;
    }

    public void setText(final String text) {
        this.text = text;
    }

    public String getUrl() {
        return url;
    }

    public void setUrl(final String url) {
        this.url = url;
    }

    public ArrayList<Uri> getUris() {
        return uris;
    }

    public void addUri(final Uri uri)
    {
        uris.add(uri);
    }

    public String getTwitterAccount() {
        return twitterAccount;
    }

    public void setTwitterAccount(final String twitterAccount) {
        this.twitterAccount = twitterAccount;
    }
}
