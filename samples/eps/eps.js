//
// Engage Provisioning Service REST Service (eps)
// Copyright (c) 2023 Rally Tactical Systems, Inc.
//
// Dependencies:
//      The following npm packages are required.  They are listed in package.json and
//      can be manually installed or simply through using "npm install":
//
//              formidable
//              fs-extra
//              sqlite3
//              dateformat
//

const formidable = require('formidable');
const url = require('url');
const querystring = require('querystring');
const https = require("https");
const fileSystem = require("fs-extra");
const sqlite3 = require('sqlite3').verbose();
const crypto = require("crypto");
const dateFormat = require('dateformat');
const { abort } = require('process');

// Our version number
const VERSION = "0.1";

// Out logging levels
const LOG_FATAL = 0;
const LOG_ERROR = 1;
const LOG_WARN = 2;
const LOG_INFO = 3;
const LOG_DEBUG = 4;

// A request path to ignore (usually requested by browsers)
const FAVICON_FILE_NAME_URL = "/favicon.ico";

// Max number of rows that can be returned in responses
const RESPONSE_ROW_LIMIT = 64;

// Number of bytes in keys
const ITEM_KEY_SIZE = 32;

// Operation modes
const OPMODE_STANDARD = "standard";
const OPMODE_STORAGE_RELAY = "noDatabase";

// Load up and validate the configuration
var configuration = fileSystem.readJSONSync("./eps_conf.json");
if(!configuration.logging || !configuration.logging.level)
{
    configuration.logging = {"level": LOG_INFO};
}

// Translate trhe URIs for version
configuration.uris.postSingle = configuration.uris.postSingle.replace(/\${apiVersion}/g, configuration.uris.apiVersion);
configuration.uris.getSingle = configuration.uris.getSingle.replace(/\${apiVersion}/g, configuration.uris.apiVersion);
configuration.uris.getSingleRaw = configuration.uris.getSingleRaw.replace(/\${apiVersion}/g, configuration.uris.apiVersion);
configuration.uris.getMultiple = configuration.uris.getMultiple.replace(/\${apiVersion}/g, configuration.uris.apiVersion);

// Make sure our data directory exists
fileSystem.ensureDirSync(configuration.localStorageRoot);

// Setup the tenants and retain a map indexed by API key
const tenantMap = setupAllTenants();

// Build up the mapping of API keys to their tenants
const apiKeyMap = new Map();
tenantMap.forEach((tenant) =>
{
    tenant.apiKeys.forEach((apiKey) => {
        var element = { "tenant": tenant, "permissions": apiKey.permissions };
        apiKeyMap.set(apiKey.key, element);
    });
});

// Process and route incoming requests
const requestListener = function (request, response) 
{
    // Hand off
    if(equalsIgnoringCase(request.method, "POST"))
    {
        handlePost(request, response);
    }
    else if(equalsIgnoringCase(request.method, "GET"))
    {
        handleGet(request, response);
    }
    else
    {
        logW("unsupported request method '" + request.method + "'");
        response.writeHead(405);
        response.end();
        return;
    }
};

// Setup our server
const httpsServerOpts = {
    cert: fileSystem.readFileSync(configuration.https.certPem),
    key: fileSystem.readFileSync(configuration.https.keyPem),
    requestCert: true,
    rejectUnauthorized: true,
    ca :[]
};

// Add CA certificates
configuration.https.ca.forEach((fn) => {
    httpsServerOpts.ca.push(fileSystem.readFileSync(fn));
});

const server = https.createServer(httpsServerOpts, requestListener);

server.listen(configuration.https.port, configuration.https.host, () => 
    {
        logI("---------------------------------------------------------------------------------------------------", true);
        logI("Engage Provisioning Service REST Service (eps) " + VERSION, true);
        logI("Copyright (c) 2023 Rally Tactical Systems, Inc.", true);
        logI("", true);
        logI(configuration, true);
        logI("---------------------------------------------------------------------------------------------------", true);
    }
);

