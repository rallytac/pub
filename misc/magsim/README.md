# Magellan Simulator (magsim)
`Magellan` is a project of ours that allows for the automated provisioning of communications *assets* on a network.  Basically, what this means is that an Engage-powered application can determine what assets are available on a network and automatically provision those assets inside the application.

For example: in the past, to add two-way radio talk groups to your voice application, you'd need administrative personnel configure something like a two-way radio gateway to interface to your radio system.  Then, that configuration would need to be setup on a management/provisioning server system of some sort.  Then your application would need to login to that server with all the right credentials and download information about the available talk groups.

Now, if things change on the gateway, the management system has to be kept synchronized with the gateway, and all the applications that had previously load talk groups would need to notified that changes have occurred - and those changes merged into the running app.

While its pretty straightforward to do this kind of thing in very small environments, it just doesn't scale and is prone to a host of errors.  (Oh, and you need servers for all this stuff!)

So ... we put together the `Magellan` specification which makes the gateway the single source of *truth* regarding the voice assets it offers.  By this we mean that the gateway is the **only** reliable source of valid information regarding its voice assets.  There's no need for its configuration to be duplicated and synchronized somewhere else like a server. There's no need for user applications to login to servers, there's no need for users to know what network they're on in order to determine what voice assets are available.  And so on, and so on.

## Magellan Operation
As a quick overview/reminder of Magellan; let's take a high-level step-by-step look at how this operates.

