//
//  Copyright (c) 2020 Rally Tactical Systems, Inc.
//  All rights reserved.
//

const fs = require('fs');
const dgram = require('dgram');
const { exit } = require('process');

// Install a SIGINT handler for Ctrl-C
process.on('SIGINT', function() 
{
    console.log("Stop requested!");
    process.exit();
});

// Load up certificate goodies
const caX509Certificate = fs.readFileSync('./certs/ca.crt');
const caX509Serial = fs.readFileSync('./certs/ca.serial').toString().replace('\n', '');
const serverX509Certificate = fs.readFileSync('./certs/server.crt');
const serverX509Key = fs.readFileSync('./certs/server.key');

// Advertising and other content
var groupDefFile = './groups.json';

// A Magellan "device" is a "thing" (because it can be a gateway, server, client, or something else), so
// we need some configuration for it.  Portions of that configuration is advertised and/or returned
// as part of REST responses.  Modify below as needed.
var thingInfo = {   'version': 234,
				    'dateTimeStamp': new Date(),
					'thingInfo': {
						'id': '{9e8412ef-44e8-48c4-9c2d-0e5b0aca9100}',
						'type': 'Radio Gateway',
						'manufacturer': 'Super Dooper Gateway Manufacturer',
						'capabilities': [
							'voice'
						],
					}
};

// SSDP advertising parameters.
const SSDP_MULTICAST_IP = '239.255.255.250';
const SSDP_BROADCAST_IP = '255.255.255.255';
const SSDP_PORT = 1900;

var ssdpIntervalSecs = 5;
var ssdpMulticast = true;
var ssdpBroadcast = true;

// Some other goodies
var useHttps = true;
var requireClientCert = true;
var machineAddress = '';
var restServerPort = 8081;

console.log('---------------------------------------------------------------------------');
console.log('Magellan Simulator (magsim) version 0.2');
console.log('');
console.log('Copyright (c) 2020 Rally Tactical Systems, Inc.');
console.log('---------------------------------------------------------------------------');


// Strip off the first two arguments - we don't care about them
var args = process.argv.slice(2);

// Process whatever arguments we have left
args.forEach((val) => {
	if(val.startsWith('-ma:')) {
		machineAddress = val.substr(4);
	}	
	else if(val.startsWith('-rsp:')) {
		restServerPort = parseInt(val.substr(5));
	}	
	else if(val.startsWith('-ssdpint:')) {
		ssdpIntervalSecs = parseInt(val.substr(9));
	}	
	else if(val == '-nossdpmc') {
		ssdpMulticast = false;
	}
	else if(val == '-nossdpbc') {
		ssdpBroadcast = false;
	}
	else if(val == '-nohttps') {
		useHttps = false;
	}
	else if(val == '-noclientcert') {
		requireClientCert = false;
	}
	else {
		console.error('ERROR: Unrecognized command-line option \'' + val + '\'');
		showSyntax();
		process.exit(1);
	}
});

// Check that we have an address to listen on
if(machineAddress == '') {
	console.error('ERROR: No machineAddress');
	showSyntax();
	process.exit(1);
}

// Check that we have a port to listen on
if(restServerPort <= 0) {
	console.error('ERROR: No or invalid restServerPort');
	showSyntax();
	process.exit(1);
}

const restServer = require(useHttps ? 'https' : 'http');

console.log('REST Server (' + (useHttps ? 'SECURED' : 'NON-SECURED') + ') listening on ' + machineAddress + ':' + restServerPort + '.');

if(ssdpMulticast || ssdpBroadcast) {
	if(ssdpMulticast) {
		console.log('SSDP multicast advertising every ' + ssdpIntervalSecs + ' seconds.');
	}
	if(ssdpBroadcast) {
		console.log('SSDP broadcast advertising every ' + ssdpIntervalSecs + ' seconds.');
	}
}
else {
	console.log('WARNING: SSDP advertising has been disabled');
}


console.log('');
console.log('Press Ctrl-C to stop.');
console.log('---------------------------------------------------------------------------');