// Setup the archive cleanup
setInterval(archiveCleanup, configuration.timers.cleanupIntervalSecs * 1000);
function archiveCleanup()
{
    tenantMap.forEach((tenant) => {
        // Do nothing if this guy does not have a database
        if(tenant.db)
        {
            tenant.db.all(`SELECT
                                item_key,
                                content_uri,
                                ((strftime('%s', 'now') * 1000) - ts_ended) AS age_ms
                            FROM 
                                RECORDINGS
                            WHERE
                                age_ms > (${tenant.maxEventAgeHours} * 3600 * 1000);`,
            function(err, rows) {
                if(err)
                {
                    logE(err);
                }
                else
                {
                    rows.forEach((row) => {
                        logD("removing stale item " + row.item_key + ", uri=" + row.content_uri );

                        fileSystem.removeSync(row.content_uri);
                        fileSystem.removeSync(row.content_uri + ".meta");
                        tenant.db.run("DELETE FROM RECORDINGS WHERE item_key = ?", [row.item_key]);
                    });
                }
            }
        );
        }
    });
}

// Logging goodies
function logD(msg, excludeDetail)
{
    logMsg(LOG_DEBUG, msg, excludeDetail);
}

function logI(msg, excludeDetail)
{
    logMsg(LOG_INFO, msg, excludeDetail);
}

function logW(msg, excludeDetail)
{
    logMsg(LOG_WARN, msg, excludeDetail);
}

function logE(msg, excludeDetail)
{
    logMsg(LOG_ERROR, msg, excludeDetail);
}

function logF(msg, excludeDetail)
{
    logMsg(LOG_FATAL, msg, excludeDetail);
}

function logDetail(level)
{
    var ltype;

    switch(level)
    {
        case LOG_DEBUG:
            ltype = "D";
            break;

        case LOG_INFO:
            ltype = "I";
            break;

        case LOG_WARN:
            ltype = "W";
            break;        

        case LOG_ERROR:
            ltype = "E";
            break;        
    
        case LOG_FATAL:
            ltype = "F";
            break;        
    
        default:
            ltype = "?";
            break;
    }

    return dateFormat(Date.now(), "yyyy-mm-dd hh:MM:ss:l") + " " + ltype;
}

function logMsg(level, msg, excludeDetail)
{    
    if(level <= configuration.logging.level)
    {
        if(typeof(msg) == "object")
        {
            if(!excludeDetail)
            {
                console.log(logDetail(level) + ": ");
            }
            
            console.log(msg);
        }
        else
        {
            if(excludeDetail)
            {
                console.log(msg);
            }
            else
            {
                console.log(`${logDetail(level)}: ${msg}`);
            }
        }
    }
}


// Setup all the tenants and cache those instances
function setupAllTenants()
{
    var theMap = new Map();

    configuration.tenants.forEach((tenant) => {
        fileSystem.ensureDirSync(configuration.localStorageRoot + "/" + tenant.id);

        // Make sure we have a valid operation mode
        if(!tenant.opMode || tenant.opMode == "")
        {
            tenant.opMode = OPMODE_STANDARD;
        }

        if(!equalsIgnoringCase(tenant.opMode, OPMODE_STANDARD) &&
           !equalsIgnoringCase(tenant.opMode, OPMODE_STORAGE_RELAY) )
        {
            logF("unsupported opMode '" + tenant.opMode + "'");
            abort();
        }
                
        // Only setup a database if we're operating in standard mode
        if(equalsIgnoringCase(tenant.opMode, OPMODE_STANDARD))
        {
            var dbFileName = configuration.localStorageRoot + "/" + tenant.id + "/database.sqlite";
            tenant.db = setupDatabase(dbFileName);
        }

        theMap.set(tenant.id, tenant);
    });

    return theMap;
}


// Get the tenant for an API key and validate the operation permission
function getTenant(apikey, op)
{
    const element = apiKeyMap.get(apikey);
    if(element == null)
    {
        logW("apikey:" + apikey + " not found");
        return null;
    }

    if(!element.permissions.includes(op))
    {
        logW("apikey:" + apikey + " does not have " + op + " permission");
        return null;
    }

    return element.tenant;
}