- A device (such as a radio gateway) on a network is configured with talk groups.
- The device (in Magellan we call it a `thing`) then advertises itself on the network as providing a set of assets.  It does this by using standard protocols like [SSDP](https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol), [mDNS](https://en.wikipedia.org/wiki/Multicast_DNS), and [SAP](https://en.wikipedia.org/wiki/Session_Announcement_Protocol).
- Magellan-aware on the network *discover* the device using the above protocols.
- Once the device has been discovered, the application establishes a secure connection to it using TLS protected by X.509 certificates. (This authentication is mutual; meaning that not only does the user application verify the device's X.509 certificate, the device asks for the user application's certificate as well.)
- A transaction (we call it `interrogation`) is then carried out between these two whereby the application requests the device to provide it with configuration for the voice assets the device supports.  The device, in turn, returns voice assets that are available to the application based on content of the application's X.509 certificate.
- Once the application has the voice asset configuration(s) - including things like multicast/unicast addressing, details about Rallypoints if utilized, encryption, floor-control parameters, priority configuration, and so on; it can present those to the user for them to add to their existing user interface - so they can talk to these newly acquired voice assets.
  
## This Application (magsim)
Alright, lets say you're developing an application to do the user-side stuff we described above.  But you don't actually have a Magellan-capable device at your disposal.  Well, you'd like to *simulate* it - right!?  That's what this little app does.

`magsim` is a small Node.js-based application that implments the high-level `advertising` and `interrogation` features of a *real* Magellan device.  It doesn't have all the bells and whistles of a full-blown Magellan-capable product but it gives you, the application developer, the means to develop pretty much everything you need in your Engage-powered application to interact with real Magellan devices.

### How magsim works
As stated above, `magsim` is a Node.js application that covers the important basics of a real Magellan device.  To that end it does advertising using SSDP only (not mDNS or SAP), and doesn't perform significant security checks of the user applicatrion's certificate for which voice assets are to be returned.  Rather, `magsim` will, upon interrogation, simply read a JSON file of pre-configured voice assets (talk groups or just `groups` in Engage terminology), format the data in such a way that the user application will understand it, and return the configuration as JSON to to app.

#### Dependencies
In order to do its job `magsim` needs a few things:
- A fairly new version of Node.js installed on your simulated *gateway*.  This can be your development machine or another box on the network.  (There are no other Node.js dependencies.)
- A *server* certificate and private key (`server.crt` and `server.key` in the `certs` directory).
- A Certificate Authority (CA) Certificate (`ca.crt` in the `certs` directory).
- A file containing the voice asset configuration (`groups.json` in tis directory).
- A network capable of supporting IPv4 **multicasting** of SSDP packets.
- A network capable of supporting IPv4 **broadcasting** of SSDP packets.

>If you're familiar with SSDP, you should know that SSDP uses IP multicast for sending out `NOTIFY` packets on the network - i.e. *advertising*.  But networks and/or operating systems often have a love-hate relationship with multicast - resulting in multicast being unreliable if not properly configured and managed.  So, many vendors opt to not only *multicast* the packets, they *broadcast* them as well.  This is terribly inefficient and is a security threat and bandwidth hog (amongst other issues), but this is the way of the world unfortunately.  So, for compatibility purposes, `magsim` does both multicast AND broadcast (both optional) advertsing.  And, of course, your Engage-powered application will listen for both multicast and broadcast - because its nice in tha way ;).


We've provided magsim in source form so you can change it to your heart's content.  While `magsim` has some built-in parameters that can be set from the command-line; if that's not enough, have at it and modify as you please.

## Running masgim
To run this puppy you'll need to open a terminal window/command prompt and navigate to the directory where `magsim` is located.  Then, invoke it using Nodejs:

```shell
$ node magsim.js
```

If you do just that, though, `magsim` will complain that you did not provide a machine address (the `-ma` parameter).  In fact, `magsim` will, at that point give you the invocation syntax as below:

```shell
$ node magsim.js
---------------------------------------------------------------------------
Magellan Simulator (magsim) version 0.2

Copyright (c) 2020 Rally Tactical Systems, Inc.
---------------------------------------------------------------------------
ERROR: No machineAddress

Usage: node magsim.js <options>

   options:
      -ma:machine_address ............................. specify the local machine IPv4 address (REQUIRED)
      [-rsp:rest_server_port] ......................... specify the REST server port to listen on (default is 8081)
      [-ssdpint:ssdp_advertising_interval_secs] ....... specify SSDP advertising interval in seconds (default is 5)
      [-nossdpmc] ..................................... disable SSDP advertising over IP multicast
      [-nossdpbc] ..................................... disable SSDP advertising over IP broadcatcast
      [-nohttps] ...................................... disable HTTPS
      [-noclientcert] ................................. disable client certificate request
```

The parameters supported are:
- `-ma` : The IPv4 address of the local machine. This is required.
- `-rsp`: The IPv4 TCP port that `magsim`'s built-in web server will listen on for interrogation requests.  The default is `8081`.
- `-ssdpint`: The interval (in seconds) at which SSDP advertisements are sent.  The default is `5`.  (Also, those SSDP packets will have the `max-age` property of their `CACHE-CONTROL` header set to **twice** this value.)
- `-nossdpmc` : Will turn **OFF** SSDP advertising via IP multicast.
- `-nossdpbc` : Will turn **OFF** SSDP advertising via IP broadcast.
- `-nohttps` : Will turn **OFF** HTTPS (secured connections).
- `-noclientcert` : Will turn **OFF** the request for the client certificate (i.e. no mutual authentication).

>If you specify both `-nossdpmc` and `-nossdpbc` then there will be no advertising via SSDP.  But `magsim`'s REST web server will still continue to operate.

>*!!!! DEVELOPMENT ONLY !!!!* The `-nohttps` parameter allows for client connections without TLS-based encryption and authentication.  While not recommended for production purposes, turning off HTTPS allows for easier debugging of packet captures and alleviates the client-side developer from having to support TLS right off the bat.
>
>Futhermore, the `-noclientcert` parameter indicates **NOT** to request a certificate from the client-side.
This is meant for development purposes only as production Magellan systems always use the contents of the client certificate to determine what voice asset definitions are to be returned in queries.  Hence, if your client does not support client-side certificates, your client will not work in the field.

Alright, let's actually run it.  For this example, the IP address we want `magsim` to use is `192.168.1.79`.  Also, our client application does not yet support certificates or TLS.  So we'll turn off HTTPS with the `-nohttps` parameter.

```shell
$ node magsim.js -ma:192.168.1.79 -nohttps
---------------------------------------------------------------------------
Magellan Simulator (magsim) version 0.2

Copyright (c) 2020 Rally Tactical Systems, Inc.
---------------------------------------------------------------------------
REST Server (NON-SECURED) listening on 192.168.1.79:8081.
SSDP multicast advertising every 5 seconds.
SSDP broadcast advertising every 5 seconds.

Press Ctrl-C to stop.
---------------------------------------------------------------------------
unsecured config request from ::ffff:192.168.1.182
```

Pretty straightforward - no!?

The last line of the output above shows "`unsecured config request from ::ffff:192.168.1.182`".  This means that a discovering application on the network at `192.168.1.182` saw the SSDP advertisment and contacted the REST server for a configuration request.  This will repeat each time a discovering client contacts the REST server.

Note, though, that it specifically says `unsecured` in the log output.  That's because we specified `-nohttps` and therefore `magsim` will not request the far-end to send its own certificate.  If you want to secure the connection, you need to do some work on the application side as below.

## Certificates
In production, an application that will discover Magellan assets needs to implement HTTPS (specifically TLS 1.2 and higher).  Hence, to verify the certificate presented by `magsim`, your application will need the CA certificate (the public portion only of course) that was used to create `magsim`'s certificate.  All this sample's certificates can be found in the `certs` directory - including the CA certificate (`ca.crt`) which was used to create `server.crt`.  You'll need to add `ca.crt` to your trust chain for verification.

Also, when your application connects to `magsim`, you'll be asked to provide the *client's* certificate as well.  For this sample, use the `client.crt` certificate (it was also created using `ca.crt`).

In production, however, you'll not be using these sample certificates but, rather, certificates that were obtained from a commercial provider or some other certificate management organization.  In that case, remember to include the correct CA certificates on both sides' CA chains.

|File|Description|Distribution|
|-|-|-|
|ca.crt|The CA certificate used to create server and client certificates.|Add this file to your application's CA chain.|
|ca.key|The CA certificate's private key.|It stays where it is. Keep it *private*.|
|server.crt|The `magsim` server certificate.|It stays where it is.|
|server.crt|The `magsim` server certificate's private key.|It stays where it is. Keep it *private*.|
|server.serial|The serial number/thumbprint of `magsim` server certificate.|It stays where it is.|
|client.crt|The application's certificate.|Add it to your application.|
|client.key|The application's certificate private key.|Add it to your application. But keep it *private*.|
|client.serial|The serial number/thumbprint of the application's certificate.|Add it to your application and see below.|

Now, let's say we've got our far-end application developed - including all the X.509 certificate stuff.  At this point you'll want to go fully secure and *NOT* specify `-nohttps`.  Things are going to look a little different:

```shell
$ node magsim.js -ma:192.168.1.79
---------------------------------------------------------------------------
Magellan Simulator (magsim) version 0.2

Copyright (c) 2020 Rally Tactical Systems, Inc.
---------------------------------------------------------------------------
REST Server (SECURED) listening on 192.168.1.79:8081.
SSDP multicast advertising every 5 seconds.
SSDP broadcast advertising every 5 seconds.

Press Ctrl-C to stop.
---------------------------------------------------------------------------
secured config request from ::ffff:192.168.1.182
   >peer certificate O='Magellan App Developer Inc'
   >issued by O='My Own Certificate Authority Inc'
```

- First off, `magsim` will now say that its REST server is **SECURED** in the header of the log output.
- Next, when a peer (client/far-end application) successfully establishes a secure connection, `magsim` shows details of the peer's X.509 certificate as well as the CA certificate that peer certificate was created with.  (Actually, not all the certificate detail is being shown; only the `O` element of the certificate's `subject`.  But this is good enough for logging without filling the screen with goop.)

