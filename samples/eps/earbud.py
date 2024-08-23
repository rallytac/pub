#
# Engage Activity Recorder REST Archiver
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

import os
import sys
import sqlite3
import pycurl
from sqlite3 import Error
import json
import time
import random

ROW_IDX_EVENT_ID = 0
ROW_IDX_GROUP_ID = 1
ROW_IDX_METADATA = 2
ROW_IDX_FILE_URI = 3

# Our global variables
g_verbose = False
g_insecure = False
g_dbFile = ""
g_apikey = ""
g_nodeId = ""
g_cert = ""
g_key = ""
g_ca = ""
g_url = ""
g_intervalSecs = 0
g_maxEvents = -1

# Archive to a URL using cURL to POST a multipart form
def archiveRowToUrl(row):
    rc = 500

    if g_verbose:
        print("archiving", row[ROW_IDX_FILE_URI], "to", g_url)

    path = row[ROW_IDX_FILE_URI][7:]
    fn = os.path.basename(row[ROW_IDX_FILE_URI])

    try:
        c = pycurl.Curl()

        c.setopt(c.USERAGENT, "EarArch/1.0")
        c.setopt(c.SSLCERT, g_cert)
        c.setopt(c.SSLKEY, g_key)

        if len(g_ca) > 0:
            c.setopt(c.CAPATH, g_ca)

        if g_insecure:
            c.setopt(c.SSL_VERIFYPEER, False)
            c.setopt(c.SSL_VERIFYHOST, False)

        c.setopt(c.URL, g_url)
        c.setopt(c.HTTPHEADER, ["apikey: " + g_apikey])

        sendThis = [             
                        ("recordingNodeId", g_nodeId),

                        ("meta", row[ROW_IDX_METADATA]),

                        ("fileupload", (
                            c.FORM_FILE, path,
                            c.FORM_FILENAME, fn,
                            c.FORM_CONTENTTYPE, "application/octet-stream"
                        ))
                ]

        c.setopt(c.HTTPPOST, sendThis)

        c.perform()
        rc = c.getinfo(c.RESPONSE_CODE)
        c.close() 

    except pycurl.error as e:
        rc = 500
        print("curl error:", e)
        
    # We're going to remove the file here to ensure that potential database rebuilds don't
    # accidentally bring it back from archived status if the database needs to be rebuilt.
    if rc >= 200 and rc <= 299:
        os.remove(fn)        

    return rc        



# Process outstanding events
def archiveOutstandingRows():
    rc = True
    processedEventCount = 0
    eventCountToProcess = 32

    conn = None
    try:
        conn = sqlite3.connect(g_dbFile)

        queryCursor = conn.cursor()
        updateCursor = conn.cursor()

        while True:
            # We'll ask for a maximum number of rows per so that we don't have a situation
            # where, if we've been offline for a long time and now have a database with
            # zillions of unarchived rows.  We'd prefer not to have that gigantic result
            # set coming through at once.  So we'll nibble away "LIMIT" number of rows
            # at a time.
            sql = ("SELECT"
                        " event_id, group_id, metadata, file_uri"
                    " FROM"
                        " timeline_events"
                    " WHERE"
                        " in_progress = 0 AND archived = 0"
            )

            # We'll have a hard limit of 32 events at a time but allow g_maxEvents to
            # specify an optional exact amount between 1 and 32 (inclusive) if providing
            if g_maxEvents > 0 and g_maxEvents <= 32:
                eventCountToProcess = g_maxEvents

            sql += " LIMIT " + str(eventCountToProcess)                

            queryCursor.execute(sql)
            rows = queryCursor.fetchall()

            if not rows:
                break

            for row in rows:
                if g_verbose:
                    print("Processing event: " + row[ROW_IDX_EVENT_ID] + ", file=", row[ROW_IDX_FILE_URI])

                archiveResult = archiveRowToUrl(row)

                if archiveResult == 200:
                    sql = "UPDATE timeline_events SET archived = 1 WHERE event_id = '" + row[ROW_IDX_EVENT_ID] + "'"
                    updateCursor.execute(sql)
                    conn.commit()
                    processedEventCount += 1

                else:
                    print("archiveRowToUrl failed for", row[ROW_IDX_EVENT_ID], "- error code", archiveResult)
                    break

            break                

    except Error as e:
        rc = False
        print(e)

    if g_verbose:
        print(processedEventCount, "events archived from " + g_dbFile)

    # Change rc to False if we might have only processed a subset of all rows in the database
    if processedEventCount >= eventCountToProcess:
        rc = False

    return rc



# Show how to use this thing
def showSyntax():
    print("usage: " + sys.argv[0] + " -db:<database_file> -node:<nodeId> -api:<apikey> -cert:<certificate> -key:<private_key> -url:<archive_url> [-ca:<server_ca_certificate>] [-int:<archive_interval_secs>] [-maxe:<max_events_per_interval>] [-verbose] [-insecure]")


# Checks a string argument
def checkStringArg(str, msg):
    if len(str) <= 0:
        print(msg)
        showSyntax()
        exit(1)



# main()
if __name__ == "__main__":
    for x in range(1, len(sys.argv)):
        if sys.argv[x] == "-verbose":
            g_verbose = True
        elif sys.argv[x] == "-insecure":
            g_insecure = True
        elif sys.argv[x].startswith("-db:"):
            g_dbFile = sys.argv[x][4:]
        elif sys.argv[x].startswith("-cert:"):
            g_cert = sys.argv[x][6:]
        elif sys.argv[x].startswith("-key:"):
            g_key = sys.argv[x][5:]
        elif sys.argv[x].startswith("-ca:"):
            g_ca = sys.argv[x][4:]
        elif sys.argv[x].startswith("-url:"):
            g_url = sys.argv[x][5:]
        elif sys.argv[x].startswith("-api:"):
            g_apikey = sys.argv[x][5:]
        elif sys.argv[x].startswith("-node:"):
            g_nodeId = sys.argv[x][6:]
        elif sys.argv[x].startswith("-int:"):
            g_intervalSecs = int(sys.argv[x][5:])
        elif sys.argv[x].startswith("-maxe:"):
            g_maxEvents = int(sys.argv[x][6:])            
        else:
            print("Unknown argument '" + sys.argv[x] + "'")
            showSyntax()
            exit(1)

    checkStringArg(g_dbFile, "No database file provided")
    checkStringArg(g_cert, "No certificate provided")
    checkStringArg(g_key, "No private key provided")
    checkStringArg(g_url, "No URL provided")
    checkStringArg(g_apikey, "No API key provided")
    checkStringArg(g_nodeId, "No node ID provided")

    if g_verbose:
        print("---------------------------------------------------------------------------------------------------")
        print("Engage Activity Recorder REST Archiver 0.1")
        print("Copyright (c) 2020 Rally Tactical Systems, Inc.")
        print("---------------------------------------------------------------------------------------------------")

    while True:
        rc = archiveOutstandingRows()
        if g_intervalSecs <= 0:
            break

        # If all rows were processed, then check again after 1 second + a random amount based on
        # g_intervalSecs.  If an error was encountered, then check again sometime
        # within the next 5 seconds
        if rc:
            sleepSecs = (1 + random.randrange(g_intervalSecs))
        else:
            sleepSecs = (1 + random.randrange(5))

        time.sleep(sleepSecs)

# END