// Sets up the database, creating tables if necessary
function setupDatabase(fn)
{
    var db = new sqlite3.Database(fn);

    db.serialize(() => {
        db.run(`CREATE TABLE IF NOT EXISTS 
                kv_pairs(
                    key_name CHAR(64) PRIMARY KEY NOT NULL, 
                    key_value TEXT
                );`)

        db.run(`CREATE TABLE IF NOT EXISTS items(
                    item_key CHAR(32) PRIMARY KEY NOT NULL,
                    type CHAR(32) NOT NULL,
                    ts_created DATETIME NOT NULL,
                    ts_last_accessed DATETIME NULL,
                    content_uri TEXT NULL
                 );`);

        db.run(`CREATE INDEX IF NOT EXISTS idx_items_type
                         ON items(type);`);

        db.run(`CREATE INDEX IF NOT EXISTS idx_items_ts_created
                         ON items(ts_created);`);

        db.run(`CREATE INDEX IF NOT EXISTS idx_items_ts_last_accessed
                         ON items(ts_last_accessed);`);
    });
    
    return db;
}


// Generates an item key
function generateItemKey()
{
    return crypto.randomBytes(ITEM_KEY_SIZE).toString('hex');
}


// Simplify comparing strings while ignoring case in JS
function equalsIgnoringCase(text, other) 
{
    return text.localeCompare(other, undefined, { sensitivity: 'base' }) === 0;
}


// Move files into the archive location.  In this case we're using the local file 
// system for storage but we could just as simply be using a cloud-provided
// storage mechanism.  
function moveFilesToArchive(dstDir, srcFnBin, dstFnBin, srcFnMeta, dstFnMeta)
{
    // Make sure the directory we're using exists
    fileSystem.ensureDirSync(dstDir);

    // Rename/move is the faster option but prone to problems on platforms
    // such as Windows where we may have different drives for the source and
    // destination - a situation which may not support rename/move

    var renameOk = false;

    try
    {
        fileSystem.renameSync(srcFnBin, dstDir + "/" + dstFnBin);
        fileSystem.renameSync(srcFnMeta, dstDir + "/" + dstFnMeta);
        renameOk = true;
    }
    catch(error)
    {
        renameOk = false;
    }

    // If the rename/move didn't work, then resort to copy and delete
    if(!renameOk)
    {
        fileSystem.copyFileSync(srcFnBin, dstDir + "/" + dstFnBin);
        fileSystem.copyFileSync(srcFnMeta, dstDir + "/" + dstFnMeta);

        fileSystem.removeSync(srcFnBin);
        fileSystem.removeSync(srcFnMeta);
    }

    return dstDir;
}