### Client-Side Certificates
Alright, in order to end up with output that looks like what we see above, some work is going to be required on the end-user application side.  Assuming you're developing an Engage-based application (and why would you not be!?), you're going to need to provide your Engage Engine with some certificate goodies in its policy JSON.  Specifically, you're going to need updates to the `discovery` field of that policy JSON.  Something like this:

```javascript
    .
    .
    .
    "discovery": {
        "magellan": {
            "enabled":true,
            "security": {
                    "certificate": "@./client.crt",
                    "key": "@./client.key"
            },
            "tls":{
                "caCertificates" : [
                    "@./ca.crt"
                ]
            }
        },
    }
    .
    .
    .
```

Let's go through this stuff:

- `discovery.magellan.enabled`: You obviously need to enable Magelllan discovery in your policy.  So this value needs to be `true`.
- `discovery.magellan.security`: Here you'll need to provide the X.509 certificate that the application will present to `magsim` when the interrogation connection is made.  You'll also need to provide the private key for that certificate so that the connection can be properly secured.
- `discovery.tls.caCertificates`: This list needs to include the CA certificate that `magsim`'s X.509 certificate was created with.

In this example we've copied files from `magsim`'s `certs` directory to the application side.  Specifically, we brought over `client.crt`, `client.key`, and `ca.crt`.  In a production environment, you'll need to provide your own client certificate and provide that certificates' CA certificate to your Magellan-enabled devices.  Similarly, in your client application, you'll need CA certificates for the Magellan devices you intend connecting to.

