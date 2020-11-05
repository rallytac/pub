//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//
//  With thanks to Antonio Gutierrez <agutierrez88s@gmail.com>
//

package com.rallytac.engageandroid;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Parcelable;
import android.text.Html;
import android.text.TextUtils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class ShareHelper {

    private static final String SHARE_TXT_HTML = "text/html"; //NON-NLS
    private static final String SHARE_TXT_PLAIN = "text/plain"; //NON-NLS

    private static final String TWITTER_PACKAGE = "com.twitter.android"; //NON-NLS

    private ShareHelper() { }

    private static boolean isAppAvailable(final Context context, final String appPackageName) {
        try {
            ApplicationInfo info = context.getPackageManager().getApplicationInfo(appPackageName, 0);
            return info != null;
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        }
    }

    private static List<ResolveInfo> getAppsSupportingIntentType(final Context context, final String type) {
        final Intent shareIntent = new Intent(Intent.ACTION_SEND);
        shareIntent.setType(type);

        return context.getPackageManager().queryIntentActivities(shareIntent, 0);
    }

    private static List<Intent> buildListIntents(final List<ResolveInfo> resInfo, final Intent baseIntent,
                                                 final Set<String> selectedPackageNames) {

        List<Intent> targetedShareIntents = new ArrayList<>();
        for (ResolveInfo resolveInfo : resInfo) {
            final String packageName = resolveInfo.activityInfo.packageName;
            if (selectedPackageNames.contains(packageName)) {
                continue;
            }

            final Intent targetedShareIntent = (Intent) baseIntent.clone();
            targetedShareIntent.setPackage(packageName);

            targetedShareIntents.add(targetedShareIntent);
            selectedPackageNames.add(packageName);
        }

        return targetedShareIntents;
    }

    private static List<Intent> buildIntentsForAppsSupportingType(final Activity context, final String intentType,
                                                                  final Intent targetedShareIntent, final Set<String> selectedPackageNames) {

        // Query app supporting Intent type
        List<ResolveInfo> resInfo = getAppsSupportingIntentType(context, intentType);
        if (resInfo.size() == 0) {
            return Collections.emptyList();
        }

        return buildListIntents(resInfo, targetedShareIntent, selectedPackageNames);
    }

    private static List<Intent> buildIntentsForAppsSupportingPlainText(final Activity context, final ShareableData data,
                                                                       final Set<String> selectedPackageNames) {
        if (TextUtils.isEmpty(data.getUrl()) || TextUtils.isEmpty(data.getText())) {
            return Collections.emptyList();
        }

        // Build base intent
        final Intent targetedShareIntent = new Intent(Intent.ACTION_SEND);
        targetedShareIntent.setType(SHARE_TXT_PLAIN);
        targetedShareIntent.putExtra(Intent.EXTRA_TEXT, data.getText() + (!Utils.isEmptyString(data.getUrl()) ? "\n" + data.getUrl() : ""));
        targetedShareIntent.putExtra(Intent.EXTRA_SUBJECT, data.getSubject());

        if(!data.getUris().isEmpty())
        {
            targetedShareIntent.putExtra(Intent.EXTRA_STREAM, data.getUris().get(0));
            //targetedShareIntent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, data.getUris());
        }

        return buildIntentsForAppsSupportingType(context, SHARE_TXT_PLAIN, targetedShareIntent, selectedPackageNames);

    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    private static List<Intent> buildIntentsForAppsSupportingHtml(final Activity context, final ShareableData data,
                                                                  final Set<String> selectedPackageNames) {

        if (TextUtils.isEmpty(data.getHtml())) {
            return Collections.emptyList();
        }

        // Build base intent
        final Intent targetedShareIntent = new Intent(Intent.ACTION_SEND);
        targetedShareIntent.setType(SHARE_TXT_HTML);

        targetedShareIntent.putExtra(Intent.EXTRA_TEXT, Html.fromHtml(data.getHtml()));
        targetedShareIntent.putExtra(Intent.EXTRA_SUBJECT, data.getSubject());
        if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            targetedShareIntent.putExtra(Intent.EXTRA_HTML_TEXT, data.getHtml());
        }

        if(!data.getUris().isEmpty())
        {
            targetedShareIntent.putExtra(Intent.EXTRA_STREAM, data.getUris().get(0));
            //targetedShareIntent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, data.getUris());
        }

        return buildIntentsForAppsSupportingType(context, SHARE_TXT_HTML, targetedShareIntent, selectedPackageNames);
    }

    public static Intent buildTwitterShareIntent(final ShareableData data) {
        final Intent targetedShareIntent = new Intent(Intent.ACTION_SEND);
        targetedShareIntent.setType(SHARE_TXT_PLAIN);
        targetedShareIntent.putExtra(Intent.EXTRA_TEXT,
                data.getText() + (!Utils.isEmptyString(data.getUrl()) ? "\n" + data.getUrl() : ""));
        targetedShareIntent.setPackage(TWITTER_PACKAGE);

        if(!data.getUris().isEmpty())
        {
            targetedShareIntent.putExtra(Intent.EXTRA_STREAM, data.getUris().get(0));
            //targetedShareIntent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, data.getUris());
        }

        return targetedShareIntent;

    }

    public static Intent buildShareIntent(final Activity context, final ShareableData data,
                                          final String sharingHeader) {
        final Set<String> selectedPackageNames = new HashSet<>();
        final List<Intent> targetedShareIntents = new ArrayList<>();

        // Special handling for Twitter App
        if (isAppAvailable(context, TWITTER_PACKAGE)) {

            targetedShareIntents.add(buildTwitterShareIntent(data));
            selectedPackageNames.add(TWITTER_PACKAGE);
        }

        // App supporting HTML
        targetedShareIntents.addAll(buildIntentsForAppsSupportingHtml(context, data, selectedPackageNames));

        // App supporting simple text
        targetedShareIntents.addAll(buildIntentsForAppsSupportingPlainText(context, data, selectedPackageNames));

        Intent chooserIntent = Intent.createChooser(new Intent(Intent.ACTION_SEND), sharingHeader);
        if (targetedShareIntents.size() > 0) {

            // Grab first one so the chooser can match something and add rest as extra param
            // Intent added when calling Intent#createChooser will be added at the end.
            chooserIntent = Intent.createChooser(targetedShareIntents.remove(0), sharingHeader);
            chooserIntent.putExtra(Intent.EXTRA_INITIAL_INTENTS,
                    targetedShareIntents.toArray(new Parcelable[targetedShareIntents.size()]));

        }

        return chooserIntent;
    }
}