// Handle POSTs
function handlePost(request, response)
{
    // Get the tenant
    var tenant = getTenant(request.headers.apikey, "POST");
    if(tenant == null)
    {
        logW("tenant not found for: " + request.headers.apikey);
        response.writeHead(401);            
        response.end();
        return;
    }

    // The URL must be configuration.uris.postSingle
    if(request.url != configuration.uris.postSingle)
    {
        logW("invalid url: " + request.url);
        response.writeHead(400);            
        response.end();
        return;
    }

    // We expect the upload to be a multipart form
    var form = new formidable.IncomingForm();
    if(form == null)
    {
        logW("no form data");
        response.writeHead(400);            
        response.end();
        return;
    }

    // Process the parsed form
    form.parse(request, function(err, fields, files) 
                    {
                        // Bail if we had an error
                        if(err) 
                        {
                            logW("form parsing error: " + err);
                            response.writeHead(400);
                            response.end();
                            return;
                        }

                        // Make sure we have a recording node
                        recordingNodeId = fields.recordingNodeId;
                        if(recordingNodeId == null)
                        {
                            logW("no recordingNodeId in form");
                            response.writeHead(400);
                            response.end();
                            return;
                        }

                        // Make sure we have metadata accompanying the upload
                        metadataText = fields.meta;
                        if(metadataText == null)
                        {
                            logW("no metatdata in form");
                            response.writeHead(400);
                            response.end();
                            return;
                        }

                        // Assign a unique key for this item
                        itemKey = generateItemKey();

                        // Extract some info from the metadata JSON
                        metadataJson = JSON.parse(metadataText);
                        groupId = metadataJson.engageEvent.groupId;
                        groupName = metadataJson.engageEvent.groupName;

                        // The (ideal) destination directory
                        dstPath = configuration.localStorageRoot + "/" + tenant.id                        

                        // The binary and accompanying metadata source files
                        srcFnBin = files.fileupload.path;
                        srcFnMeta = files.fileupload.path + ".meta";

                        // The binary and accompanying metadata destination files
                        dstFnBin = files.fileupload.name;
                        dstFnMeta = dstFnBin + ".meta";

                        // Save the meta data to the metadata file
                        fileSystem.writeFileSync(srcFnMeta, metadataText);

                        // Move our files into the archive - the return string is the actual storage location of the files
                        archiveDir = moveFilesToArchive(dstPath, srcFnBin, dstFnBin, srcFnMeta, dstFnMeta);

                        // Only do database work if we have a database
                        if(tenant.db)
                        {
                            // Construct the URI for the content
                            contentUri = archiveDir + "/" + dstFnBin;

                            // Plug it into our database
                            tenant.db.serialize(() => {
                                tenant.db.run(`INSERT INTO items VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);`,
                                        [
                                            itemKey,
                                            recordingNodeId, 
                                            metadataJson.engageEvent.id,
                                            metadataJson.engageEvent.groupId,
                                            metadataJson.engageEvent.type,
                                            metadataJson.engageEvent.direction,
                                            metadataJson.engageEvent.thisNodeId,
                                            metadataJson.engageEvent.started,
                                            metadataJson.engageEvent.ended,
                                            metadataJson.engageEvent.nodeId,
                                            metadataJson.engageEvent.alias,
                                            metadataJson.engageEvent.rxTxFlags,
                                            fields.meta,
                                            contentUri,
                                            metadataJson.engageEvent.txId
                                        ], function(err) {
                                                if(err)
                                                {
                                                    // TODO: rollback the moveFilesToArchive() operation from above

                                                    logE("database error (1) :" + err);
                                                    response.writeHead(500);
                                                    response.end();
                                                }
                                                else
                                                {
                                                    tenant.db.run("INSERT OR IGNORE INTO groups VALUES (?,?);",
                                                    [
                                                        metadataJson.engageEvent.groupId,
                                                        metadataJson.engageEvent.groupName
                                                    ], function(err) {
                                                            if(err)
                                                            {
                                                                // TODO: rollback the moveFilesToArchive() operation from above
                    
                                                                logE("database error (2) :" + err);
                                                                response.writeHead(500);
                                                                response.end();
                                                            }
                                                            else
                                                            {
                                                                // Maybe display some info
                                                                if(configuration.logging.level >= LOG_DEBUG)
                                                                {
                                                                    var eventId = metadataJson.engageEvent.id;
                                                                    logD("from: " + request.connection.remoteAddress + ", group:" + groupId + " (" + groupName + "), event:" + eventId + " -> " + dstFnBin);
                                                                }
                    
                                                                response.writeHead(200);
                                                                response.end();
                                                            }
                                                    });                                                            
                                                }
                                            });
                            });
                        }
                        else
                        {
                            // Maybe display some info
                            if(configuration.logging.level >= LOG_DEBUG)
                            {
                                var eventId = metadataJson.engageEvent.id;
                                logD("from: " + request.connection.remoteAddress + ", group:" + groupId + " (" + groupName + "), event:" + eventId + " -> " + dstFnBin);
                            }

                            response.writeHead(200);
                            response.end();
                        }
                    }
            );
}


// This little class helps in building SQL queries
class SqlQuery
{
    constructor(sql)
    {
        this.sql = sql;
        this.paramsForSql = [];
    }

