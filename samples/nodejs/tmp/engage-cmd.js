//
//  Copyright (c) 2018 Rally Tactical Systems, Inc.
//  All rights reserved.
//

// There's a LOT of logging happening so it can be difficult to pick out the
// lines produced by this module.  So ... we'll use the chalk module for coloring 
// our console text.  Install it wih "npm install chalk".  If you don't want colorized
// logging, just comment out the line below and modify the "logMsg()" function.
const chalk = require('chalk');

logMsg("=================================================================");
logMsg("engage-cmd for Node.js");
logMsg("Copyright (c) 2018 Rally Tactical Systems, Inc.");
logMsg("=================================================================");

// Our global Engage object - its "methods" closely match the API calls in the Engine
let engage = require("engage-engine");

// Our TX priority and flags
var txPriority = 0;
var txFlags = 0;

// Load up the engine policy/general configuration
var enginePolicy = require("./sample_engine_policy.json");

// This is the user identity that we'll use for the Engine
var userIdentity = require("./sample_user_identity.json");

// TODO !!!
// What we should do here it to either generate or retrieve a previously-generated UUID 
// to identify this node and plug that into the userIdentity object.  If we don't do this, 
// the Engine will create a new node ID every time and we will show up as a new node everywhere 
// each time the Engine is started.

// Fire up the Engine
fireUpTheEngine();

// Load the mission package ...

// *****************************************************************************
// For simplicity's sake, we'll just load a multicast or unicast mission package
// *****************************************************************************
var missionPackage = require("./sample_mission_multicast.json");
//var missionPackage = require("./sample_mission_unicast.json");

// ... it contains (amongst other goodies) an array of group objects
var groups = missionPackage["groups"];

logMsg("Loaded mission '" + missionPackage.name + "'");

// Create all the groups here (because we do it often when testing)
createAllGroups();

// Join all the groups here (because we do it often when testing)
joinAllGroups();

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
        engage.disableCallbacks()
        showdownTheEngine();        
        process.exit();  
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
    console.log("tc.................. test crypto");
    console.log("ca||c<n>............ create all groups || group n");
    console.log("da||d<n>............ delete all groups || group n");
    console.log("ja||j<n>............ join all groups || group n");
    console.log("la||l<n>............ leave all groups || group n");
    console.log("ba||b<n>............ begin tx on all groups || group n");
    console.log("ea||e<n>............ end tx on all groups || group n");
    console.log("ma||m<n>............ mute rx on all groups || group n");
    console.log("ua||m<n>............ unmute rx on all groups || group n");
}

//--------------------------------------------------------
function fireUpTheEngine()
{
    setupEventHandlers();
    engage.setLogLevel(4);
    engage.initialize(JSON.stringify(enginePolicy), JSON.stringify(userIdentity), "");
    engage.start();    
}

