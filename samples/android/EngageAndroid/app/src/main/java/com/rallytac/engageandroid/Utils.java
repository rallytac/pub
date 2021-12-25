//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.location.Location;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.text.format.DateUtils;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.BinaryBitmap;
import com.google.zxing.DecodeHintType;
import com.google.zxing.MultiFormatReader;
import com.google.zxing.MultiFormatWriter;
import com.google.zxing.RGBLuminanceSource;
import com.google.zxing.Result;
import com.google.zxing.common.BitMatrix;
import com.google.zxing.common.HybridBinarizer;
import com.journeyapps.barcodescanner.BarcodeEncoder;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.nio.channels.FileChannel;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.EnumMap;
import java.util.EnumSet;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.Set;
import java.util.UUID;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;


public class Utils
{
    private static String TAG = Utils.class.getSimpleName();

    public FileContentDescriptor getContentDescriptor(byte[] data)
    {
        // Maybe clear-text JSON?
        try
        {
            String plainText = new String(data, StandardCharsets.UTF_8);
            if(!Utils.isEmptyString(plainText))
            {
                JSONObject jo = new JSONObject(plainText);
                return new FileContentDescriptor(FileContentDescriptor.Type.fctJson, jo);
            }
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }

        // TODO: .....

        return null;
    }

    public FileContentDescriptor getContentDescriptor(Uri uri)
    {
        return getContentDescriptor(Utils.readBinaryFile(Globals.getContext(), uri));

        /*
        FileContentDescriptor rc = null;

        // First, let's see if this a JSON text file
        try
        {
            String plainText = Utils.readTextFile(Globals.getEngageApplication().getApplicationContext(), uri);
            if(!Utils.isEmptyString(plainText))
            {
                JSONObject jo = new JSONObject(plainText);
                rc = new FileContentDescriptor(FileContentDescriptor.Type.fctJson, jo);
            }
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }

        try
        {
            BufferedInputStream is = new BufferedInputStream(getContentResolver().openInputStream(uri));
            Bitmap bm = BitmapFactory.decodeStream(is);
            String dataString = Utils.qrCodeBitmapToString(bm);
            Globals.getLogger().e(TAG, "dataString=" + dataString);

            //if(dataString.startsWith(Constants.QR_CODE_HEADER))
            {
                byte[] base91DecodedBytes = Base91.decode(dataString.getBytes(Utils.getEngageCharSet()));
                byte[] decompressed = Utils.inflate(base91DecodedBytes);
                dataString = new String(decompressed, Utils.getEngageCharSet());
                Globals.getLogger().e(TAG, "dataString=" + dataString);
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

        //byte[] base91DecodedBytes = Base91.decode(encryptedString.getBytes(Utils.getEngageCharSet()));

        //qrCodeBitmapToString

        return rc;
        */
    }

