//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

// There's a LOT of logging happening so it can be difficult to pick out the
// lines produced by this module.  So ... we'll use the chalk module for coloring 
// our console text.  Install it wih "npm install chalk".  If you don't want colorized
// logging, just comment out the line below and modify the "logMsg()" function.
const chalk = require('chalk');

logMsg("=================================================================");
logMsg("engage-cmd for Node.js");
logMsg("Copyright (c) 2020 Rally Tactical Systems, Inc.");
logMsg("=================================================================");

// Our global Engage object - its "methods" closely match the API calls in the Engine
// Normally, we'd use "engage-engine" which Node would look for in node_modules.  But if
// we're developing or testing, we may want to override that and point to a specific
// location.  We'll do that with an environment variable named "ENGAGE_NODEJS_MODULE".
var engageModule = ""

try
{
        engageModule = process.env.ENGAGE_NODEJS_MODULE;        
}
catch
{
        engageModule = "engage-engine";
}

console.log(engageModule)

if(engageModule == undefined || engageModule == "")
{
    engageModule = "engage-engine";
}

console.log("engageModule=" + engageModule);

let engage = require(engageModule);

// Our TX priority and flags
var txPriority = 0;
var txFlags = 0;

// Load up the engine policy/general configuration
var enginePolicy = require("./sample_engine_policy.json");

// This is the user identity that we'll use for the Engine
var userIdentity = require("./sample_user_identity.json");

var restartOnStopped = false;

// TODO !!!
// What we should do here it to either generate or retrieve a previously-generated UUID 
// to identify this node and plug that into the userIdentity object.  If we don't do this, 
// the Engine will create a new node ID every time and we will show up as a new node everywhere 
// each time the Engine is started.

// *****************************************************************************
// For simplicity's sake, we'll just load a multicast or unicast mission package
// *****************************************************************************
var missionPackage = require("./sample_mission_multicast.json");
//var missionPackage = require("./sample_mission_unicast.json");

// ... it contains (amongst other goodies) an array of group objects
var groups = missionPackage["groups"];

logMsg("Loaded mission '" + missionPackage.name + "'");

// Fire up the Engine
fireUpTheEngine();

// Start up our little CLI - we'll go round and round here until "q"
// is entered.
var stdin = process.openStdin();
stdin.addListener("data", function(d) {
    var input = d.toString().trim();

    if(input == "?")
    {
        showHelp();
    }
    else if(input == "tc")
    {
        testCrypto();
    }
    else if(input == "q")
    {
        engage.stop();
    }
    else if(input.startsWith("c"))
    {
        var p = input.substring(1);
        if(p == "a")
        {
            createAllGroups();
        }
        else
        {
            createGroup(parseInt(p));
        }        
    }
    else if(input.startsWith("d"))
    {
        var p = input.substring(1);
        if(p == "a")
        {
            deleteAllGroups();
        }
        else
        {
            engage.deleteGroup(groups[parseInt(p)].id);
        }        
    }
    else if(input.startsWith("j"))
    {
        var p = input.substring(1);
        if(p == "a")
        {
            joinAllGroups();
        }
        else
        {
            engage.joinGroup(groups[parseInt(p)].id);
        }        
    }
    else if(input.startsWith("l"))
    {
        var p = input.substring(1);
        if(p == "a")
        {
            leaveAllGroups();
        }
        else
        {
            engage.leaveGroup(groups[parseInt(p)].id);
        }        
    }
    else if(input.startsWith("b"))
    {
        var p = input.substring(1);
        if(p == "a")
        {
            beginTxOnAllGroups();
        }
        else
        {
            engage.beginGroupTx(groups[parseInt(p)].id, txPriority, txFlags);
        }        
    }
    else if(input.startsWith("e"))
    {
        var p = input.substring(1);
        if(p == "a")
        {
            endTxOnAllGroups();
        }
        else
        {
            engage.endGroupTx(groups[parseInt(p)].id, txPriority, txFlags);
        }        
    }
    else if(input.startsWith("m"))
    {
        var p = input.substring(1);
        if(p == "a")
        {
            muteRxOnAllGroups();
        }
        else
        {
            engage.muteGroupRx(groups[parseInt(p)].id, txPriority, txFlags);
        }        
    }
    else if(input.startsWith("u"))
    {
        var p = input.substring(1);
        if(p == "a")
        {
            unmuteRxOnAllGroups();
        }
        else
        {
            engage.unmuteGroupRx(groups[parseInt(p)].id, txPriority, txFlags);
        }        
    }

    else if(input.startsWith("y"))
    {
        var p = input.substring(1);
        if(p == "a")
        {
            muteTxOnAllGroups();
        }
        else
        {
            engage.muteGroupTx(groups[parseInt(p)].id, txPriority, txFlags);
        }        
    }
    else if(input.startsWith("k"))
    {
        var p = input.substring(1);
        if(p == "a")
        {
            unmuteTxOnAllGroups();
        }
        else
        {
            engage.unmuteGroupTx(groups[parseInt(p)].id, txPriority, txFlags);
        }        
    }

    else if(input.startsWith("gm"))
    {
        var params = input.substring(2);
        var tokens = params.split(",");

        var passphrase = tokens[0].trim();

        if(passphrase == "?")
        {
            console.log("example for 4 channels: gm theDogHowlsAtTheMoon,4,demo.rallytac.com,My Example Mission");
        }
        else
        {
            var groupCount = parseInt(tokens[1].trim());
            var rallypoint = tokens[2].trim();
            var missionName = tokens[3].trim();
            var json = engage.generateMission(passphrase, groupCount, rallypoint, missionName);
    
            console.log("passphrase='" + passphrase + "', groupCount=" + groupCount + ", rallypoint='" + rallypoint + "', missionName='" + missionName + "'");
            console.log(json);
        }
    }

    else if(input.startsWith("r"))
    {
        restartTheEngine();
    }

    else
    {
        if( input != "" )
        {
            console.log("unknown command '" + input + "'");
        }
    }
});


