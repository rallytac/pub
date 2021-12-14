//
// Engage JSON Archive REST Service (ejars)
// Copyright (c) 2021 Rally Tactical Systems, Inc.
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
const { createHash } = require('crypto');

// Our version number
const VERSION = "0.1";

// Our logging levels
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

// Methods
const METHOD_GET = "GET";
const METHOD_POST = "POST";

// Our results
const BAD_REQUEST_RESULT = 400;
const UNAUTHORIZED_RESULT = 401;
const NOT_FOUND_RESULT = 404;
const METHOD_NOT_ALLOWED_RESULT = 405;

const OK_RESULT = 200;
const GENERAL_ERROR_RESULT = -1;
const SERVER_ERROR_RESULT = 500;
const DUPLICATE_RESULT = 513;


// Operation modes
const OPMODE_STANDARD = "standard";
const OPMODE_STORAGE_RELAY = "storageRelay";

// Install a SIGINT handler for Ctrl-C
process.on('SIGINT', function() 
{
    console.log("Stop requested!");
    process.exit();
});

// Make sure we're in the directory where this script is located so that relative paths work
process.chdir(__dirname)

// Load up and validate the configuration
var configuration = fileSystem.readJSONSync("./ejars_conf.json");
if(!configuration.logging || !configuration.logging.level)
{
    configuration.logging = {"level": LOG_INFO};
}

// Translate the URIs for version
configuration.api.uris.postSingle = configuration.api.uris.postSingle.replace(/\${version}/g, configuration.api.version).toLowerCase();
configuration.api.uris.getSingle = configuration.api.uris.getSingle.replace(/\${version}/g, configuration.api.version).toLowerCase();
configuration.api.uris.getSingleRaw = configuration.api.uris.getSingleRaw.replace(/\${version}/g, configuration.api.version).toLowerCase();
configuration.api.uris.getMultiple = configuration.api.uris.getMultiple.replace(/\${version}/g, configuration.api.version).toLowerCase();
configuration.api.uris.getMostRecentRaw = configuration.api.uris.getMostRecentRaw.replace(/\${version}/g, configuration.api.version).toLowerCase();

// Make sure our data directory exists
fileSystem.ensureDirSync(configuration.localStorageRoot);

// Setup the tenants and retain a map indexed by API key
const tenantMap = setupAllTenants();

// Build up the mapping of API keys to their tenants
const apiKeyMap = new Map();
tenantMap.forEach((tenant) =>
{
    tenant.apiKeys.forEach((apiKey) => 
    {
        var element = { "tenant": tenant, "permissions": apiKey.permissions };
        apiKeyMap.set(apiKey.key, element);
    });
});

// Process and route incoming requests
const requestListener = function (request, response) 
{
    // Hand off
    if(equalsIgnoringCase(request.method, METHOD_POST))
    {
        handlePost(request, response);
    }
    else if(equalsIgnoringCase(request.method, METHOD_GET))
    {
        handleGet(request, response);
    }
    else
    {
        logW("unsupported request method '" + request.method + "'");
        response.writeHead(METHOD_NOT_ALLOWED_RESULT);
        response.end();
        return;
    }
};

// Setup our server
const httpsServerOpts = 
{
    cert: fileSystem.readFileSync(configuration.https.certPem),
    key: fileSystem.readFileSync(configuration.https.keyPem),
    requestCert: configuration.https.requestCert,
    rejectUnauthorized: configuration.https.rejectUnauthorized,
    ca :[]
};

// Add CA certificates
configuration.https.ca.forEach((fn) => 
{
    httpsServerOpts.ca.push(fileSystem.readFileSync(fn));
});

const server = https.createServer(httpsServerOpts, requestListener);

server.listen(configuration.https.port, configuration.https.host, () => 
{
    logI("---------------------------------------------------------------------------------------------------", true);
    logI("Engage JSON Archive REST Service (ejars) " + VERSION, true);
    logI("Copyright (c) 2021 Rally Tactical Systems, Inc.", true);
    logI("", true);
    logI(configuration, true);
    logI("---------------------------------------------------------------------------------------------------", true);
});

// Setup the archive cleanup to run periodically
setInterval(archiveCleanup, configuration.timers.cleanupIntervalSecs * 1000);

// And run it right away
archiveCleanup();