    addCriteria(columnName, jsonRootText)
    {
        if(jsonRootText)
        {
            const jsonRoot = JSON.parse(jsonRootText);

            if(jsonRoot)
            {
                if(this.paramsForSql.length == 0)
                {
                    this.sql += " WHERE ";
                }

                if(this.paramsForSql.length > 0)
                {
                    this.sql += " AND ";
                }

                this.sql += " " + columnName + " ";

                if(jsonRoot.eq)
                {
                    this.sql += " = ?"
                    this.paramsForSql.push(jsonRoot.eq);
                }
                else if(jsonRoot.ne)
                {
                    this.sql += " != ?"
                    this.paramsForSql.push(jsonRoot.ne);
                }
                else if(jsonRoot.gt)
                {
                    this.sql += " > ?"
                    this.paramsForSql.push(jsonRoot.gt);
                }
                else if(jsonRoot.gte)
                {
                    this.sql += " >= ?"
                    this.paramsForSql.push(jsonRoot.gte);
                }
                else if(jsonRoot.lt)
                {
                    this.sql += " < ?"
                    this.paramsForSql.push(jsonRoot.lt);
                }
                else if(jsonRoot.lte)
                {
                    this.sql += " <= ?"
                    this.paramsForSql.push(jsonRoot.lte);
                }
                else
                {
                    this.sql += " = ?"
                    this.paramsForSql.push(jsonRoot);
                }

                this.sql += " ";
            }
        }
    }

    finalize(sql)
    {
        this.sql += " " + sql;
    }

    getSql()
    {
        return this.sql;
    }

    getParams()
    {
        return this.paramsForSql;
    }
}


// Handle GETs for raw data
function handleRawGet(request, response, tenant, itemKey)
{
    var q = new SqlQuery(`SELECT 
                                recordings.content_uri
                            FROM 
                                recordings`);

    q.addCriteria("recordings.item_key", `{"eq":"${itemKey}"}`);
    q.finalize("LIMIT 1");

    logD(q.getSql());
    logD(q.getParams());

    tenant.db.all(q.getSql(), q.getParams(),
        function(err, rows) {
            if(err)
            {
                logE(err);
                response.writeHead(500);
                response.end();
            }
            else
            {
                var content_uri = null;

                rows.forEach((row) => {
                    content_uri = row.content_uri;
                });

                if(content_uri)
                {
                    logD("returning:" + content_uri);

                    var stat = fileSystem.statSync(content_uri);
    
                    response.writeHead(200, {
                        "Content-Type": "application/octet-stream",
                        "Content-Length": stat.size
                    });
    
                    var data = fileSystem.readFileSync(content_uri);
    
                    response.write(data);
                    response.end();
                }
                else
                {
                    response.writeHead(404);
                    response.end();
                }
            }
        }
    );
}