    public static boolean deleteDirectory(String dirName)
    {
        try
        {
            File directoryToBeDeleted = new File(dirName);
            File[] allContents = directoryToBeDeleted.listFiles();
            if (allContents != null)
            {
                for (File file : allContents)
                {
                    deleteDirectory(file.getAbsolutePath());
                }
            }

            return directoryToBeDeleted.delete();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

        return false;
    }

    public static void copyFileOrDirectory(String srcDir, String dstDir)
    {
        try
        {
            File src = new File(srcDir);
            File dst = new File(dstDir, src.getName());

            if (src.isDirectory())
            {
                String[] files = src.list();
                int filesLength = files.length;
                for (int i = 0; i < filesLength; i++)
                {
                    String src1 = (new File(src, files[i]).getPath());
                    String dst1 = dst.getPath();
                    copyFileOrDirectory(src1, dst1);
                }
            }
            else
            {
                copyFile(src, dst);
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public static void copyFile(File sourceFile, File destFile) throws IOException
    {
        if (!destFile.getParentFile().exists())
        {
            destFile.getParentFile().mkdirs();
        }

        if (!destFile.exists())
        {
            destFile.createNewFile();
        }

        FileChannel source = null;
        FileChannel destination = null;

        try
        {
            source = new FileInputStream(sourceFile).getChannel();
            destination = new FileOutputStream(destFile).getChannel();
            destination.transferFrom(source, 0, source.size());
        }
        finally
        {
            if (source != null)
            {
                source.close();
            }

            if (destination != null)
            {
                destination.close();
            }
        }
    }

    public static Charset getEngageCharSet()
    {
        return Charset.forName(Constants.CHARSET);
    }

    public static String emptyAs(String s, String v)
    {
        if(!isEmptyString(s))
        {
            return s;
        }
        else
        {
            return v;
        }
    }

    public static boolean isEmptyString(String s)
    {
        return (s == null || s.isEmpty());
    }

    public static String trimString(String s)
    {
        if(isEmptyString(s))
        {
            return "";
        }

        return s.trim();
    }

    public static NetworkInterface getFirstViableMulticastNetworkInterface()
    {
        NetworkInterface rc = null;

        try
        {
            // Try for the default WiFi NIC
            rc = getNetworkInterfaceByName(Constants.DEF_BINDING_NIC_NAME);

            // Make sure it's not some weird name that is loopback!
            if(rc != null && rc.isLoopback())
            {
                rc = null;
            }

            // If we didn't find it, loop through seeing what we can find
            if(rc == null)
            {
                for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();)
                {
                    NetworkInterface ni = en.nextElement();

                    // We're not interested in loopbacks
                    if(ni.isLoopback())
                    {
                        continue;
                    }

                    if(ni.isUp() && ni.supportsMulticast())
                    {
                        rc = ni;
                        break;
                    }
                }
            }
        }
        catch(Exception e)
        {
            rc = null;
        }

        return rc;
    }

    public static NetworkInterface getNetworkInterfaceByName(String nm)
    {
        NetworkInterface rc = null;

        try
        {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();)
            {
                NetworkInterface ni = en.nextElement();
                if(ni.getName().compareTo(nm) == 0)
                {
                    rc = ni;
                    break;
                }
            }
        }
        catch(Exception e)
        {
            rc = null;
        }

        return rc;
    }


    public static ArrayList<NetworkInterface> getNetworkInterfaces()
    {
        ArrayList<NetworkInterface> rc = new ArrayList<>();

        try
        {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();)
            {
                rc.add(en.nextElement());
            }
        }
        catch(Exception e)
        {
            rc = null;
        }

        return rc;
    }

    public static byte[] getBinaryResource(Context ctx, int id)
    {
        byte[] rc = null;

        try
        {
            Resources res = ctx.getResources();
            InputStream in_s = res.openRawResource(id);

            rc = new byte[in_s.available()];
            in_s.read(rc);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            rc = null;
        }

        return rc;
    }

    public static String getStringResource(Context ctx, int id)
    {
        String rc = null;

        try {
            Resources res = ctx.getResources();
            InputStream in_s = res.openRawResource(id);

            byte[] b = new byte[in_s.available()];
            in_s.read(b);
            rc = new String(b);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

        return rc;
    }

    /*
    public static void generateSampleMission(Context ctx)
    {
        ActiveConfiguration ac = new ActiveConfiguration();
        ac.parseTemplate(getStringResource(ctx, R.raw.sample_mission_template));
        String tmp = ac.makeTemplate().toString();

        Globals.getSharedPreferencesEditor().putString(PreferenceKeys.ACTIVE_MISSION_CONFIGURATION_JSON, tmp);
        Globals.getSharedPreferencesEditor().apply();
    }
    */

    public static boolean isValidMulticastAddress(String addr)
    {
        boolean rc;

        try
        {
            InetAddress ia = InetAddress.getByName(addr);
            rc = ia.isMulticastAddress();
        }
        catch(Exception e)
        {
            rc = false;
        }

        return rc;
    }

    public static boolean isValidPort(int port)
    {
        return (port >= Constants.MIN_IP_PORT && port <= Constants.MAX_IP_PORT);
    }

    public static boolean isValidMulticastAndPort(String addr, String port)
    {
        boolean rc;

        try
        {
            rc = isValidMulticastAndPort(addr, Integer.parseInt(port));
        }
        catch(Exception e)
        {
            rc = false;
        }

        return rc;
    }

    public static boolean isValidMulticastAndPort(String addr, int port)
    {
        boolean rc;

        try
        {
            rc = isValidMulticastAddress(addr) && isValidPort(port);
        }
        catch(Exception e)
        {
            rc = false;
        }

        return rc;
    }

    public static void saveSelectedGroups(ActiveConfiguration ac)
    {
        HashSet<String> selectedGroups = new HashSet<>();
        String singleViewId = null;

        for(GroupDescriptor gd : ac.getMissionGroups())
        {
            if(gd.selectedForMultiView)
            {
                selectedGroups.add(gd.id);
            }

            if(gd.selectedForSingleView)
            {
                singleViewId = gd.id;
            }
        }

        Globals.getSharedPreferencesEditor().putString(PreferenceKeys.ACTIVE_MISSION_CONFIGURATION_SELECTED_GROUPS_SINGLE, singleViewId);
        Globals.getSharedPreferencesEditor().putStringSet(PreferenceKeys.ACTIVE_MISSION_CONFIGURATION_SELECTED_GROUPS_MULTI, selectedGroups);
        Globals.getSharedPreferencesEditor().apply();
    }

    public static void selectPreviouslySavedSelectedGroups(ActiveConfiguration ac)
    {
        Set<String> defaultSet = new HashSet<>();
        HashSet<String> selectedMultiViewGroups = (HashSet) Globals.getSharedPreferences().getStringSet(PreferenceKeys.ACTIVE_MISSION_CONFIGURATION_SELECTED_GROUPS_MULTI, defaultSet);
        String singleViewId = Globals.getSharedPreferences().getString(PreferenceKeys.ACTIVE_MISSION_CONFIGURATION_SELECTED_GROUPS_SINGLE, null);

        // Let's assume none are selected
        for(GroupDescriptor gd : ac.getMissionGroups())
        {
            gd.selectedForSingleView = false;
            gd.selectedForMultiView = false;
        }

        // Take care of the one selected for single view, but first see if we even have one
        if(isEmptyString(singleViewId))
        {
            for(GroupDescriptor gd : ac.getMissionGroups())
            {
                if(gd.type == GroupDescriptor.Type.gtAudio)
                {
                    singleViewId = gd.id;
                    break;
                }
            }
        }

        // Next, see if we can find the group that singleViewId points to
        boolean foundIt = false;
        for(GroupDescriptor gd : ac.getMissionGroups())
        {
            if(gd.id.compareTo(singleViewId) == 0)
            {
                gd.selectedForSingleView = true;
                foundIt = true;
                break;
            }
        }

        // If we didn't find the group selected for single view, then just select the first
        // one we find
        if(!foundIt)
        {
            for(GroupDescriptor gd : ac.getMissionGroups())
            {
                if(gd.type == GroupDescriptor.Type.gtAudio)
                {
                    gd.selectedForSingleView = true;
                    break;
                }
            }
        }

        // Take care of the ones selected for multi-view
        if(!selectedMultiViewGroups.isEmpty())
        {
            for (String s : selectedMultiViewGroups)
            {
                for (GroupDescriptor gd : ac.getMissionGroups())
                {
                    if (gd.id.compareTo(s) == 0)
                    {
                        gd.selectedForMultiView = true;
                        break;
                    }
                }
            }
        }
        else
        {
            // TODO: we're selecting ALL the groups for multi-view right now!
            for (GroupDescriptor gd : ac.getMissionGroups())
            {
                if (gd.type == GroupDescriptor.Type.gtAudio)
                {
                    gd.selectedForMultiView = true;
                }
            }
        }
    }


    public static ActiveConfiguration loadConfiguration(ActiveConfiguration previousConfiguration, HashMap<String, GroupDescriptor> dynamics)
    {
        ActiveConfiguration rc;

        rc = new ActiveConfiguration();
        if(!rc.parseTemplate(Globals.getSharedPreferences().getString(PreferenceKeys.ACTIVE_MISSION_CONFIGURATION_JSON, "")))
        {
            rc = null;
        }

        if(rc != null)
        {
            // Merge in the dynamics (if any)
            if(dynamics != null)
            {
                for(GroupDescriptor gd : dynamics.values())
                {
                    rc.addDynamicGroup(gd);
                }
            }

            try
            {
                // !!!!!!!!!! BEGIN EXPERIMENTAL !!!!!!!!!!
                rc.setEnforceTransmitSmoothing(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_EXPERIMENT_ENABLE_TX_SMOOTHING, true));
                rc.setAllowDtx(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_EXPERIMENT_ALLOW_DTX, true));
                rc.setDiscoverSsdpAssets(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_EXPERIMENT_ENABLE_SSDP_DISCOVERY, false));
                rc.setDiscoverTrelliswareAssets(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_EXPERIMENT_ENABLE_TRELLISWARE_DISCOVERY, false));

                rc.setDiscoverCistechGv1Assets(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_EXPERIMENT_ENABLE_CISTECH_GV1_DISCOVERY, false));
                rc.setCistechGv1DiscoveryAddress(Globals.getSharedPreferences().getString(PreferenceKeys.USER_EXPERIMENT_CISTECH_GV1_DISCOVERY_ADDRESS, ""));
                rc.setCistechGv1DiscoveryPort(Integer.parseInt(Globals.getSharedPreferences().getString(PreferenceKeys.USER_EXPERIMENT_CISTECH_GV1_DISCOVERY_PORT, "0")));
                rc.setCistechGv1DiscoveryTimeoutSecs(Integer.parseInt(Globals.getSharedPreferences().getString(PreferenceKeys.USER_EXPERIMENT_CISTECH_GV1_DISCOVERY_TIMEOUT_SECS, "0")));