function archiveCleanup()
{
    tenantMap.forEach((tenant) => 
    {
        // Do nothing if this guy does not have a database
        if(tenant.db)
        {
            if(tenant.retentions)
            {
                tenant.retentions.forEach((item) => 
                {
                    tenant.db.all(`SELECT
                                        item_key,
                                        type,
                                        content_uri,
                                        ((strftime('%s', 'now') * 1000) - ts) AS age_ms
                                    FROM 
                                        JSONS
                                    WHERE
                                            type LIKE '${item.type}'
                                        AND
                                            age_ms > (${item.maxAgeHours} * 3600 * 1000)
                                    ORDER BY
                                        type, age_ms;`,

                    function(err, rows) 
                    {
                        if(err)
                        {
                            logE(err);
                        }
                        else
                        {
                            var rowsToRetain = item.retained;

                            rows.forEach((row) => 
                            {
                                if(rowsToRetain <= 0)
                                {
                                    logD("removing stale item " + row.item_key + ", uri=" + row.content_uri);

                                    fileSystem.removeSync(row.content_uri);
                                    tenant.db.run("DELETE FROM JSONS WHERE item_key = ?", [row.item_key]);
                                }
                                else
                                {
                                    logD("retaining item " + row.item_key + ", uri=" + row.content_uri );
                                }

                                if(rowsToRetain > 0)
                                {
                                    rowsToRetain--;
                                }
                            });
                        }
                    });
                });
            }
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

    configuration.tenants.forEach((tenant) => 
    {
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

    db.serialize(() => 
    {
        db.run(`CREATE TABLE IF NOT EXISTS 
                KV_PAIRS(
                    key_name CHAR(64) PRIMARY KEY NOT NULL, 
                    key_value TEXT
                );`)

        db.run(`CREATE TABLE IF NOT EXISTS JSONS(
                    item_key CHAR(64) PRIMARY KEY NOT NULL,
                    instance CHAR(64) NOT NULL,
                    node_id CHAR(38) NOT NULL,
                    type INT NOT NULL,
                    ts DATETIME NOT NULL,
                    content_uri TEXT NULL
                 );`);

        db.run(`CREATE INDEX IF NOT EXISTS JSONS_IDX_INSTANCE
                         ON JSONS(instance);`);

        db.run(`CREATE INDEX IF NOT EXISTS JSONS_IDX_NODE_ID
                        ON JSONS(node_id);`);

        db.run(`CREATE INDEX IF NOT EXISTS JSONS_IDX_TYPE
                         ON JSONS(type);`);

        db.run(`CREATE INDEX IF NOT EXISTS JSONS_IDX_TS
                         ON JSONS(ts);`);
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

// Computes a SHA256 hash for content for a specific tenant
function computeSHA256(tenantId, dataToHash) 
{
    const hash = createHash('sha256');
    hash.write(tenantId);
    hash.write(dataToHash);
    return hash.digest("hex");
  }

// Move the file into the archive location.  In this case we're using the local file 
// system for storage but we could just as simply be using a cloud-provided
// storage mechanism.  
function moveFileToArchive(dstDir, srcFnBin, dstFnBin)
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

        fileSystem.removeSync(srcFnBin);
    }

    return dstDir;
}


// Handle POSTs
function handlePost(request, response)
{
    // Get the API key
    var apiKey = request.headers.apikey;
    if(apiKey == null)
    {
        logW("no apiKey");
        response.writeHead(UNAUTHORIZED_RESULT);            
        response.end();
        return;
    }

    // Get the tenant
    var tenant = getTenant(apiKey, METHOD_POST);
    if(tenant == null)
    {
        logW("tenant not found for: " + apiKey);
        response.writeHead(UNAUTHORIZED_RESULT);            
        response.end();
        return;
    }

    // The URL must be configuration.api.uris.postSingle
    if(request.url != configuration.api.uris.postSingle)
    {
        logW("invalid url: " + request.url);
        response.writeHead(BAD_REQUEST_RESULT);            
        response.end();
        return;
    }

    // We expect the upload to be a multipart form
    var form = new formidable.IncomingForm();
    if(form == null)
    {
        logW("no form data");
        response.writeHead(BAD_REQUEST_RESULT);            
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
            response.writeHead(BAD_REQUEST_RESULT);
            response.end();
            return;
        }

        // Make sure we have a node ID
        nodeId = fields.nodeId;
        if(nodeId == null)
        {
            logW("no nodeId in form");
            response.writeHead(BAD_REQUEST_RESULT);
            response.end();
            return;
        }

        // Make sure we have a type
        type = fields.type;
        if(type == null)
        {
            logW("no type in form");
            response.writeHead(BAD_REQUEST_RESULT);
            response.end();
            return;
        }

        // Make sure we have an instance
        instance = fields.instance;
        if(instance == null)
        {
            logW("no instance in form");
            response.writeHead(BAD_REQUEST_RESULT);
            response.end();
            return;
        }

        // Make sure we have a timestamp
        ts = fields.ts;
        if(ts == null)
        {
            logW("no ts in form");
            response.writeHead(BAD_REQUEST_RESULT);
            response.end();
            return;
        }

        // The hash of the tenant ID + content is the unique item key in the database (if we have it)
        itemKey = computeSHA256(tenant.id, fileSystem.readFileSync(files.fileupload.path));

        // The (ideal) destination directory
        dstPath = configuration.localStorageRoot + "/" + tenant.id                        

        // The source and destination files
        srcFn = files.fileupload.path;
        dstFn = itemKey;

        // Move our file into the archive - the return string is the actual storage location of the file
        archiveDir = moveFileToArchive(dstPath, srcFn, dstFn);

        // Only do database work if we have a database
        if(tenant.db)
        {
            // Construct the URI for the content
            contentUri = archiveDir + "/" + dstFn;

            // Plug it into our database
            tenant.db.serialize(() => 
            {
                tenant.db.run(`INSERT INTO JSONS VALUES (?,?,?,?,?,?);`,
                        [
                            itemKey,
                            instance,
                            nodeId,
                            type,
                            ts,
                            contentUri
                        ], 
                        
                        function(err) 
                        {
                                if(err)
                                {
                                    // TODO: rollback the moveFileToArchive() operation from above

                                    if(err.code == "SQLITE_CONSTRAINT")
                                    {
                                        logW(itemKey + " already exists - from " + request.connection.remoteAddress);
                                        response.writeHead(DUPLICATE_RESULT);
                                    }
                                    else
                                    {
                                        logE("database error (1) :" + err + " - from " + request.connection.remoteAddress);
                                        response.writeHead(SERVER_ERROR_RESULT);
                                    }
                                    
                                    response.end();
                                }
                                else
                                {
                                    logD("received " + itemKey + " - from " + request.connection.remoteAddress);
                                    response.writeHead(OK_RESULT);
                                    response.end();                                                    
                                }
                        });
            });
        }
        else
        {
            // Maybe display some info
            logD("received " + itemKey + " - from " + request.connection.remoteAddress);
            response.writeHead(OK_RESULT);
            response.end();
        }
    });
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

// Return a JSON file in the response and end it
function returnJsonFile(response, contentUri)
{
    logD("returning:" + contentUri);

    var stat = fileSystem.statSync(contentUri);
    
    response.writeHead(OK_RESULT, 
    {
        "Content-Type": "application/json",
        "Content-Length": stat.size
    });

    var data = fileSystem.readFileSync(contentUri);

    response.write(data);
    response.end();    
}

// Handle GETs for raw data
function handleRawGet(request, response, tenant, itemKey)
{
    var q = new SqlQuery(`SELECT 
                                JSONS.content_uri
                            FROM 
                                JSONS`);

    q.addCriteria("JSONS.item_key", `{"eq":"${itemKey}"}`);
    q.finalize("LIMIT 1");

    if(configuration.logging.logSql)
    {
        logD(q.getSql());
        logD(q.getParams());
    }

    tenant.db.all(q.getSql(), q.getParams(),
        function(err, rows) {
            if(err)
            {
                logE(err);
                response.writeHead(SERVER_ERROR_RESULT);
                response.end();
            }
            else
            {
                var content_uri = null;

                rows.forEach((row) => 
                {
                    content_uri = row.content_uri;
                });

                if(content_uri)
                {
                    returnJsonFile(response, content_uri);
                }
                else
                {
                    response.writeHead(NOT_FOUND_RESULT);
                    response.end();
                }
            }
        });
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

    const parsedUrl = url.parse(request.url, true);
    const thePath = parsedUrl.pathname.toLowerCase();

    // Make sure we have a valid path
    /*
    if(thePath != configuration.api.uris.getSingle &&
        thePath != configuration.api.uris.getSingleRaw &&
        thePath != configuration.api.uris.getMultiple)
    {
        response.writeHead(BAD_REQUEST_RESULT);
        response.end();
        return;
    }
    */

    // Get the tenant
    var tenant = getTenant(request.headers.apikey, "GET");
    if(tenant == null)
    {
        logW("tenant not found or operation not allowed for: " + request.headers.apikey);
        response.writeHead(UNAUTHORIZED_RESULT);            
        response.end();
        return;
    }

    // Special handling for a single recording where the caller wants the raw content
    /*
    if(thePath.startsWith(configuration.api.uris.getSingleRaw))
    {
        handleRawGet(request, 
            response, 
            tenant,
            thePath.substring(configuration.api.uris.getSingleRaw.length + 1));

        return;
    }
    */
   
    // We can't continue if we don't have a database for the tenant
    if(!tenant.db)
    {
        logW("GET not supported as tenant has no database");
        response.writeHead(METHOD_NOT_ALLOWED_RESULT);
        response.end();
        return;
    }

    if(thePath == configuration.api.uris.getMostRecentRaw)
    {
        var q = new SqlQuery(`SELECT 
                                JSONS.content_uri
                            FROM 
                                JSONS`);

        q.addCriteria("JSONS.type", parsedUrl.query.type);
        q.addCriteria("JSONS.node_id", parsedUrl.query.nodeId);
        q.addCriteria("JSONS.instance", parsedUrl.query.instance);
        
        q.finalize("ORDER BY ts DESC LIMIT 1");

        if(configuration.logging.logSql)
        {
            logD(q.getSql());
            logD(q.getParams());
        }    

        tenant.db.all(q.getSql(), q.getParams(),
        function(err, rows) {
            if(err)
            {
                logE(err);
                response.writeHead(SERVER_ERROR_RESULT);
                response.end();
            }
            else
            {
                var contentUri = null;

                rows.forEach((row) => 
                {
                    contentUri = row.content_uri;
                });

                if(contentUri)
                {
                    returnJsonFile(response, contentUri);
                }
                else
                {
                    response.writeHead(NOT_FOUND_RESULT);
                    response.end();
                }
            }
        });

        return;                            
    }

    var q = new SqlQuery(`SELECT 
                                JSONS.item_key,
                                JSONS.node_id,
                                JSONS.instance,
                                JSONS.type,
                                JSONS.ts
                            FROM 
                                JSONS`);

    var theLimit = RESPONSE_ROW_LIMIT;
    var isSingleElementGet = false;
    var fixedLimit = false;

    // getSingle retricts us to just one
    if(thePath == configuration.api.uris.getSingle)
    {
        theLimit = 1;
        fixedLimit = true;
        isSingleElementGet = true;
    }
    // getMostRecentRaw
    else if(thePath == configuration.api.uris.getMostRecentRaw)
    {
        theLimit = 1;
        fixedLimit = true;
        isSingleElementGet = true;
    }
    
    if(parsedUrl.query)
    {
        q.addCriteria("JSONS.item_key", parsedUrl.query.itemKey);
        q.addCriteria("JSONS.node_id", parsedUrl.query.nodeId);
        q.addCriteria("JSONS.instance", parsedUrl.query.instance);
        q.addCriteria("JSONS.type", parsedUrl.query.type);
        q.addCriteria("JSONS.ts", parsedUrl.query.ts);
    }

    // "limit" with a max of RESPONSE_ROW_LIMIT
    if(!fixedLimit && parsedUrl.query && parsedUrl.query.limit)
    {
        theLimit = parseInt(parsedUrl.query.limit);
        if(theLimit > RESPONSE_ROW_LIMIT)
        {
            theLimit = RESPONSE_ROW_LIMIT;
        }
    }

    q.finalize("ORDER BY JSONS.ts DESC LIMIT " + theLimit);

    if(configuration.logging.logSql)
    {
        logD(q.getSql());
        logD(q.getParams());
    }

    const started = Date.now();

    tenant.db.all(q.getSql(), q.getParams(),
        function(err, rows) 
        {
            if(err)
            {
                logE(err);
                response.writeHead(SERVER_ERROR_RESULT);
                response.end();
            }
            else
            {
                if(isSingleElementGet && rows.length == 0)
                {
                    response.writeHead(NOT_FOUND_RESULT);
                    response.end();
                }
                else
                {
                    const execTime = Date.now() - started;
                    response.writeHead(OK_RESULT, {'Content-Type': 'application/json'});
                    
                    var resultJson = 
                    {
                        "ts":Date.now(), 
                        "execMs":execTime,
                        "jsons":[]
                    };

                    var ctr = 0;
                
                    rows.forEach((row) => 
                    {
                        // Our counter keeps track of how many rows we've returned.  While
                        // we're within the row limit, just add the row.  Otherwise, set
                        // the value of nextItemKey which, for sure, is going to be the last 
                        // row we process.
                        ctr++;
                        if(ctr <= theLimit)
                        {
                            var rowJson = 
                            { 
                                "itemKey": row.item_key,
                                "nodeId": row.node_id,
                                "instance": row.instance,
                                "type": row.type,
                                "ts": row.ts
                            };
                    
                            resultJson.jsons.push(rowJson);
                        }
                        else
                        {
                            resultJson.nextItemKey = row.item_key;
                        }
                    });
                
                    response.end(JSON.stringify(resultJson));
                }
            }
        });    
}
