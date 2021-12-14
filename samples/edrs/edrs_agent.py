#
# Engage Document Repository Server Agent (edrs_agent)
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

import os
import sys
import pycurl
import time
import random
import json

UPLOAD_OK_RESULT = 200
UPLOAD_GENERAL_ERROR_RESULT = -1
UPLOAD_SERVER_ERROR_RESULT = 500
UPLOAD_DUPLICATE_RESULT = 513

# Our global variables
g_verbose = False
g_insecure = False
g_file = ""
g_apikey = ""
g_nodeId = ""
g_cert = ""
g_key = ""
g_ca = ""
g_url = ""
g_mimeType = ""
g_tag = ""
g_instance = ""
g_intervalSecs = 0  # NOTE: A value of 0 for the interval will cause this script to run once and exit
g_useFileTs = False

# Upload to a URL using cURL to POST a multipart form
def uploadFileToUrl(path, fileTs):
    rc = UPLOAD_GENERAL_ERROR_RESULT

    if g_verbose:
        print("uploading", path, "to", g_url)

    fn = os.path.basename(path)

    try:
        if g_useFileTs:
            ts = str(fileTs)
        else:
            jsonRoot = loadJson(path)
            ts = str(jsonRoot["ts"])            

        c = pycurl.Curl()

        c.setopt(c.USERAGENT, "EdrsAgent/1.0")
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
                        ("nodeId", g_nodeId),
                        ("mimeType", g_mimeType),
                        ("tag", g_tag),
                        ("instance", g_instance),
                        ("ts", ts),
                        
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

        if rc == UPLOAD_DUPLICATE_RESULT:
            print(path, "already present on remote")
            rc = UPLOAD_OK_RESULT

    except pycurl.error as e:
        rc = UPLOAD_GENERAL_ERROR_RESULT
        print("curl error:", e)

    return rc        


# Show how to use this thing
def showSyntax():
    print("usage: " + sys.argv[0] + " -f:<file> -mt:<mime_type> -tag:<tag> -instance:<instance> -api:<apikey> -cert:<certificate> -key:<private_key> -url:<destination_url> -node:<node_id> [-ca:<server_ca_certificate>] [-int:<polling_interval_secs>] [-verbose] [-insecure] [-filets]")

# Checks a string argument
def checkStringArg(str, msg):
    if len(str) <= 0:
        print(msg)
        showSyntax()
        exit(1)

# Loads JSON from a file
def loadJson(path):
    with open(path) as f:
        return json.load(f)

# main()
if __name__ == "__main__":
    for x in range(1, len(sys.argv)):
        if sys.argv[x] == "-verbose":
            g_verbose = True
        elif sys.argv[x] == "-insecure":
            g_insecure = True
        elif sys.argv[x].startswith("-f:"):
            g_file = sys.argv[x][3:]
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
        elif sys.argv[x].startswith("-tag:"):
            g_tag = sys.argv[x][5:]
        elif sys.argv[x].startswith("-mt:"):
            g_mimeType = sys.argv[x][4:]
        elif sys.argv[x].startswith("-instance:"):
            g_instance = sys.argv[x][10:]
        elif sys.argv[x] == "-filets":
            g_useFileTs = True            
        else:
            print("Unknown argument '" + sys.argv[x] + "'")
            showSyntax()
            exit(1)

    checkStringArg(g_file, "No file provided")
    checkStringArg(g_cert, "No certificate provided")
    checkStringArg(g_key, "No private key provided")
    checkStringArg(g_url, "No URL provided")
    checkStringArg(g_apikey, "No API key provided")
    checkStringArg(g_nodeId, "No node ID provided")
    checkStringArg(g_tag, "No tag provided")
    checkStringArg(g_instance, "No instance provided")
    checkStringArg(g_mimeType, "No mime type provided")

    if g_verbose:
        print("---------------------------------------------------------------------------------------------------")
        print("Engage Document Archive Agent 0.1")
        print("Copyright (c) 2021 Rally Tactical Systems, Inc.")
        print("---------------------------------------------------------------------------------------------------")
    
    lastFileMTime = 0
    while True:
        rc = False

        try:
            # Get the file's modification time
            currentFileMTime = os.stat(g_file).st_mtime

            # If it's different from the last time we polled we will try to upload
            if lastFileMTime != currentFileMTime:                
                uploadResult = uploadFileToUrl(g_file, str(int(currentFileMTime) * 1000))

                # If the upload went well, then save the mtime for our next polling check
                if uploadResult == UPLOAD_OK_RESULT:
                    lastFileMTime = currentFileMTime
                    rc = True

            else:
                rc = True
                
        except os.error as e:
            rc = False
            print(e)

        if g_intervalSecs <= 0:
            break

        # If the upload operation worked processed, then check again after 1 
        # second + a random amount based on g_intervalSecs.  If an error was 
        # encountered, then check again sometime within the next 5 seconds
        if rc:
            sleepSecs = (1 + random.randrange(g_intervalSecs))
        else:
            sleepSecs = (1 + random.randrange(5))
            if g_verbose:
                print("retry in", sleepSecs, "seconds")

        time.sleep(sleepSecs)

# END