// This is our little REST server.  It responds only to the "/config" REST request by
// returning the "restResponseJson" from above
if(useHttps) {
	// Setup options for our REST web server
	const httpsOptions = {
		// Our certificate which will be presented to clients
		cert: serverX509Certificate,
		// Our private key which helps us prove we are supposed to be in possession of the certificate
		key: serverX509Key,
		// The CA certificate against which clients certs will be verified
		ca: caX509Certificate,
		// Indicate whether we wanrt certificates from the client - i.e. mutual authentiation
		requestCert: requireClientCert,
		// We won't allow unauthorized clients
		rejectUnauthorized: true
	};

	restServer.createServer(httpsOptions, (req, res) => processHttpRequest(req, res)).listen(restServerPort);
}
else {
	restServer.createServer((req, res) => processHttpRequest(req, res)).listen(restServerPort);
}

// Process HTTP requests - we'll only respond to the "/config" query
function processHttpRequest (req, res) {
	// Assume a server-side error
	var retval = 500;

	// Our client may have a list of tokens which we'll use to filter the groups we return
	var clientCertTokens = []

	// Make sure the client is asking us for a URL we know about
	if(req.url == "/config") {		
		console.log('config request from ' + req.connection.remoteAddress);

		// See if we're asking for a client certificate and, if so, make sure
		// we have a subject
		if(useHttps && options.requestCert) {
			var cert = req.socket.getPeerCertificate();
			if(cert == undefined) {
				retval = 403;
				console.log('no client certificate');
			}
			else {
				var subject = cert.subject;
		
				if(subject == undefined) {
					retval = 403;
					console.log('no subject in client certificate');
				}
				else {
					// TODO: Extract the tokens from the certificate and plug into clientCertTokens 
					retval = 200;
					console.log('   cert: C=' + subject.C 
									+ ', O=' + subject.O
									+ ', L=' + subject.L);
				}
			}
		}
		else {
			retval = 200;
		}
	}
	else {
		retval = 404;
		console.log('unknown url ' + req.url);
	}

	if(retval == 200) {
		var responseJsonString = buildRestResponseJson(clientCertTokens);

		res.writeHead(retval, 
				{
					'Content-Type': 'application/json',
					'Content-Length': responseJsonString.length					
				}
		);

		res.end(responseJsonString);
	}
	else {
		res.writeHead(retval);		
		res.end("");
	}
}

// Create UDP sockets to send our SSDP advertisements
var ssdpMulticastSock = null;
var ssdpBroadcastSock = null;

if(ssdpMulticast) {
	ssdpMulticastSock = dgram.createSocket('udp4');
	ssdpMulticastSock.bind(0, machineAddress, function() {
		ssdpMulticastSock.setTTL(64);
		ssdpMulticastSock.setMulticastTTL(64);
	});
}

if(ssdpBroadcast) {
	ssdpBroadcastSock = dgram.createSocket('udp4');
	ssdpBroadcastSock.bind(0, machineAddress, function() {
		ssdpBroadcastSock.setTTL(64);
		ssdpBroadcastSock.setBroadcast(true);
	});
}

// Construct and send an SSDP advertisement packet
function advertiseViaSsdp() {
	var advert = 'NOTIFY * HTTP/1.1\r\n' +
			   	 'SERVER: MagellanGeneric/0.1.0 UPnP/1.0 ssdpd/1.7\r\n' +
			   	 'DATE: ' + dateForSsdp(new Date()) + '\r\n' +
			     'LOCATION: ' + (useHttps ? 'https' : 'http') + '://' + machineAddress + ':' + restServerPort + '/config\r\n' +
			     'ST: urn:rallytac-magellan:device:Gateway:1\r\n' +
			     'EXT:\r\n' +
			     'USN: uuid:' + thingInfo.thingInfo.id + '::urn:rallytac-magellan:device:Gateway:1\r\n' +
			     'CACHE-CONTROL: max-age=' + (ssdpIntervalSecs * 2) +'\r\n' +
			     'X-MAGELLAN-CV: ' + thingInfo.version + '\r\n' +
			     'X-MAGELLAN-ID: ' + thingInfo.thingInfo.id + '\r\n' +
				 'X-MAGELLAN-CA: ' + caX509Serial + '\r\n' +
				 '\r\n';
	
	try {
		if(ssdpMulticast) {
			ssdpMulticastSock.send(advert, 0, advert.length, SSDP_PORT, SSDP_MULTICAST_IP);	
		}
		if(ssdpBroadcast) {
			ssdpBroadcastSock.send(advert, 0, advert.length, SSDP_PORT, SSDP_BROADCAST_IP);	
		}
	}
	catch (e) {
		console.log(e);
	}	
}