// Handle GETs
function handleGet(request, response)
{   
    // Ignore requests for favicon.ico
    if(request.url == FAVICON_FILE_NAME_URL)
    {
        response.writeHead(204);
        response.end();
        return;
    }

    // Get the tenant
    var tenant = getTenant(request.headers.apikey, "GET");
    if(tenant == null)
    {
        logW("tenant not found or operation not allowed for: " + request.headers.apikey);
        response.writeHead(401);            
        response.end();
        return;
    }

    // We can't continue if we don't have a database for the tenant
    if(!tenant.db)
    {
        logW("GET no supported as tenant has no database");
        response.writeHead(405);
        response.end();
        return;
    }

    const parsedUrl = url.parse(request.url, true);

    // Special handling for a single recording where the caller wants the raw content
    if(parsedUrl.pathname.startsWith(configuration.uris.getSingleRaw))
    {
        handleRawGet(request, 
            response, 
            tenant,
            parsedUrl.pathname.substr(configuration.uris.getSingleRaw.length));

        return;
    }

    var q = new SqlQuery(`SELECT 
                                recordings.item_key,
                                recordings.recording_node_id,
                                recordings.event_id,
                                recordings.group_id,
                                recordings.type,
                                recordings.direction,
                                recordings.ts_started,
                                recordings.ts_ended,
                                recordings.node_id,
                                recordings.alias,
                                recordings.rxtx_flags,
                                recordings.transmission_id,
                                (recordings.ts_ended - recordings.ts_started) AS duration_ms,
                                (groups.group_name)
                            FROM 
                                recordings
                            LEFT JOIN groups ON recordings.group_id = groups.group_id`);

    var theLimit = RESPONSE_ROW_LIMIT;
    var isSingleElementGet = false;

    // Single recording
    if(parsedUrl.pathname.startsWith(configuration.uris.getSingle))
    {
        // Only 1 record please
        theLimit = 1;
        isSingleElementGet = true;

        // Use our query constructor
        q.addCriteria("recordings.item_key", `{"eq":"${parsedUrl.pathname.substr(configuration.uris.getSingle.length)}"}`);
    }
    // Multiple recordings
    else if(parsedUrl.pathname === configuration.uris.getMultiple)
    {
        if(parsedUrl.search)
        {
            var params = parsedUrl.search;
            if(params && params.startsWith("?"))
            {
                params = params.substr(1);
            }
        
            const paramsJson = querystring.parse(params);
                
            q.addCriteria("recordings.item_key", paramsJson.itemKey);
            q.addCriteria("recordings.recording_node_id", paramsJson.recordingNodeId);
            q.addCriteria("recordings.event_id", paramsJson.eventId);
            q.addCriteria("recordings.group_id", paramsJson.groupId);
            q.addCriteria("recordings.type", paramsJson.type);
            q.addCriteria("recordings.direction", paramsJson.direction);
            q.addCriteria("recordings.ts_started", paramsJson.started);
            q.addCriteria("recordings.ts_ended", paramsJson.ended);
            q.addCriteria("recordings.node_id", paramsJson.nodeId);
            q.addCriteria("recordings.alias", paramsJson.alias);
            q.addCriteria("recordings.rxtx_flags", paramsJson.flags);
            q.addCriteria("recordings.transmission_id", paramsJson.transmission_id);
            
            q.addCriteria("duration_ms", paramsJson.durationMs);
    
            // "limit" with a max of RESPONSE_ROW_LIMIT
            if(paramsJson.limit)
            {
                theLimit = parseInt(paramsJson.limit);
                if(theLimit > RESPONSE_ROW_LIMIT)
                {
                    theLimit = RESPONSE_ROW_LIMIT;
                }
            }
        }
    }
    // Bad request
    else
    {
        response.writeHead(400);
        response.end();
        return;
    }

    // Order by the item_key so that if we have more than what the limit allows, we'll be able to respond
    // with what the next key in the sequence would be
    q.finalize("ORDER BY recordings.item_key LIMIT " + (theLimit + 1));

    logD(q.getSql());
    logD(q.getParams());

    const started = Date.now();

    tenant.db.all(q.getSql(), q.getParams(),
        function(err, rows) {
            if(err)
            {
                logE(err);
                response.writeHead(500);
                response.end();
            }
            else
            {
                if(isSingleElementGet && rows.length == 0)
                {
                    response.writeHead(404);
                    response.end();
                }
                else
                {
                    const execTime = Date.now() - started;
                    response.writeHead(200, {'Content-Type': 'application/json'});
                    
                    var resultJson = {
                        "ts":Date.now(), 
                        "execMs":execTime,
                        "recordings":[]
                    };

                    var ctr = 0;
                
                    rows.forEach((row) => {
                        // Our counter keeps track of how many rows we've returned.  While
                        // we're within the row limit, just add the row.  Otherwise, set
                        // the value of nextItemKey which, for sure, is going to be the last 
                        // row we process.
                        ctr++;
                        if(ctr <= theLimit)
                        {
                            var rowJson = { 
                                "itemKey": row.item_key,
                                "recordingNodeId": row.recording_node_id,
                                "groupId": row.group_id,
                                "groupName": row.group_name,
                                "type": row.type,
                                "direction": row.direction,
                                "started": row.ts_started,
                                "ended": row.ts_ended,
                                "nodeId": row.node_id,
                                "alias": row.alias,
                                "flags": row.rxtx_flags,
                                "durationMs": row.duration_ms,
                                "transmission_id": row.transmission_id,
                            };
                    
                            resultJson.recordings.push(rowJson);
                        }
                        else
                        {
                            resultJson.nextItemKey = row.item_key;
                        }
                    });
                
                    response.end(JSON.stringify(resultJson));
                }
            }
        }
    );    
}