                // !!!!!!!!!! EXPERIMENTAL !!!!!!!!!!


                rc.setUiMode(Constants.UiMode.values()[Globals.getSharedPreferences().getInt(PreferenceKeys.UI_MODE, Constants.DEF_UI_MODE.ordinal())]);
                rc.setShowTextMessaging(Globals.getSharedPreferences().getBoolean(PreferenceKeys.UI_SHOW_TEXT_MESSAGING, Constants.DEF_UI_SHOW_TEXT_MESSAGING));

                rc.setNotifyOnNodeJoin(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_NOTIFY_NODE_JOIN, Constants.DEF_NOTIFY_NODE_JOIN));
                rc.setNotifyOnNodeLeave(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_NOTIFY_NODE_LEAVE, Constants.DEF_NOTIFY_NODE_LEAVE));
                rc.setNotifyOnNewAudio(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_NOTIFY_NEW_AUDIO_RX, Constants.DEF_NOTIFY_NEW_AUDIO_RX));
                rc.setNotifyOnNetworkError(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_NOTIFY_NETWORK_ERROR, Constants.DEF_NOTIFY_NETWORK_ERROR));
                rc.setEnableVibrations(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_NOTIFY_VIBRATIONS, Constants.DEF_NOTIFY_VIBRATIONS));
                rc.setNotifyPttEveryTime(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_NOTIFY_PTT_EVERY_TIME, Constants.DEF_NOTIFY_PTT_EVERY_TIME));

                rc.setPttToneNotificationLevel(Float.parseFloat(Globals.getSharedPreferences().getString(PreferenceKeys.USER_TONE_LEVEL_PTT, Float.toString(Constants.DEF_PTT_TONE_LEVEL))));
                rc.setErrorToneNotificationLevel(Float.parseFloat(Globals.getSharedPreferences().getString(PreferenceKeys.USER_TONE_LEVEL_ERROR, Float.toString(Constants.DEF_ERROR_TONE_LEVEL))));
                rc.setNotificationToneNotificationLevel(Float.parseFloat(Globals.getSharedPreferences().getString(PreferenceKeys.USER_TONE_LEVEL_NOTIFICATION, Float.toString(Constants.DEF_PTT_TONE_LEVEL))));

                rc.setMicrophoneAgcLevel(Integer.parseInt(Globals.getSharedPreferences().getString(PreferenceKeys.USER_AUDIO_MICROPHONE_AGC_LEVEL, "0")));
                rc.setSpeakerAgcLevel(Integer.parseInt(Globals.getSharedPreferences().getString(PreferenceKeys.USER_AUDIO_SPEAKER_AGC_LEVEL, "0")));

                rc.setNetworkInterfaceName(Globals.getSharedPreferences().getString(PreferenceKeys.NETWORK_BINDING_NIC_NAME, Constants.DEF_BINDING_NIC_NAME));

                rc.setNodeId(Globals.getSharedPreferences().getString(PreferenceKeys.USER_NODE_ID, Constants.DEF_USER_NODE_ID));
                rc.setUserId(Globals.getSharedPreferences().getString(PreferenceKeys.USER_ID, Constants.DEF_USER_ID));
                rc.setUserDisplayName(Globals.getSharedPreferences().getString(PreferenceKeys.USER_DISPLAY_NAME, Constants.DEF_USER_DISPLAY_NAME));
                rc.setUserAlias(Globals.getSharedPreferences().getString(PreferenceKeys.USER_ALIAS_ID, Constants.DEF_USER_ALIAS_ID));

                rc.setPttLatching(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_UI_PTT_LATCHING, Constants.DEF_USER_UI_PTT_LATCHING));
                rc.setPttVoiceControl(Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_UI_PTT_VOICE_CONTROL, Constants.DEF_USER_UI_PTT_VOICE_CONTROL));

                rc.setAudioInputDeviceId(Integer.parseInt(Globals.getSharedPreferences().getString(PreferenceKeys.USER_AUDIO_INPUT_DEVICE, Integer.toString(Constants.INVALID_AUDIO_DEVICE_ID))));
                rc.setAudioOutputDeviceId(Integer.parseInt(Globals.getSharedPreferences().getString(PreferenceKeys.USER_AUDIO_OUTPUT_DEVICE, Integer.toString(Constants.INVALID_AUDIO_DEVICE_ID))));

                if(rc.getUserAlias().isEmpty())
                {
                    rc.setUserAlias(generateUserAlias(Constants.DEF_USER_ALIAS_ID));
                }

                // Location
                ActiveConfiguration.LocationConfiguration lc = new ActiveConfiguration.LocationConfiguration();

                lc.enabled = Globals.getSharedPreferences().getBoolean(PreferenceKeys.USER_LOCATION_SHARED, Constants.DEF_LOCATION_ENABLED);
                lc.accuracy = Integer.parseInt(Globals.getSharedPreferences().getString(PreferenceKeys.USER_LOCATION_ACCURACY, Integer.toString(Constants.DEF_LOCATION_ACCURACY)));
                lc.intervalMs = (Integer.parseInt(Globals.getSharedPreferences().getString(PreferenceKeys.USER_LOCATION_INTERVAL_SECS, Integer.toString(Constants.DEF_LOCATION_INTERVAL_SECS))) * 1000);
                // TODO: figure out how to explain minIntervalMs in preference settings
                lc.minIntervalMs = lc.intervalMs;
                //lc.minIntervalMs = (Integer.parseInt(Globals.getSharedPreferences().getString(PreferenceKeys.USER_LOCATION_MIN_INTERVAL_SECS, Integer.toString(Constants.DEF_LOCATION_INTERVAL_SECS))) * 1000);
                lc.minDisplacement = Float.parseFloat(Globals.getSharedPreferences().getString(PreferenceKeys.USER_LOCATION_MIN_DISPLACEMENT, Float.toString(Constants.DEF_LOCATION_MIN_DISPLACEMENT)));

                rc.setLocationConfiguration(lc);

                // Multicast failover
                ActiveConfiguration.MulticastFailoverConfiguration mc = new ActiveConfiguration.MulticastFailoverConfiguration();

                ActiveConfiguration.MulticastFailoverPolicy mcfo = rc.getMissionMulticastFailoverPolicy();
                if(mcfo == ActiveConfiguration.MulticastFailoverPolicy.overrideAndAllow)
                {
                    mc.enabled = true;
                }
                else if(mcfo == ActiveConfiguration.MulticastFailoverPolicy.overrideAndPrevent)
                {
                    mc.enabled = false;
                }
                else
                {
                    mc.enabled = Globals.getSharedPreferences().getBoolean(PreferenceKeys.NETWORK_MULTICAST_FAILOVER_ENABLED, Constants.DEF_MULTICAST_FAILOVER_ENABLED);
                }
                //mc.thresholdSecs = Integer.parseInt(Globals.getSharedPreferences().getString(PreferenceKeys.NETWORK_MULTICAST_FAILOVER_SECS, Integer.toString(Constants.DEF_MULTICAST_FAILOVER_THRESHOLD_SECS)));
                rc.setMulticastFailoverConfiguration(mc);

                rc.setPriorityTxLevel(0);

                // Update states from the previous configuration
                rc.updateGroupStates(previousConfiguration);

                selectPreviouslySavedSelectedGroups(rc);
            }
            catch (Exception e)
            {
                e.printStackTrace();
                rc.setUseRp(false);
            }
        }

        return rc;
    }

    public static boolean isNullGuid(String guid)
    {
        if(isEmptyString(guid))
        {
            return true;
        }
        else if(guid.compareTo("{00000000-0000-0000-0000-000000000000}") == 0)
        {
            return true;
        }
        else if(guid.compareTo("00000000-0000-0000-0000-000000000000") == 0)
        {
            return true;
        }

        return false;
    }

    public static String generateMissionId()
    {
        String rc = UUID.randomUUID().toString();
        return "{" + rc + "}";
    }

    public static String generateUserNodeId()
    {
        String rc = UUID.randomUUID().toString();
        return "{" + rc + "}";
    }

    public static String generateUserAlias(String fmt)
    {
        Random rnd = new Random();
        int val = rnd.nextInt();
        if(val < 0)
        {
            val *= -1;
        }

        return String.format(fmt, val);
    }

    public static String generateGroupId()
    {
        String rc = UUID.randomUUID().toString();
        return "{" + rc + "}";
    }

    public static String generateCryptoPassword()
    {
        Random random = new Random();

        byte[] byteArray = new byte[32];
        random.nextBytes(byteArray);

        return bytesToHex(byteArray);
    }

    private final static char[] hexArray = "0123456789ABCDEF".toCharArray();//NON-NLS

    public static String bytesToHex(byte[] bytes)
    {
        char[] hexChars = new char[bytes.length * 2];

        for ( int j = 0; j < bytes.length; j++ )
        {
            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = hexArray[v >>> 4];
            hexChars[j * 2 + 1] = hexArray[v & 0x0F];
        }

        return new String(hexChars);
    }

    public static byte[] compress(byte[] data, int ofs, int len)
    {
        byte[] rc;

        try
        {
            ByteArrayOutputStream bos = new ByteArrayOutputStream(len);
            GZIPOutputStream gzip = new GZIPOutputStream(bos);
            gzip.write(data, ofs, len);
            gzip.close();
            rc = bos.toByteArray();
            bos.close();
        }
        catch(Exception e)
        {
            e.printStackTrace();
            rc = null;
        }

        return rc;
    }

    public static byte[] inflate(byte[] data)
    {
        byte[] rc;

        try
        {
            ByteArrayInputStream bis = new ByteArrayInputStream(data);
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            GZIPInputStream gis = new GZIPInputStream(bis);

            byte[] buffer = new byte[1024];
            int len;
            while ((len = gis.read(buffer)) != -1)
            {
                bos.write(buffer, 0, len);
            }
            rc = bos.toByteArray();

            gis.close();
            bis.close();
            bos.close();
        }
        catch(Exception e)
        {
            e.printStackTrace();
            rc = null;
        }

        return rc;
    }

    public static Bitmap byteArrayToQrCodeBitmap(byte[] ba, int width, int height)
    {
        Bitmap rc;

        try
        {
            String str = new String(ba);

            MultiFormatWriter multiFormatWriter = new MultiFormatWriter();
            BitMatrix bitMatrix = multiFormatWriter.encode(str, BarcodeFormat.QR_CODE, width, height);
            BarcodeEncoder barcodeEncoder = new BarcodeEncoder();
            rc = barcodeEncoder.createBitmap(bitMatrix);
        }
        catch (Exception e)
        {
            rc = null;
        }

        return rc;
    }

    public static Bitmap stringToQrCodeBitmap(String str, int width, int height)
    {
        Bitmap rc;

        try
        {
            MultiFormatWriter multiFormatWriter = new MultiFormatWriter();
            BitMatrix bitMatrix = multiFormatWriter.encode(str, BarcodeFormat.QR_CODE, width, height);
            BarcodeEncoder barcodeEncoder = new BarcodeEncoder();
            rc = barcodeEncoder.createBitmap(bitMatrix);
        }
        catch (Exception e)
        {
            rc = null;
        }

        return rc;
    }

    public static String qrCodeBitmapToString(Bitmap bm)
    {
        String rc;

        try
        {
            int h = bm.getHeight();
            int w = bm.getWidth();
            int[] ints = new int[bm.getWidth() * bm.getHeight()];

            bm.getPixels(ints, 0, bm.getWidth(), 0, 0, w, h);

            BinaryBitmap bitmap = new BinaryBitmap(new HybridBinarizer(new RGBLuminanceSource(w, h, ints)));
            Map<DecodeHintType, Object> hints = new EnumMap<>(DecodeHintType.class);
            hints.put(DecodeHintType.PURE_BARCODE, Boolean.FALSE);
            hints.put(DecodeHintType.POSSIBLE_FORMATS, EnumSet.allOf(BarcodeFormat.class));
            hints.put(DecodeHintType.TRY_HARDER, Boolean.TRUE);

            Result res = new MultiFormatReader().decode(bitmap, hints);

            rc = res.getText();
        }
        catch (Exception e)
        {
            rc = null;
            e.printStackTrace();
        }

        return rc;
    }

    public static String toHexString(byte[] bytes)
    {
        if(bytes == null || bytes.length == 0)
        {
            return null;
        }

        StringBuffer hs = new StringBuffer();
        for (int x = 0; x < bytes.length; x++)
        {
            String s = Integer.toHexString(0xFF & bytes[x]);
            if(s.length() == 1)
            {
                s = "0" + s;
            }

            hs.append(s);
        }

        return hs.toString().toUpperCase();
    }

    public static void showErrorMsg(Context ctx, String msg)
    {
        /*
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(ctx);

        alertDialogBuilder.setTitle("Error");
        alertDialogBuilder.setMessage(msg);
        alertDialogBuilder.setCancelable(true);

        AlertDialog dlg = alertDialogBuilder.create();
        dlg.show();
        */

        showShortPopupMsg(ctx, msg);
    }

    private static void showPopupMsg(Context ctx, String msg, int len)
    {
        Toast.makeText(ctx, msg, len).show();
    }

    public static void showShortPopupMsg(Context ctx, String msg)
    {
        showPopupMsg(ctx, msg, Toast.LENGTH_SHORT);
    }

    public static void showLongPopupMsg(Context ctx, String msg)
    {
        showPopupMsg(ctx, msg, Toast.LENGTH_LONG);
    }

    public static void showShortPopupMsg(Context ctx, int msgId)
    {
        showPopupMsg(ctx, ctx.getString(msgId), Toast.LENGTH_SHORT);
    }

    public static void showLongPopupMsg(Context ctx, int msgId)
    {
        showPopupMsg(ctx, ctx.getString(msgId), Toast.LENGTH_LONG);
    }

    public static void showMessageInDialog(Context ctx, String title, String msg)
    {
        AlertDialog dlg = new AlertDialog.Builder(ctx)
                .setTitle(title)
                .setMessage(msg)
                .setCancelable(false)
                .setPositiveButton(ctx.getString(R.string.button_ok), new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                    }
                }).create();

        dlg.show();
    }

    public static Intent intentToIgnoreBatteryOptimization(Context ctx)
    {
        Intent intent = null;

        if(Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP)
        {
            intent = new Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS);

            intent.setData(Uri.parse("package:" + ctx.getPackageName()));//NON-NLS

            if(!isIntentCallable(ctx, intent))
            {
                intent.setData(null);
                if(!isIntentCallable(ctx, intent))
                {
                    intent = null;
                }
            }
        }

        return intent;
    }


    @SuppressWarnings("BooleanMethodIsAlwaysInverted")
    public static boolean isIntentCallable(Context ctx, Intent intent)
    {
        List<ResolveInfo> list = ctx.getPackageManager()
                .queryIntentActivities(intent, PackageManager.MATCH_DEFAULT_ONLY);

        return list.size() > 0;
    }

    public static boolean isLocationValid(Location loc)
    {
        if(loc.getLatitude() < -90.0 || loc.getLatitude() > 90.0)
        {
            return false;
        }

        if(loc.getLongitude() < -180.0 || loc.getLongitude() > 180.0)
        {
            return false;
        }

        if(loc.hasSpeed() && Float.isNaN(loc.getSpeed()))
        {
            return false;
        }

        if(loc.hasAltitude() && Double.isNaN(loc.getAltitude()))
        {
            return false;
        }

        return !loc.hasBearing() || !Float.isNaN(loc.getBearing());
    }

    public static long nowMs()
    {
        return System.currentTimeMillis();
    }

    public static boolean isManifestPermissionPresent(String permissionName)
    {
        try
        {
            PackageInfo info = Globals.getEngageApplication()
                                        .getPackageManager()
                                        .getPackageInfo(Globals.getEngageApplication().getPackageName(),
                                                    PackageManager.GET_PERMISSIONS);

            if (info.requestedPermissions != null)
            {
                for (String p : info.requestedPermissions)
                {
                    if (p.equals(permissionName))
                    {
                        return true;
                    }
                }
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

        return false;
    }

    public static String getMetaData(String keyName)
    {
        try
        {
            ApplicationInfo ai = Globals.getEngageApplication()
                                        .getPackageManager()
                                        .getApplicationInfo(Globals.getEngageApplication()
                                                .getPackageName(), PackageManager.GET_META_DATA);
            Bundle bundle = ai.metaData;
            return bundle.getString(keyName);
        }
        catch (Exception e)
        {
            return null;
        }
    }

    public static int parseIntSafe(String s)
    {
        int rc;

        try
        {
            rc = Integer.parseInt(s);
        }
        catch (Exception e)
        {
            rc = 0;
        }

        return rc;
    }

    public static String md5HashOfString(String s)
    {
        String rc;

        try
        {
            MessageDigest md = MessageDigest.getInstance("MD5");//NON-NLS
            md.update(s.getBytes());
            byte[] digest = md.digest();
            rc = bytesToHex(digest);
        }
        catch (Exception e)
        {
            rc = null;
        }

        return rc;
    }

    public static void hideKeyboardFrom(Context context, View view)
    {
        InputMethodManager imm = (InputMethodManager) context.getSystemService(Activity.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }

    public static Date javaDateFromUnixSeconds(long seconds)
    {
        return javaDateFromUnixMilliseconds(seconds * 1000L);
    }

    public static Date javaDateFromUnixMilliseconds(long milliseconds)
    {
        return new java.util.Date(milliseconds);
    }

    public static String formatDateUtc(Date dt)
    {
        try
        {
            SimpleDateFormat sdf = new java.text.SimpleDateFormat("yyyy-MM-dd HH:mm:ss z");//NON-NLS
            sdf.setTimeZone(java.util.TimeZone.getTimeZone("GMT"));//NON-NLS
            return sdf.format(dt);
        }
        catch (Exception e)
        {
            return null;
        }
    }

    public static boolean stringsMatch(String s1, String s2)
    {
        if(s1 == null && s2 == null)
        {
            return true;
        }

        if((s1 == null && s2 != null) || (s1 != null && s2 == null))
        {
            return false;
        }

        return (s1.compareTo(s2) == 0);
    }

    public static String formattedTimespan(long from, long to)
    {
        String rc;
        long span = (to - from);

        if (span < 0)
        {
            rc = null;
        }
        else if (span < (60 * 1000))
        {
            rc = DateUtils.getRelativeTimeSpanString(to, from, DateUtils.SECOND_IN_MILLIS).toString();
        }
        else if (span < (60 * 1000 * 60))
        {
            rc = DateUtils.getRelativeTimeSpanString(to, from, DateUtils.MINUTE_IN_MILLIS).toString();
        }
        else if (span < (60 * 1000 * 60 * 24))
        {
            rc = DateUtils.getRelativeTimeSpanString(to, from, DateUtils.HOUR_IN_MILLIS).toString();
        }
        else if (span < (60 * 1000 * 60 * 24 * 7))
        {
            rc = DateUtils.getRelativeTimeSpanString(to, from, DateUtils.DAY_IN_MILLIS).toString();
        }
        else if (span < (60 * 1000 * 60 * 24 * 7 * 4))
        {
            rc = DateUtils.getRelativeTimeSpanString(to, from, DateUtils.WEEK_IN_MILLIS).toString();
        }
        else
        {
            rc = DateUtils.getRelativeTimeSpanString(to, from, DateUtils.WEEK_IN_MILLIS).toString();
        }

        return rc;
    }

    public static ProgressDialog showProgressMessage(Context ctx, String message, ProgressDialog existingDialog)
    {
        hideProgressMessage(existingDialog);

        ProgressDialog rc = new ProgressDialog(ctx);
        rc.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        rc.setMessage(message);
        rc.setIndeterminate(true);
        rc.setCanceledOnTouchOutside(false);
        rc.show();

        return rc;
    }

    public static ProgressDialog hideProgressMessage(ProgressDialog existingDialog)
    {
        if(existingDialog != null)
        {
            existingDialog.dismiss();
        }

        return null;
    }

    public static JSONObject getJsonObject(JSONObject obj, String name)
    {
        JSONObject rc = null;

        try
        {
            if(!obj.isNull(name))
            {
                rc = obj.getJSONObject(name);
            }
        }
        catch (Exception e)
        {
            rc = null;
        }

        return rc;
    }

    /*
    public static int intOpt(String s, int defaultValue)
    {
        int rc;

        try
        {
            rc = Integer.parseInt(s);
        }
        catch (Exception e)
        {
            rc = defaultValue;
        }

        return rc;
    }

    public static boolean boolOpt(String s, boolean defaultValue)
    {
        boolean rc;

        try
        {
            rc = (intOpt(s, defaultValue ? 0 : 1) != 0);
        }
        catch (Exception e)
        {
            rc = defaultValue;
        }

        return rc;
    }

    public static String stringOpt(String s, String defaultValue)
    {
        if(isEmptyString(s))
        {
            return defaultValue;
        }
        else
        {
            return s;
        }
    }

    public static int intOpt(int resId, int defaultValue)
    {
        int rc;

        try
        {
            rc = Globals.getContext().getResources().getInteger(resId);
        }
        catch (Exception e)
        {
            rc = defaultValue;
        }

        return rc;
    }

    public static boolean boolOpt(int resId, boolean defaultValue)
    {
        boolean rc;

        try
        {
            rc = Globals.getContext().getResources().getBoolean(resId);
        }
        catch (Exception e)
        {
            rc = defaultValue;
        }

        return rc;
    }

    public static String stringOpt(int resId, String defaultValue)
    {
        String rc;

        try
        {
            rc = Globals.getContext().getResources().getString(resId);
        }
        catch (Exception e)
        {
            rc = defaultValue;
        }

        return rc;
    }
    */

    public static String readTextFile(Context ctx, Uri uri)
    {
        String rc;

        try
        {
            InputStreamReader inputStreamReader = new InputStreamReader(ctx.getContentResolver().openInputStream(uri));
            BufferedReader bufferedReader = new BufferedReader(inputStreamReader);
            StringBuilder sb = new StringBuilder();

            String s;
            while ((s = bufferedReader.readLine()) != null)
            {
                sb.append(s);
            }

            rc = sb.toString();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            rc = null;
        }

        return rc;
    }

    public static byte[] readBinaryFile(Context ctx, Uri uri)
    {
        byte[] rc;

        try
        {
            InputStream inputStream = ctx.getContentResolver().openInputStream(uri);

            ByteArrayOutputStream byteBuffer = new ByteArrayOutputStream();
            int bufferSize = 1024;
            byte[] buffer = new byte[bufferSize];

            int len = 0;
            while((len = inputStream.read(buffer)) != -1)
            {
                byteBuffer.write(buffer, 0, len);
            }

            inputStream.close();

            rc = byteBuffer.toByteArray();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            rc = null;
        }

        return rc;
    }

    public static boolean doesFileExist(String fn)
    {
        boolean rc = false;

        try
        {
            rc = new File(fn).exists();
        }
        catch (Exception e)
        {
            rc = false;
        }

        return rc;
    }

    public static void setInboundMissionPassword(String pwd)
    {
        Globals.getSharedPreferencesEditor().putString(PreferenceKeys.INCOMING_MISSION_PASSWORD, pwd);
        Globals.getSharedPreferencesEditor().apply();
    }

    public static String getInboundMissionPassword()
    {
        return Globals.getSharedPreferences().getString(PreferenceKeys.INCOMING_MISSION_PASSWORD, null);
    }
}