// Send the SSDP advertisement immediately (really only useful for interactive demos)
if(ssdpMulticast || ssdpBroadcast) {
	advertiseViaSsdp();

	// ... and then send it periodically
	setInterval(advertiseViaSsdp, ssdpIntervalSecs * 1000)
}

// Show the usage syntax
function showSyntax() {
	const msg = '\nUsage: node magsim.js <options>\n\n' +
				'   options:\n' +
				'      -ma:machine_address ............................. specify the local machine IPv4 address (REQUIRED)\n' +
				'      [-rsp:rest_server_port] ......................... specify the REST server port to listen on (default is 8081)\n' +
				'      [-ssdpint:ssdp_advertising_interval_secs] ....... specify SSDP advertising interval in seconds (default is 5)\n' +
				'      [-nossdpmc] ..................................... disable SSDP advertising over IP multicast\n' +
				'      [-nossdpbc] ..................................... disable SSDP advertising over IP broadcatcast\n' +
				'      [-nohttps] ...................................... disable HTTPS\n' +
				'      [-noclientcert] ................................. disable client certificate request\n' +				
				'\n\n';
				
	console.log(msg);
}

// Build JSON for a Magellan "config" query from our groups definition file
function buildRestResponseJson(arrayOfClientTokens) {
	// Get the current JSON
	var groupsDefJson = JSON.parse(fs.readFileSync(groupDefFile));

	// Start out the Magellan 'thingInfo JSON object
	var rc = thingInfo;

	// Create/clear the 'talkGroups' array
	rc.talkGroups = [];

	// For each group we find ...
	groupsDefJson.groups.forEach((group) => {

		// ... if it has a Magellan object
		if(group.magellan != undefined) {

			// ... and the object says its enabled to be made available
			if(group.magellan.enabled) {

				// Convert "@host" addresses to this machine's IP address

				// TX
				if(group.tx) {
					if(group.tx == "@host") {
						group.tx = machineAddress;
					}						
				}

				// RX
				if(group.rx) {
					if(group.rx == "@host") {
						group.rx = machineAddress;
					}						
				}

				// Rallypoint(s)?
				if(group.rallypoints) {
					group.rallypoints.forEach((rp) => {
						if(rp.host.address == "@host") {
							rp.host.address = machineAddress;
						}						
					});
				}

				// Remove the Magellan goodies
				delete group['magellan'];

				if(group.comment != undefined) {
					delete group['comment'];
				}
				
				// Add to the array of groups being returned
				rc.talkGroups.push(group);
			}
		}
	})

	return JSON.stringify(rc);
}


// Formats a date for SSDP
function dateForSsdp(d) {
	var weekday = new Array(7);
	weekday[0] = "Sun";
	weekday[1] = "Mon";
	weekday[2] = "Tue";
	weekday[3] = "Wed";
	weekday[4] = "Thu";
	weekday[5] = "Fri";
	weekday[6] = "Sat";

	var month = new Array(12);
	month[0] = "Jan";
	month[1] = "Feb";
	month[2] = "Mar";
	month[3] = "Apr";
	month[4] = "May";
	month[5] = "Jun";
	month[6] = "Jul";
	month[7] = "Aug";
	month[8] = "Sep";
	month[9] = "Oct";
	month[10] = "Nov";
	month[11] = "Dec";

	return weekday[d.getUTCDay()] + ', ' +
			d.getUTCDate() + ' ' +
			month[d.getMonth()] + ' ' +
			(d.getYear() + 1900) + ' ' +
			("000" + d.getUTCHours()).slice(-2) + ':' +
			("000" + d.getUTCMinutes()).slice(-2) + ':' +
			("000" + d.getUTCSeconds()).slice(-2) + ' ' +
			'GMT';
}