//--------------------------------------------------------
function showHelp()
{
    console.log("?................... help");
    console.log("q................... quit");
    console.log("r................... restart");
    console.log("tc.................. test crypto");
    console.log("ca||c<n>............ create all groups || group n");
    console.log("da||d<n>............ delete all groups || group n");
    console.log("ja||j<n>............ join all groups || group n");
    console.log("la||l<n>............ leave all groups || group n");
    console.log("ba||b<n>............ begin tx on all groups || group n");
    console.log("ea||e<n>............ end tx on all groups || group n");
    console.log("ma||m<n>............ mute rx on all groups || group n");
    console.log("ua||m<n>............ unmute rx on all groups || group n");
    console.log("ya||m<n>............ mute tx on all groups || group n");
    console.log("ka||m<n>............ unmute tx on all groups || group n");
    console.log("gm pp,gc,rp,mn...... generate mission for passphrase 'pp', with 'gc' groups, 'rp' rallypoint, named 'mn'");
}

//--------------------------------------------------------
function fireUpTheEngine()
{
    setupEventHandlers();
    engage.setLogLevel(3);
    engage.initialize(JSON.stringify(enginePolicy), JSON.stringify(userIdentity), "");
    engage.start();    
}

function restartTheEngine()
{
    restartOnStopped = true;
    engage.stop();
}

