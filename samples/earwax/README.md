
# earwax : A Simple REST Service and client agent for archiving Engage recordings
When you use the Engage Activity Recorder (`eard`), those recordings generally reside on the same machine where `eard` is running.  And you could have many of these recorders distributed throughout your network.  You'd most likely want to archive those recordings on a centralized system which provides a single place to search and retrieve recordings - such as on a cloud or enterprise server.

So, for fun, we built a simple archival application that offers up an API via a [REST](https://en.wikipedia.org/wiki/Representational_state_transfer) interface over HTTPS.  We call it "`earwax`" (because "stuff" gets stored there in a "sticky" fashion - yuck!).

`earwax` is a pretty straightforward Javascript application fully contained in a single file (`earwax.js`) run by [Node.js](https://en.wikipedia.org/wiki/Node.js), and is designed to serve as a multi-tenant, centralized storage web microservice.  Because its built using Javascript on top of Node.js and is provided in open-source form, you can use `earwax` for your own purposes with no licensing requirements and are free to modify it as you please.

>To be completely open here: this is the first real Node.js web service we've built and so professional Javascript/Node.js folks will likely look at this code and not have kind things to say.  Nonetheless, here it is - use it as you please.

Here's a few things that `earwax` is and does:
- Open-source Javascript using Node.js
- Uses SQlite for its database
- Uses the local file system for file storage
- Supports HTTPS with mutual authentication
- Supports multi-tenency through API keys

To configure this thing you'll only need one JSON file: `earwax_conf.json`.  It looks like this:
```javascript
{
  "id":"idOfThisService",
  "localStorageRoot": "./.data",              

	"uris":{
		"apiVersion": "v1",
		"postSingle": "/${apiVersion}/archive/recording/",
		"getSingle": "/${apiVersion}/archive/recording/",
		"getSingleRaw": "/${apiVersion}/archive/raw/",
		"getMultiple": "/${apiVersion}/archive/recordings"
	},

	"https":{
		"host":"",                          
		"port":4343,                        
		"certPem":"./EarRestServer.cert",   
		"keyPem":"./EarRestServer.key",     
		"ca":[                              
			"./EarCA.cert"
		]
	},

	"timers":{
		"cleanupIntervalSecs":600
	},                                      

	"logging":{
		"level":4                           
	},

	"tenants":[
		{
			"id":"29FF17400ACA45128158DC451507B550",
			"maxEventAgeHours":168,
			"apiKeys":[
				{
					"key":"83535939aabd4b50a1a6ff6215099dce521a8fd7dd8e4dd9ace7b59478627600",
					"permissions": [
						"POST",
						"GET"
					]
				},
				{
					"key":"029ea4a0ef1046e9b7b33ee4d1a0537f3b506f76df2645088a46c9c2e37ca4ed",
					"permissions": [
						"POST"
					]
				},
				{
					"key":"7548b736f26545108d32bd1174887f18e33c815709464bc4a5a29c5c9a4a2b2c",
					"permissions": [
						"GET"
					]
				}
			]
		},
		{
			"id":"E3665FA8745B4D92AD1BEDD37197E59B",
			"maxEventAgeHours":168,
			"apiKeys":[
				{
					"key":"9d486e2d2cf44d14ad3571fd763fa1d168aab9fee3b74037b9fa8b24718fcdab",
					"permissions": [
						"POST",
						"GET"
					]
				}				
			]
		},
		{
			"id":"DFD2F5D61AD04700A5F2705911A2ADD7",
			"maxEventAgeHours":168,
			"apiKeys":[
				{
					"key":"db891fc6999143a396d12fa78d2432ace9973e1aed76418394aa06d5545a09b1",
					"permissions": [
						"POST",
						"GET"
					]
				}				
			]
		}
	]
}
```

* `id` : A unqiue identifier for the instance.  Machine FQDN works fine.
* `localStorageRoot` : Root directory for data storage on the local file system.
* `uris` : Configuration for the REST resource URIs to be used by clients.
  * `apiVersion` : The "version" of the REST API.
  * `postSingle` : URI to be used by clients uploading (POST'ing) to the service.
  * `getSingle` : Base URI used by clients to request JSON data for a single item.  For example, a request for an item identified by `01596a17a271a3517cebda731cba694071510731828fc0b23cda7afd10962ae2` would have a formatted URL of `https://server.name:port/v1/archive/recording/01596a17a271a3517cebda731cba694071510731828fc0b23cda7afd10962ae2`.
  * `getSingleRaw` : Base URI used by clients to request the raw binary data for a single item.  The returned data is `application/octet-stream`.
  * `getMultiple` : URI used by clients to perform complext queries.  This URI is followed by a `"?"` and then a query string formatted as described below in [GET query parameters]().
* `https` : HTTPS configuration:
  * `host` : The host interface on which to listen - blank for all.
  * `port` : TCP port.
  * `certPem` : Server certificate (PEM format).
  * `keyPem` : Server private key (PEM format).
  * `ca` : List of CA certificates to validate clients.
* `timers` : Timers
  * `cleanupIntervalSecs` : Interval (seconds) at which to run archive cleanup - the default is 60 (every minute).
* `logging` : Logging configuration.
  * `level` : Logging level: 0 = FATAL, 1 = ERROR, 2 = WARNING, 3 = INFORMATION, 4 = DEBUG.
* `tenants` : List of tenants on the system where each element is as follows:
  * `id` : The unique ID for the tenant.  All data for the tenant - including their database - is privately stored relative to a directory named for the `id` under `localStorageRoot`.  For example, the first tenant will have all their data stored in `./.data/29FF17400ACA45128158DC451507B550` while the second tenant's data will go to `./.data/E3665FA8745B4D92AD1BEDD37197E59B`.
  * `maxEventAgeHours`: Maximum age (hours) of recordings before being purged.
  * `apiKeys` : List of valid API keys for the tenant.  Each entry is as follows:
    * `key` : A unique ID for the key.
    * `permissions`: A list of permissions (strings) that indicate what permissions are available for the API key.  Valid values are `POST`, indicating that the API key allows uploading events, and `GET` indicating that the API key allows for querying and downloading of events.

> NOTE: The tenant ID must never be shared outside of the server as it (at least indirectly) is an identifier of the location of data objects stored on behalf of the tenant.  Client applications must always only use the API keys associated with the tenant.

Notice how the configuration above has 3 API keys for the first tenant.  This is a great example of how you can set things up to allow some applications to uploaded and download data (the first API key allowing for `POST` and `GET`) while others can either only upload (the second API key allowing only for `POST`) or download (the third API key which allows only for `GET`).
## Setting things up
Once you've setup your configuration, you'll actually want to run it.  To do that you're going to need to install some depencies for Node.js (we'll assume you already have Node.js installed).

You can install the dependencies manually or simply have npm do it.
```shell
$ npm install
```

With the dependencies installed we can now run
## Running the server
```shell
$ npm start

---------------------------------------------------------------------------------------------------
Engage Activity Recorder REST Service 0.1
Copyright (c) 2020 Rally Tactical Systems, Inc.

{
  id: 'idOfThisService',
  localStorageRoot: './.data',
  uris: {
    apiVersion: 'v1',
    postSingle: '/v1/archive/recording/',
    getSingle: '/v1/archive/recording/',
    getSingleRaw: '/v1/archive/raw/',
    getMultiple: '/v1/archive/recordings'
  },
  https: {
    host: '',
    port: 4343,
    certPem: './EarRestServer.cert',
    keyPem: './EarRestServer.key',
    ca: [ './EarCA.cert' ]
  },
  timers: { cleanupIntervalSecs: 600 },
  logging: { level: 4 },
  tenants: [
    {
      id: '29FF17400ACA45128158DC451507B550',
      maxEventAgeHours: 27,
      apiKeys: [Array],
      db: Database {}
    },
    {
      id: 'E3665FA8745B4D92AD1BEDD37197E59B',
      maxEventAgeHours: 168,
      apiKeys: [Array],
      db: Database {}
    },
    {
      id: 'DFD2F5D61AD04700A5F2705911A2ADD7',
      maxEventAgeHours: 168,
      apiKeys: [Array],
      db: Database {}
    }
  ]
}
---------------------------------------------------------------------------------------------------
```

## Certificates
`earresrtd` conducts all transactions over TLS and uses X.509 certificates for ***mutual*** authentication.  This means that not only does the server side need to provide a valid certificate to the client, the server will also require a certificate from the client.

### Server-Side Certificate Configuration
Configuring the server for certificates is pretty straightforward.  You'll need to set the `certPem`, `keyPem`, and `ca` elements of the `https` JSON configuration subobject as follows:

```javascript
    "https":{
        "host":"",
        "port":4343,
        "certPem":"./EarRestServer.cert",       <-- The server certificate in PEM format
        "keyPem":"./EarRestServer.key",         <-- The server private key in PEM format
        "ca":[                                  <-- An array of CA certificates used to
            "./EarCA.cert"                      <-- validate client certificates
        ]
    }
```

While the `certPem` and `keyPem` are self-explanatory, the `ca` element needs some explanation...

Basically, because the server is requesting a client's certificate, it needs to validate that certificate.  And the way that this is done is for the server to see if the client's certificate (presented when the client makes a connection to the server) has been signed by a Certificate Authority (CA) that the server trusts.  The `ca` element, therefore, specifies the file(s) that contain all the CA certificates for all the clients that would connect to the server.

For example:  Let's say a client is using a certificate signed by `EarCA.cert`.  In order for the server to validate the client certificate, it will check to see if that certificate has been signed by `EarCA.cert`.  In our example, this will work just fine because our `ca` element lists `EarCA.cert` as a CA certificate file.

But, what if the client presents a certificate signed by another CA - let's call it "Joe Schmoe Certificate Authority".  Well, when that certificate gets to the server, the server is going to deny entry to the client.

Now, if we want to allow clients that use Joe Schmoe's certificate authority, we're going to have to get the CA certificate that Joe uses to create the certificates he issues.  This would be a public CA file (in PEM format) and there should not be a problem obtaining it.

Once we have Joe's CA certificate - let's say the file name is `JoeSchmoeCA.cert` - all we need to do is add that file to the `ca` element - like this:

```javascript
	"https":{
		"host":"",
		"port":4343,
		"certPem":"./EarRestServer.cert",
		"keyPem":"./EarRestServer.key",
		"ca":[
            "./EarCA.cert",
            "./JoeSchmoeCA.cert"
		]
	}
```

From now on, clients presenting certificates issued by Joe Schmoe will be accepted.
>You can add as many CA certificates as you need.  There is no practical limit.

### Client-Side Certificates
Configuring certificates on the client side is, of course, dependent on the nature of the client application so there's not too much to say here.  However, some things to bear in mind:
- The client will need a certificate ***and*** the associated private key.  Make sure to keep both - but especially the private key - safe.
- The certificate used by the client must be issued by a CA that the server trusts.  See above for more information.
- In order to verify the server's certificate, the client needs to have the CA certificate of the authority that issued the server's certificate.  How this is configured is obviously dependent on your client software.  (See further down for an example of how this is done using `curl` on the command-line.)

>In this directory you'll find a little script named `mkcert.sh` which uses OpenSSL to create CA certificates and issue server and/or client certificates using those CA certificates.  It's a great way to try out issuing your own certificates.

# Using the service
Use of the service is, of course, over HTTPS and therefore requires clients to support it.

Clients that will be uploading recordings or querying for recordings require two items of key importance:
1. An X.509 certificate.  This is a certificate as described above.
2. An API key issued by the server administrator.  The API key is a unique string associated with a tenant identifier that gives access to the tenant's dataspace without comprimising the tenant ID.  For each API key associated with a tenant, the API key further defines what operation(s) (`POST` or `GET`) can be conducted. 

All HTTP requests made by clients must include the API key in an HTTP header named `apikey` (case-sensitive).  Requests without the API key are immediately denied.  Therefore using a web browser to test the service will not result in happiness.  Rather, test with something like [cURL](https://curl.se/).

## POSTs and GETs

### Uploading recordings (`POST`)
Uploading recordings can be done by any client capable of sending an FORM object via an HTTP POST.  In addition to the HTTP header `"apikey"` containing a valid API ID and, of course, all the X.509 security goodies in place, the client need only POST a form with the following fields:

- `"recordingNodeId"` : A unique identifier for the recording node - up to 38 characters, a GUID is recommended.
- `"meta"` : The JSON metadata associated with the event file.
- `"fileupload"` : The binary content of the event file.

#### Uploading with `earbud` ("*cleaning out the ear*" if you will)
A good example to look at for uploading is the `earbud.py` Python script in this directory.  If what you want to do is periodically archive events from your local EAR timeline database to `earwax`, look no further.

This little guy will check your local database on an interval; looking for unarchived event records. The Engage Engine underlying the Engage Activity Recorder maintains fields in each record named `"in_progress` and `"archived"` for this exact purpose. `earbud` simply checks the database periodically for records that are not in progress and have not yet been archived.  For each of those, it grabs the metadata associated with the event and sets up an HTTP POST of the data using the [PycURL](http://pycurl.io) library.

The command-line for `earbud` is as follows:

```shell
$ python earbud.py -db:<database_file> -node:<nodeId> -api:<apikey> -cert:<certificate> -key:<private_key> -url:<archive_url> [-ca:<server_ca_certificate>] [-int:<archive_interval_secs>] [-maxe:<max_events_per_interval>] [-verbose] [-insecure]
```

- `-db:` : The full path to local the EAR database.
- `-node:` : The ID of the recording node.
- `-api:` : The API key.
- `-cert:` : The client certificate.
- `-key:` : The client private key.
- `-url:` : The server URL.
- `-ca:` : [OPTIONAL] The CA certificate to validate the server certificate.
- `-int:` : [OPTIONAL] The interval (in seconds) at which to run.  If you provide a value here, `earbud` will stay running until you end the process, checking periodically for new events that are ready to be archived.
- `-maxe:` : The maximum number of events to process (up to 32) on every database check. (Useful for testing, debugging, and limiting bandwidth usage.)
- `-verbose` : Have the script print out lots of info as it does it's thing.
- `-insecure` : Bypass security and validation checks against the server certificate.  This is not advised for production but very useful during testing.

Here's a full example command-line for `earbud`.

Now, what we want to do is monitor the local EAR database at `./eardata/eardb.sqlite`, checking it every `30` seconds for new events.  Once we have an event, we'll upload it to a machine named `central.archive.storage` on port `4343`.  Our client certificate is `./EarRestClient.cert` and its private key is `./EarRestClient.key`.  We're identifying the local node as `{35bb350b-2750-4ddb-bbd7-3047cc016b54}` and will be using our assigned API key of `29FF17400ACA45128158DC451507B550`.

The server is using certificates issued by "Some Custom Certificate Authority" so, to validate the server's certificate, we're telling `earbud` to use `./SomeCustomCA.cert` for validationn.

```shell
python earbud.py \
        -db:./eardata/eardb.sqlite \
        -node:{35bb350b-2750-4ddb-bbd7-3047cc016b54} \
        -api:7548b736f26545108d32bd1174887f18e33c815709464bc4a5a29c5c9a4a2b2c \
        -int:30 \
        -url:https://central.archive.storage:4343/archive/recording \
        -cert:./EarRestClient.cert \
        -key:./EarRestClient.key \
        -ca:./SomeCustomCA.cert \
        -verbose
```

### Querying & downloading recordings (`GET`)
- GET details (JSON) for a single recording, 404 if <item_key> does not exist
```shell
/archive/recording/<item_key>
```
- GET raw content (binary) for a single recording, 404 if <item_key> does not exist
```shell
/archive/recording/<item_key>/raw
```

- GET details (JSON) based on a query - one or more elements may be returned
```shell
/archive/recordings/<item_key>[?<query_parameters>]
```
### GET query parameters

`<query_parameters>` is a URL query string formatted as individual JSON objects where each object has a name that corresponds to a field in the database record with an operator applied to it.

Operators are:
- `"eq"` : equals
- `"ne"` : not equals
- `"gt"` : greater than
- `"gte"`: greater than or equal
- `"lt"` : less than
- `"lte"`: less than or equal

For example, let's say we're looking for records where the "started" field (a UNIX timestamp in UTC time) of the event is on or after April 1, 2020 at 21:22:38 (UTC time).  The UNIX timestamp for that date and time is 1585776158000.  The query string element would be:

```javascript
started={"gte":1585776158000}
```

If we further wanted to filter the results to give us only events that were longer than 2500 milliseconds; the query string would be:

```javascript
started={"gte":1585776158000}&durationMs={"gt":2500}
```

We can also limit the number of items returned by specifying a "limit".  Let's say we want only up to 10 elements returned for the query above.  Our query string would be:

```javascript
started={"gte":1585776158000}&durationMs={"gt":2500}&limit=10
```
>`limit`'s value is ***NOT*** a JSON structure, just a value.  Also, `limit` cannot be more than the maximum number of elements this program is configured to return in RESPONSE_ROW_LIMIT (whcih is 64)

All put together, our querying URL looks something like this:
```javascript
https://server:port/v1/archive/recordings?started={"gte":1585776158000}&durationMs={"gt":2500}&limit=10
```

Fields available for query are:
- `itemKey` : string identifying the element
- `recordingNodeId` : string (GUID) identifying the Engage Activity Recorder node that recorded the event
- `eventId` : string (GUID) identifying the event as assigned by the Engage Engine on the Engage Activity Recorder node
- `groupId` : string (GUID) identifying the Engage group
- `type` : integer identifying the type of event
- `direction` : integer denoting the direction of the event (RX = 1, TX =2)
- `started` : integer denoting UNIX UTC timestamp in milliseconds when the event started (as per the clock on the Recorder node)
- `ended` : integer denoting UNIX UTC timestamp in milliseconds when the event ended (as per the clock on the Recorder node)
- `durationMs` : integer representing the duration of the event in milliseconds (essentially "ended - started")
- `nodeId` : string (GUID) identifying the Engage node that made the transmission
- `alias` : string identifying the transmitting user's alias
- `flags` : integer denoting RXTX flags associated with the transmission


## cURL
You can run pretty simple `GET` tests using cURL.  Some things to remember:
1. Don't forget to provide the certificate and private key.  
2. If the server is using a self-signed certificate you likely need to specify `--insecure` 
3. If the server is using a a certificate issued by a CA not already trusted by cURL, you'll need to provide cURL with the server's CA certificate with the `--cacert` parameter:

 
```shell
curl --insecure \
     -H "apikey: 7548b736f26545108d32bd1174887f18e33c815709464bc4a5a29c5c9a4a2b2c" \
     --cert ./EarRestClient.cert \
     --key ./EarRestClient.key \
     --cacert ./EarCA.cert \
     https://localhost:4343/v1/archive/recordings
```