>This business with CA certificates seems awfully convoluted at first glance but, frankly, its not a major hurdle.  Implementors most often use certificates issued by commercial entities whose CA certificates are readily available and easy to install.  If an organization is its own CA (like governments very often tend to be), the CA certificate(s) used by that organization is similarly generally easily obtained and installed.

>It is super-important, though, that your application feature some degree of certificate management - such as importing, secured storage, and so on.  To make that task a little easier, we highly recommend your Engage-powered applications use Engage's **Certificate Store** capabilities.  Check out [X.509 Certificates](https://github.com/rallytac/pub/wiki/Engage-Security#x509-certificates) and [how to use ecstool](https://github.com/rallytac/pub/wiki/Using-ecstool) in our wiki articles.

## Group Configurations
This example of `magsim` is hardcoded to obtain its group definitions from `groups.json`.  If you're familiar with Engage configurations you'll quickly recognize that the structure of `magsim`'s groups configuration - as well as the Magellan-compliant JSON it returns in response to queries - looks almost exactly the same as Engage's `group` definitions.  Well, there's no coincidence there as we're the same people who built Engage and designed Magellan so it follows that we're going to have data structures that look similar.

However, these definitions are not exactly the same. For example:

```json
{
    <<---- Standard Engage stuff --->>
    "id": "{a15dfc12-3cfd-4667-8caf-a663cf210714}",
    "type": 1,
    "name": "LMR Channel 1",
    "cryptoPassword": "18ECC15EBC43D587653D1FA503AD8E29686488B7AE515E02AE262E0F2523F891",
    "rx": {
        "address": "239.1.2.3",
        "port": 42000
    },
        "tx": {
        "address": "239.1.2.3",
        "port": 42000
    },	 
    "txAudio": {
        "encoder": 1,
        "fdx": false,
        "framingMs": 20,
        "maxTxSecs": 86400
    },

    <<---- This is used by magsim --->>
    "magellan":{
        "enabled": true,
        "policy":[
            { "tx" : ["usdod"] },
            { "rx" : ["usdod", "ukmod"] },
            { "maxtx": 10 }
        ]
    }
}
```

Here you can see the top section of the JSON configuration for a group is the same as the items used to define an Engage group.  The lower section (`magellan`), though, is used by `magsim` to determine whether the group is Magellan-enabled (the `enabled` field in the `magellan` JSON object).  If the `enabled` field is set to `true`, then the group will be returned to the querying application.

Now, there's also a piece there for `policy`.  That isn't used by `magsim` right now but keep it around for the future because *real* Magellan systems match portions of the contents of the `policy` JSON object with tags embedded in a client's X.509 certificate.  For example, the `tx` and `rx` portions of the `policy` are lists of string *tags* that are checked against a client's certificate.  In the example, we have a tag of `usdod` for `tx`.  This ostensibly means that only certificates that have a `usdod` tag embedded in them will be granted transmit privileges on the group.  Also, for `rx`, we have have tags for `usdod` and `ukmod`.  This means that `usdod` and `ukmod` clients will have listen capabilities.

>There's actually a whole lot of settings that could go into the `magellan` section - and many of them being vendor-defined.  But it doesn't matter to you as a developer of a client application.  You'll never even see the `magellan` section returned to you as it's internal to the *server-side*.