//--------------------------------------------------------
function setupEventHandlers()
{
    // Engine events
    engage.on("engineStarted", function() {
        logMsg("engineStarted");
    });

    engage.on("engineStopped", function() {
        logMsg("engineStopped");
    });

    
    // Rallypoint events
    engage.on("rpPausingConnectionAttempt", function(id) {
        logMsg("rpPausingConnectionAttempt: " + id);
    });

    engage.on("rpConnecting", function(id) {
        logMsg("rpConnecting: " + id);
    });

    engage.on("rpConnected", function(id) {
        logMsg("rpConnected: " + id);
    });

    engage.on("rpDisconnected", function(id) {
        logMsg("rpDisconnected: " + id);
    });


    // Group events
    engage.on("groupCreated", function(id) {
        logMsg("groupCreated: " + id);
    });

    engage.on("groupCreateFailed", function(id) {
        logMsg("groupCreateFailed: " + id);
    });

    engage.on("groupDeleted", function(id) {
        logMsg("groupDeleted: " + id);
    });

    engage.on("groupJoined", function(id) {
        logMsg("groupJoined: " + id);
    });

    engage.on("groupJoinFailed", function(id) {
        logMsg("groupJoinFailed: " + id);
    });

    engage.on("groupLeft", function(id) {
        logMsg("groupLeft: " + id);
    });

    engage.on("groupConnected", function(id) {
        logMsg("groupConnected: " + id);
    });

    engage.on("groupConnectFailed", function(id) {
        logMsg("groupConnectFailed: " + id);
    });

    engage.on("groupDisconnected", function(id) {
        logMsg("groupDisconnected: " + id);
    });

    engage.on("groupRxStarted", function(id) {
        logMsg("groupRxStarted: " + id);
    });

    engage.on("groupRxEnded", function(id) {
        logMsg("groupRxEnded: " + id);
    });

    engage.on("groupRxMuted", function(id) {
        logMsg("groupRxMuted: " + id);
    });

    engage.on("groupRxUnmuted", function(id) {
        logMsg("groupRxUnmuted: " + id);
    });

    engage.on("groupRxSpeakersChanged", function(id, speakers) {
        logMsg("groupRxSpeakersChanged: " + id + ", speakers=" + speakers);
    });

    engage.on("groupNodeDiscovered", function(id, nodeJson) {
        logMsg("groupNodeDiscovered: " + id + ", nodeJson=" + nodeJson);

        let node = JSON.parse(nodeJson);
        logMsg("node: nodeId=" + node.identity.nodeId
                    + ", userId=" + node.identity.userId
                    + ", displayName=" + node.identity.displayName);
    });

    engage.on("groupNodeRediscovered", function(id, nodeJson) {
        logMsg("groupNodeRediscovered: " + id + ", nodeJson=" + nodeJson);
    });

    engage.on("groupNodeUndiscovered", function(id, nodeJson) {
        logMsg("groupNodeUndiscovered: " + id + ", nodeJson=" + nodeJson);
    });

    engage.on("groupTxStarted", function(id) {
        logMsg("groupTxStarted: " + id);
    });

    engage.on("groupTxEnded", function(id) {
        logMsg("groupTxEnded: " + id);
    });

    engage.on("groupTxFailed", function(id) {
        console.log("groupTxFailed: " + id);
    });

    engage.on("groupTxUsurpedByPriority", function(id) {
        logMsg("groupTxUsurpedByPriority: " + id);
    });

    engage.on("groupMaxTxTimeExceeded", function(id) {
        logMsg("groupMaxTxTimeExceeded: " + id);
    });

    engage.on("groupAssetDiscovered", function(id, assetJson) {
        logMsg("groupAssetDiscovered: " + id + ", assetJson=" + assetJson);
    });

    engage.on("groupAssetRediscovered", function(id, assetJson) {
        logMsg("groupAssetRediscovered: " + id + ", assetJson=" + assetJson);
    });

    engage.on("groupAssetUndiscovered", function(id, assetJson) {
        logMsg("groupAssetUndiscovered: " + id + ", assetJson=" + assetJson);
    });

    engage.on("groupTimelineEventStarted", function(id, eventJson) {
        logMsg("groupTimelineEventStarted: " + id + ", eventJson=" + eventJson);
    });    

    engage.on("groupTimelineEventUpdated", function(id, eventJson) {
        logMsg("groupTimelineEventUpdated: " + id + ", eventJson=" + eventJson);
    });    

    engage.on("groupTimelineEventEnded", function(id, eventJson) {
        logMsg("groupTimelineEventEnded: " + id + ", eventJson=" + eventJson);
    });    

    engage.on("groupTimelineReport", function(id, reportJson) {
        logMsg("groupTimelineReport: " + id + ", reportJson=" + reportJson);
    });    

    engage.on("groupTimelineReportFailed", function(id) {
        logMsg("groupTimelineRgroupTimelineReportFailedeport: " + id );
    });    

    
    // Licensing events
    engage.on("licenseExpired", function() {
        logMsg("licenseExpired");
    });

    engage.on("licenseChanged", function() {
        logMsg("licenseChanged");
    });

    engage.on("licenseExpiring", function(secondsFromNow) {
        logMsg("licenseExpiring");
    });
}

//--------------------------------------------------------
function logMsg(msg)
{
    var m = new Date();
    var dateString = m.getUTCFullYear() +"/"+ (m.getUTCMonth()+1) +"/"+ m.getUTCDate() + " " + m.getUTCHours() + ":" + m.getUTCMinutes() + ":" + m.getUTCSeconds();

    console.log(chalk.magenta(dateString + "....." + msg));
}


//--------------------------------------------------------
function showdownTheEngine()
{
    console.log(chalk.magenta("showdownTheEngine........... deleteAllGroups"));
    deleteAllGroups();

    console.log(chalk.magenta("showdownTheEngine........... stop"));
    engage.stop();

    console.log(chalk.magenta("showdownTheEngine........... shutdown"));
    engage.shutdown();

    console.log(chalk.magenta("showdownTheEngine........... all done"));
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
