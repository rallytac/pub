# Quick And Easy Testing And Tooling

A quick and easy way to utilize Engage is to use the "*engage-cnd*" command-line tool.  While not pretty, it is a functional application that will allow you to try out a variety of operations using the Engage Engine.  And, if it doesn't do what you need, you can change the source code - take a look in /samples/c/EngageMain.cpp.  That's the source code for the tool.

To use it, you're going to need some configuration files, certificates, and, of course, the binarie itself.  This direcrory contains a short script you can modify to get the files you need and get them setup quickly and easily.

You can download the script or simply copy and paste the following into a script of your own on your favorite Linux platform.

```shell
#!/bin/bash

function show_usage()
{
    echo "usage: setup <platform> <binary_version>"    
}

PLATFORM=${1}
VERSION=${2}
GITHUB_BASE=https://github.com/rallytac/pub/raw/master

if [[ "${PLATFORM}" == "" ]]; then
    show_usage
    exit 1
fi

if [[ "${VERSION}" == "" ]]; then
    show_usage
    exit 1
fi

# Certificates
wget -O rtsCA.pem ${GITHUB_BASE}/certificates/rtsCA.pem
wget -O rtsFactoryDefaultEngage.pem ${GITHUB_BASE}/certificates/rtsFactoryDefaultEngage.pem
wget -O rtsFactoryDefaultEngage.key ${GITHUB_BASE}/certificates/rtsFactoryDefaultEngage.key

# Configurations
wget -O sample_engine_policy.json ${GITHUB_BASE}/configurations/sample_engine_policy.json
sed -i 's/@..\/certificates\//@.\//g' sample_engine_policy.json
wget -O sample_mission_template.json ${GITHUB_BASE}/configurations/sample_mission_template.json
sed -i 's/@..\/certificates\//@.\//g' sample_mission_template.json

# Binaries
wget -O engage-cmd ${GITHUB_BASE}/bin/${VERSION}/${PLATFORM}/engage-cmd
chmod +x engage-cmd
wget -O libengage-shared.so ${GITHUB_BASE}/bin/${VERSION}/${PLATFORM}/libengage-shared.so
chmod +x libengage-shared.so

echo "Dont forget to run 'export LD_LIBRARY_PATH=./' in your terminal!"
```

The script should be pretty self-explanatory but, just to summarize, it does the following:

* Downloads sample certificates files from the *rallytac/pub/certificates* folder.
* Downloads sample configuration files from the *rallytac/pub/configurations* folder.
* Updates the configuration files to point to the certificates in the current directory.
* Downloads binaries from the *rallytac/bin/&lt;version&gt;/&lt;platform&gt;* directory - where &lt;version&gt; is a valid version number (like "1.92.8848") and &lt;platform&gt; is the platform name (like "linux_centos_x64").  These binaries are updated to be executable via *chmod +x*.

Once you have all these goodies in your directory, you're going to need to tell your OS where to find the *libengage-shared* library.  As it's been downloaded to the current directory, simply run:

```shell
export LD_LIBRARY_PATH=./
```

You're now ready to run the tool. It's going to need the name of a mission file and an engine policy file that lays out some parameters for the Engage Engine's overall operation.

```shell
./engage-cmd -mission:sample_mission_template.json -ep:sample_engine_policy.json
```

Once it's up an running, you'll be at a command-line prompt where you can carry out a bunch of operations - enter "?" for help.