//--------------------------------------------------------
function setupEventHandlers()
{
    // Engine events
    engage.on("engineStarted", function(eventExtraJson) {
        logMsg("engineStarted" + ", x='" + eventExtraJson + "'");
        createAllGroups();
        joinAllGroups();
    });

    engage.on("engineStopped", function(eventExtraJson) {
        logMsg("engineStopped" + ", x='" + eventExtraJson + "'");
        engage.shutdown();

        if( restartOnStopped )
        {
            restartOnStopped = false;
            fireUpTheEngine();
        }
        else
        {
            process.exit();
        }
    });

    
    // Rallypoint events
    engage.on("rpPausingConnectionAttempt", function(id, eventExtraJson) {
        logMsg("rpPausingConnectionAttempt: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("rpConnecting", function(id, eventExtraJson) {
        logMsg("rpConnecting: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("rpConnected", function(id, eventExtraJson) {
        logMsg("rpConnected: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("rpDisconnected", function(id, eventExtraJson) {
        logMsg("rpDisconnected: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("rpRoundtripReport", function(id, rtMs, rtQualityRating, eventExtraJson) {
        logMsg("rpRoundtripReport: " + id + ", rtMs=" + rtMs + ", rtQualityRating=" + rtQualityRating + ", x='" + eventExtraJson + "'");
    });

    // Group events
    engage.on("groupCreated", function(id, eventExtraJson) {
        logMsg("groupCreated: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupCreateFailed", function(id, eventExtraJson) {
        logMsg("groupCreateFailed: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupDeleted", function(id, eventExtraJson) {
        logMsg("groupDeleted: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupJoined", function(id, eventExtraJson) {
        logMsg("groupJoined: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupJoinFailed", function(id, eventExtraJson) {
        logMsg("groupJoinFailed: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupLeft", function(id, eventExtraJson) {
        logMsg("groupLeft: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupConnected", function(id, eventExtraJson) {
        logMsg("groupConnected: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupConnectFailed", function(id, eventExtraJson) {
        logMsg("groupConnectFailed: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupDisconnected", function(id, eventExtraJson) {
        logMsg("groupDisconnected: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupRxStarted", function(id, eventExtraJson) {
        logMsg("groupRxStarted: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupRxEnded", function(id, eventExtraJson) {
        logMsg("groupRxEnded: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupRxMuted", function(id, eventExtraJson) {
        logMsg("groupRxMuted: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupRxUnmuted", function(id, eventExtraJson) {
        logMsg("groupRxUnmuted: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupRxSpeakersChanged", function(id, speakers, eventExtraJson) {
        logMsg("groupRxSpeakersChanged: " + id + ", speakers=" + speakers + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupNodeDiscovered", function(id, nodeJson, eventExtraJson) {
        logMsg("groupNodeDiscovered: " + id + ", nodeJson=" + nodeJson + ", x='" + eventExtraJson + "'");

        let node = JSON.parse(nodeJson);
        logMsg("node: nodeId=" + node.identity.nodeId
                    + ", userId=" + node.identity.userId
                    + ", displayName=" + node.identity.displayName 
                    + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupNodeRediscovered", function(id, nodeJson, eventExtraJson) {
        logMsg("groupNodeRediscovered: " + id + ", nodeJson=" + nodeJson + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupNodeUndiscovered", function(id, nodeJson, eventExtraJson) {
        logMsg("groupNodeUndiscovered: " + id + ", nodeJson=" + nodeJson + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupTxStarted", function(id, eventExtraJson) {
        logMsg("groupTxStarted: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupTxEnded", function(id, eventExtraJson) {
        logMsg("groupTxEnded: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupTxFailed", function(id, eventExtraJson) {
        console.log("groupTxFailed: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupTxUsurpedByPriority", function(id, eventExtraJson) {
        logMsg("groupTxUsurpedByPriority: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupMaxTxTimeExceeded", function(id, eventExtraJson) {
        logMsg("groupMaxTxTimeExceeded: " + id + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupAssetDiscovered", function(id, assetJson, eventExtraJson) {
        logMsg("groupAssetDiscovered: " + id + ", assetJson=" + assetJson + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupAssetRediscovered", function(id, assetJson, eventExtraJson) {
        logMsg("groupAssetRediscovered: " + id + ", assetJson=" + assetJson + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupAssetUndiscovered", function(id, assetJson, eventExtraJson) {
        logMsg("groupAssetUndiscovered: " + id + ", assetJson=" + assetJson + ", x='" + eventExtraJson + "'");
    });

    engage.on("groupTimelineEventStarted", function(id, eventJson, eventExtraJson) {
        logMsg("groupTimelineEventStarted: " + id + ", eventJson=" + eventJson + ", x='" + eventExtraJson + "'");
    });    

    engage.on("groupTimelineEventUpdated", function(id, eventJson, eventExtraJson) {
        logMsg("groupTimelineEventUpdated: " + id + ", eventJson=" + eventJson + ", x='" + eventExtraJson + "'");
    });    

    engage.on("groupTimelineEventEnded", function(id, eventJson, eventExtraJson) {
        logMsg("groupTimelineEventEnded: " + id + ", eventJson=" + eventJson + ", x='" + eventExtraJson + "'");
    });    

    engage.on("groupTimelineReport", function(id, reportJson, eventExtraJson) {
        logMsg("groupTimelineReport: " + id + ", reportJson=" + reportJson + ", x='" + eventExtraJson + "'");
    });    

    engage.on("groupTimelineReportFailed", function(id, eventExtraJson) {
        logMsg("groupTimelineRgroupTimelineReportFailedeport: " + id + ", x='" + eventExtraJson + "'");
    });    

    
    // Licensing events
    engage.on("licenseExpired", function(eventExtraJson) {
        logMsg("licenseExpired" + ", x='" + eventExtraJson + "'");
    });

    engage.on("licenseChanged", function(eventExtraJson) {
        logMsg("licenseChanged" + ", x='" + eventExtraJson + "'");
    });

    engage.on("licenseExpiring", function(secondsFromNow, eventExtraJson) {
        logMsg("licenseExpiring" + ", x='" + eventExtraJson + "'");
    });
}

//--------------------------------------------------------
function logMsg(msg)
{
    var m = new Date();
    var dateString = m.getUTCFullYear() +"/"+ (m.getUTCMonth()+1) +"/"+ m.getUTCDate() + " " + m.getUTCHours() + ":" + m.getUTCMinutes() + ":" + m.getUTCSeconds();

    console.log(chalk.gray("Nodejs-" + dateString + "....." + msg));
}

//--------------------------------------------------------
function createAllGroups()
{
    for(var groupIndex in groups)
    {
        var group = groups[groupIndex];
        createGroup(groupIndex);
    }    
}

//--------------------------------------------------------
function createGroup(groupIndex)
{
    // Get the groupo object from our array of groups
    var group = groups[groupIndex];

    // Add in the alias
    group.alias = userIdentity.alias;

    // Pass the JSON configuration for the group.  Once this is done,
    // we'll only ever refer to the group by it's ID.
    engage.createGroup(JSON.stringify(group))
}

//--------------------------------------------------------
function deleteAllGroups()
{
    for(var groupIndex in groups)
    {
        engage.deleteGroup(groups[groupIndex].id)
    }    
}

//--------------------------------------------------------
function joinAllGroups()
{
    for(var groupIndex in groups)
    {
        engage.joinGroup(groups[groupIndex].id);
    }    
}

//--------------------------------------------------------
function leaveAllGroups()
{
    for(var groupIndex in groups)
    {
        engage.leaveGroup(groups[groupIndex].id);
    }    
}

//--------------------------------------------------------
function beginTxOnAllGroups()
{
    for(var groupIndex in groups)
    {
        engage.beginGroupTx(groups[groupIndex].id, txPriority, txFlags);
    }    
}

//--------------------------------------------------------
function endTxOnAllGroups()
{
    for(var groupIndex in groups)
    {
        engage.endGroupTx(groups[groupIndex].id);
    }    
}

//--------------------------------------------------------
function muteRxOnAllGroups()
{
    for(var groupIndex in groups)
    {
        engage.muteGroupRx(groups[groupIndex].id);
    }    
}

//--------------------------------------------------------
function unmuteRxOnAllGroups()
{
    for(var groupIndex in groups)
    {
        engage.unmuteGroupRx(groups[groupIndex].id);
    }    
}

//--------------------------------------------------------
function muteTxOnAllGroups()
{
    for(var groupIndex in groups)
    {
        engage.muteGroupTx(groups[groupIndex].id);
    }    
}

//--------------------------------------------------------
function unmuteTxOnAllGroups()
{
    for(var groupIndex in groups)
    {
        engage.unmuteGroupTx(groups[groupIndex].id);
    }    
}

//--------------------------------------------------------
function toHexString(byteArray) 
{
    return Array.from(byteArray, function(byte) 
    {
      return ('0' + (byte & 0xFF).toString(16)).slice(-2);
    }).join(' ')
}

function testCrypto()
{
    logMsg("Testing encryption/decryption using Engage's crypto module");

    // This is *NOT* the encryption key but, rather, the hex representation of
    // the *password* used by Engage to _derive_ an encryption key using the
    // PBKDF2 algorithm - https://en.wikipedia.org/wiki/PBKDF2
    const pbkdf2PasswordForEncryption = "adac22000d3f46829471f75515907fde";

    // Our original data - it needs to be a Buffer of unsigned bytes
    const original = Buffer.from([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]); 

    // Print out the original 
    logMsg("original data length  : " + original.length + " bytes")
    logMsg("original data         : " + toHexString(original));

    // Encrypt and print
    const encrypted = engage.encrypt(original, 0, original.length, pbkdf2PasswordForEncryption);
    logMsg("encrypted data length : " + encrypted.length + " bytes")
    logMsg("encrypted data        : " + toHexString(encrypted));

    // Decrypt our data
    const decrypted = engage.decrypt(encrypted, 0, encrypted.length, pbkdf2PasswordForEncryption);
    logMsg("decrypted data length : " + decrypted.length + " bytes")
    logMsg("decrypted data        : " + toHexString(decrypted));

    if(decrypted.length == original.length)
    {
        var errorFound = false;
        for(x = 0; x < decrypted.length; x++)
        {
            if(decrypted[x] != original[x])
            {
                errorFound = true;
                break;
            }
        }

        if(!errorFound)
        {
            logMsg("encryption & decryption test passed");
        }
        else
        {
            logMsg("encryption & decryption test failed due to decryption mismatch with original");
        }
    }
    else
    {
        logMsg("ERROR:  decrypted length does not match original length");
    }
}
