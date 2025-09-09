package com.rallytac.engageandroid;

import android.content.Context;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import androidx.core.app.ActivityCompat;

import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Collections;
import java.util.List;

public class NetworkUtils {
    private static final String TAG = "NetUtil";
    public static boolean isAnyConnectivityAvailable(Context context) {
        ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (cm == null) {
            Globals.getLogger().w(TAG, "No ConnectivityManager available");//NON-NLS
            return false; // No ConnectivityManager available
        }

        Network network = cm.getActiveNetwork();
        if (network == null) {
            Globals.getLogger().w(TAG, "No active network");//NON-NLS
            return false; // No active network
        }

        NetworkCapabilities capabilities = cm.getNetworkCapabilities(network);
        if (capabilities == null) {
            Globals.getLogger().w(TAG, "No network capabilities");//NON-NLS
            return false; // No network capabilities
        }

        if(!(capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) ||
                capabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR) ||
                capabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET) ||
                capabilities.hasTransport(NetworkCapabilities.TRANSPORT_VPN) ||
                capabilities.hasTransport(NetworkCapabilities.TRANSPORT_BLUETOOTH) ||
                capabilities.hasTransport(NetworkCapabilities.TRANSPORT_LOWPAN) ||
                capabilities.hasTransport(NetworkCapabilities.TRANSPORT_USB))) {

            Globals.getLogger().w(TAG, "No viable transports available");//NON-NLS
            return false;
        }

        // If we only have cellular then make sure we have an operator name
        if(capabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR))
        {
            if (!(capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) ||
                    capabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET) ||
                    capabilities.hasTransport(NetworkCapabilities.TRANSPORT_VPN) ||
                    capabilities.hasTransport(NetworkCapabilities.TRANSPORT_BLUETOOTH) ||
                    capabilities.hasTransport(NetworkCapabilities.TRANSPORT_LOWPAN) ||
                    capabilities.hasTransport(NetworkCapabilities.TRANSPORT_USB))) {

                if (Utils.isEmptyString(getCellularOperatorName(context))) {
                    Globals.getLogger().w(TAG, "No operator name for cellular-only network");//NON-NLS
                    return false;
                }
            }
        }

        // Check if we have an actual IP networking connection
        if (!hasValidIPAddress())
        {
            Globals.getLogger().w(TAG, "No valid IP address");//NON-NLS
            return false;
        }

        Globals.getLogger().w(TAG, "isAnyConnectivityAvailable has determined some connectivity is available");//NON-NLS
        return true;
    }

    private static boolean hasValidIPAddress() {
        try {
            List<NetworkInterface> interfaces = Collections.list(NetworkInterface.getNetworkInterfaces());
            for (NetworkInterface ni : interfaces) {
                if(!ni.getName().startsWith("dummy")) {
                    if (!ni.isLoopback() && ni.isUp()) { // Ignore loopback and inactive interfaces
                        List<InetAddress> addresses = Collections.list(ni.getInetAddresses());
                        for (InetAddress address : addresses) {
                            if (isValidIPAddress(address)) {
                                return true; // Found a usable global IP
                            }
                        }
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    private static boolean isValidIPAddress(InetAddress address) {
        // Ignore loopback addresses (127.0.0.1, ::1)
        if (address.isLoopbackAddress()) return false;

        // Ignore link-local addresses (fe80::/10)
        if (address instanceof Inet6Address) {
            Inet6Address ipv6Address = (Inet6Address) address;
            if (ipv6Address.isLinkLocalAddress()) return false;
        }

        return true; // Valid IPv4 or globally routable IPv6 address
    }

    public static String getCellularOperatorName(Context context) {
        TelephonyManager telephonyManager = (TelephonyManager)
                context.getSystemService(Context.TELEPHONY_SERVICE);
        if (telephonyManager != null) {
            if (ActivityCompat.checkSelfPermission(context, android.Manifest.permission.READ_PHONE_STATE) != PackageManager.PERMISSION_GRANTED) {
                return "";
            }
            else {
                return telephonyManager.getNetworkOperatorName();
            }
        }

        return "";
    }

    public static String getConnectivityDetails(Context context) {
        ConnectivityManager connectivityManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivityManager == null) {
            return "Connectivity Manager unavailable";
        }

        // Get the current active network
        Network activeNetwork = connectivityManager.getActiveNetwork();
        if (activeNetwork == null) {
            return "No active network connection";
        }

        NetworkCapabilities capabilities = connectivityManager.getNetworkCapabilities(activeNetwork);
        if (capabilities == null) {
            return "Connected, but unable to determine network capabilities";
        }

        StringBuilder connectivityInfo = new StringBuilder();

        // Check if connected to Wi-Fi
        if (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)) {
            connectivityInfo.append("Wi-Fi");

            // Get Wi-Fi details
            WifiManager wifiManager = (WifiManager) context.getApplicationContext()
                    .getSystemService(Context.WIFI_SERVICE);
            if (wifiManager != null) {
                WifiInfo wifiInfo = wifiManager.getConnectionInfo();
                if (wifiInfo != null) {
                    connectivityInfo.append(" (SSID: ").append(wifiInfo.getSSID()).append(")");
                }
            }
        }
        // Check if connected to Mobile Data
        else if (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR)) {
            connectivityInfo.append("Mobile Data");

            // Get mobile network details
            TelephonyManager telephonyManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);

            if (telephonyManager != null) {
                if (ActivityCompat.checkSelfPermission(context, android.Manifest.permission.READ_PHONE_STATE) == PackageManager.PERMISSION_GRANTED) {
                    int networkType = telephonyManager.getNetworkType();
                    connectivityInfo.append(" (").append(getMobileNetworkType(networkType)).append(")");

                    String operatorName = getCellularOperatorName(context);
                    if (Utils.isEmptyString(operatorName)) {
                        operatorName = "?";
                    }

                    connectivityInfo.append(" - ").append(operatorName);
                }

                /*
                if (ActivityCompat.checkSelfPermission(context, android.Manifest.permission.READ_PHONE_STATE) != PackageManager.PERMISSION_GRANTED) {
                    connectivityInfo.append(" (").append("*no permission*").append(")");
                }
                else {
                    int networkType = telephonyManager.getNetworkType();
                    String operatorName = telephonyManager.getNetworkOperatorName(); // Get operator name
                    connectivityInfo.append(" (").append(getMobileNetworkType(networkType)).append(")");
                    if (operatorName != null && !operatorName.isEmpty()) {
                        connectivityInfo.append(" - ").append(operatorName);
                    }
                    else {
                        connectivityInfo.append(" (").append("*no operator*").append(")");
                    }
                }
                 */
            }
        }
        // Check if connected via Ethernet
        else if (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET)) {
            connectivityInfo.append("Ethernet");
        }
        // Check if connected via Bluetooth
        else if (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_BLUETOOTH)) {
            connectivityInfo.append("Bluetooth");
        }
        // Unknown connection
        else {
            connectivityInfo.append("Connected via unknown network");
        }

        return connectivityInfo.toString();
    }

    // Helper function to translate TelephonyManager network types into readable strings
    private static String getMobileNetworkType(int networkType) {
        switch (networkType) {
            case TelephonyManager.NETWORK_TYPE_GPRS: return "2G (GPRS)";
            case TelephonyManager.NETWORK_TYPE_EDGE: return "2G (EDGE)";
            case TelephonyManager.NETWORK_TYPE_UMTS: return "3G (UMTS)";
            case TelephonyManager.NETWORK_TYPE_HSDPA: return "3G (HSDPA)";
            case TelephonyManager.NETWORK_TYPE_HSUPA: return "3G (HSUPA)";
            case TelephonyManager.NETWORK_TYPE_HSPA: return "3G (HSPA)";
            case TelephonyManager.NETWORK_TYPE_CDMA: return "2G (CDMA)";
            case TelephonyManager.NETWORK_TYPE_EVDO_0: return "3G (EVDO 0)";
            case TelephonyManager.NETWORK_TYPE_EVDO_A: return "3G (EVDO A)";
            case TelephonyManager.NETWORK_TYPE_EVDO_B: return "3G (EVDO B)";
            case TelephonyManager.NETWORK_TYPE_LTE: return "4G (LTE)";
            case TelephonyManager.NETWORK_TYPE_NR: return "5G (NR)";
            default: return "Unknown";
        }
    }
}
