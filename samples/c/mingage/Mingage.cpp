//
//  Copyright (c) 2020 Rally Tactical Systems, Inc.
//  All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <nlohmann/json.hpp>

#include "EngageInterface.h"
#include "WorkQueue.hpp"

static const char *TAG = "MingageMain";

static const char* MISSION_FILE_NAME = "cfg/active_mission.json";
static const char* POLICY_FILE_NAME = "cfg/active_engine_policy.json";
static const char* IDENTITY_FILE_NAME = "cfg/active_identity.json";
static const char* RP_FILE_NAME = "cfg/active_rallypoint.json";

static const size_t MAX_CMD_BUFF_SIZE = 4096;

typedef struct _tagStateTracker_t
{
    int currentChannel = 1;
    int g_groupsCreated = 0;

    bool connected = false;
    bool isTransmitting = false;
} StateTracker_t;

static nlohmann::json               g_mission;
static nlohmann::json               g_groups;
static WorkQueue                    g_wq;
static StateTracker_t               g_currentState;
static bool                         g_useRp = false;
static bool                         g_reload = false;
static bool                         g_verboseLogging = false;

void createAllGroups();
void joinGroup(const char *pId);
void loadMission();

nlohmann::json* getGroup(const char *pId)
{
    for(size_t i = 0; i < g_groups.size(); i++)
    {
        std::string id = g_groups[i].at("id");

        if(id.compare(pId) == 0)
        {
            return &g_groups[i];
        }
    }
    
    return nullptr;
}


void onGroupCreated(const char *pId, const char *pEventExtraJson)
{
    std::string id = pId;

    g_wq.submit(([id]()
 	{
        nlohmann::json *g = getGroup(id.c_str());
        if(g != nullptr)
        {
            char buff[256];
            std::string name = (*g)["name"];
            sprintf(buff, "onGroupCreated - %s (%s)", name.c_str(), id.c_str());

            (*g)["created"] = true;
            joinGroup(id.c_str());
        }
    }));
}


void onGroupDeleted(const char *pId, const char *pEventExtraJson)
{
    std::string id = pId;

    g_wq.submit(([id]()
 	{
        nlohmann::json *g = getGroup(id.c_str());

        if(g != nullptr)
        {
            char buff[256];
            std::string name = (*g)["name"];
            sprintf(buff, "onGroupDeleted - %s (%s)", name.c_str(), id.c_str());

            (*g)["created"] = false;
            
            if(g_reload)
            {
                bool canProceed = true;

                for(size_t i = 0; i < g_groups.size(); i++)
                {
                    try
                    {
                        if(g_groups[i].at("created") == true)
                        {
                            canProceed = false;
                            break;
                        }
                    }
                    catch(...)
                    {
                        // Nothing to do
                    }
                }

                if(canProceed)
                {
                    g_reload = false;

                    loadMission();
                    createAllGroups();
                }
            }
        }
    }));
}


void onGroupConnected(const char *pId, const char *pEventExtraJson)
{
    std::string id = pId;

    g_wq.submit(([id]()
 	{
        g_currentState.connected = true;
        nlohmann::json *g = getGroup(id.c_str());
        if(g != nullptr)
        {
            char buff[256];
            std::string name = (*g)["name"];
            sprintf(buff, "onGroupConnected - %s (%s)", name.c_str(), id.c_str());

            engageLogMsg(ENGAGE_LOG_LEVEL_INFORMATIONAL, TAG, buff);
        }
    }));
}


void onGroupDisconnected(const char *pId, const char *pEventExtraJson)
{
    std::string id = pId;

    g_wq.submit(([id]()
 	{
        nlohmann::json *g = getGroup(id.c_str());
        if(g != nullptr)
        {
            char buff[256];
            std::string name = (*g)["name"];
            sprintf(buff, "onGroupDisconnected - %s (%s)", name.c_str(), id.c_str());

            engageLogMsg(ENGAGE_LOG_LEVEL_INFORMATIONAL, TAG, buff);
            g_currentState.connected = false;
        }
    }));
}


void onGroupTxStarted(const char *pId, const char *pEventExtraJson)
{
    std::string id = pId;

    g_wq.submit(([id]()
 	{         
        nlohmann::json *g = getGroup(id.c_str());
        if(g != nullptr)
        {
            char buff[256];
            std::string name = (*g)["name"];
            sprintf(buff, "onGroupTxStarted - %s (%s)", name.c_str(), id.c_str());

            engageLogMsg(ENGAGE_LOG_LEVEL_INFORMATIONAL, TAG, buff);

            g_currentState.isTransmitting = true;
        }        
    }));
}


void onGroupTxEnded(const char *pId, const char *pEventExtraJson)
{
    std::string id = pId;

    g_wq.submit(([id]()
 	{
        nlohmann::json *g = getGroup(id.c_str());
        if(g != nullptr)
        {
            char buff[256];
            std::string name = (*g)["name"];
            sprintf(buff, "onGroupTxEnded - %s (%s)", name.c_str(), id.c_str());

            engageLogMsg(ENGAGE_LOG_LEVEL_INFORMATIONAL, TAG, buff);

            g_currentState.isTransmitting = false;
        }        
    }));
}

void onGroupTxFailed(const char *pId, const char *pEventExtraJson)
{
    std::string id = pId;

    g_wq.submit(([id]()
 	{
        nlohmann::json *g = getGroup(id.c_str());
        if(g != nullptr)
        {
            char buff[256];
            std::string name = (*g)["name"];
            sprintf(buff, "onGroupTxFailed - %s (%s)", name.c_str(), id.c_str());

            engageLogMsg(ENGAGE_LOG_LEVEL_ERROR, TAG, buff);

            g_currentState.isTransmitting = false;
        }        
    }));
}

void onGroupTxUsurpedByPriority(const char *pId, const char *pEventExtraJson)
{
    std::string id = pId;

    g_wq.submit(([id]()
 	{
        nlohmann::json *g = getGroup(id.c_str());
        if(g != nullptr)
        {
            char buff[256];
            std::string name = (*g)["name"];
            sprintf(buff, "onGroupTxUsurpedByPriority - %s (%s)", name.c_str(), id.c_str());

            engageLogMsg(ENGAGE_LOG_LEVEL_INFORMATIONAL, TAG, buff);

            g_currentState.isTransmitting = false;
        }        
    }));
}

void onGroupMaxTxTimeExceeded(const char *pId, const char *pEventExtraJson)
{
    std::string id = pId;

    g_wq.submit(([id]()
 	{
        nlohmann::json *g = getGroup(id.c_str());
        if(g != nullptr)
        {
            char buff[256];
            std::string name = (*g)["name"];
            sprintf(buff, "onGroupMaxTxTimeExceeded - %s (%s)", name.c_str(), id.c_str());

            engageLogMsg(ENGAGE_LOG_LEVEL_WARNING, TAG, buff);

            g_currentState.isTransmitting = false;
        }        
    }));
}

void onEngineStarted(const char *pEventExtraJson)
{
    g_wq.submit(([]()
 	{
        engageLogMsg(ENGAGE_LOG_LEVEL_INFORMATIONAL, TAG, "onEngineStarted");
        createAllGroups();
    }));
}

bool registerCallbacks()
{
    EngageEvents_t callback;

    memset(&callback, 0, sizeof(callback) );

    callback.PFN_ENGAGE_GROUP_CREATED = onGroupCreated;
    callback.PFN_ENGAGE_GROUP_DELETED = onGroupDeleted;

    callback.PFN_ENGAGE_GROUP_DISCONNECTED = onGroupDisconnected;

    callback.PFN_ENGAGE_GROUP_CONNECTED = onGroupConnected;

    callback.PFN_ENGAGE_ENGINE_STARTED = onEngineStarted;

    callback.PFN_ENGAGE_GROUP_TX_STARTED = onGroupTxStarted;
    callback.PFN_ENGAGE_GROUP_TX_ENDED = onGroupTxEnded;
    callback.PFN_ENGAGE_GROUP_TX_FAILED = onGroupTxFailed;
    callback.PFN_ENGAGE_GROUP_TX_USURPED_BY_PRIORITY = onGroupTxUsurpedByPriority;
    callback.PFN_ENGAGE_GROUP_MAX_TX_TIME_EXCEEDED = onGroupMaxTxTimeExceeded;

    return (engageRegisterEventCallbacks(&callback) == ENGAGE_RESULT_OK);
}

void createAllGroups()
{  
    for(size_t x = 0; x < g_groups.size(); x++)
    {
        engageCreateGroup(g_groups[x].dump().c_str());
    }
}

void joinGroup(const char *pId)
{
    nlohmann::json *g = getGroup(pId);
    std::string id = g->at("id");
    int type = g->at("type");

    if(type == 2 )
    {
        engageJoinGroup(id.c_str());   
    }
    else if(type == 1 && !g_currentState.connected)
    {
        engageJoinGroup(id.c_str());
        g_currentState.connected = true;
    }
}

void deleteAllGroups()
{
    for(size_t i = 0; i < g_groups.size(); i++)
    {
        std::string id = g_groups[i].at("id");
        engageDeleteGroup(id.c_str());
    }
}

char* readFile(const char* fileName)
{
    FILE *fp;
    fp = fopen(fileName,"rb");

    fseek(fp,0, SEEK_END);
    int size = ftell(fp);
    fseek(fp,0,SEEK_SET);
    
    char* output = new char[size+1];
    
    size_t count = fread(output, 1, size+1, fp);
    output[count]=0;
    fclose(fp);

    return output;
}

void loadMission()
{
    char* missionJSON = readFile(MISSION_FILE_NAME);

    g_mission = nlohmann::json::parse(missionJSON);
	delete[] missionJSON;

	g_groups = g_mission.at("groups");
    if(g_useRp)
    {
        char* rp = readFile(RP_FILE_NAME);
        nlohmann::json rpJson = nlohmann::json::parse(rp);
        
        for(size_t i = 0; i < g_groups.size(); i++)
        {
            g_groups[i]["rallypoints"] = rpJson;
        }

        delete[] rp;
    } 
}

bool readInput(char *buff, size_t maxSize)
{
    if( !fgets(buff, maxSize, stdin) )
    {
        return false;
    }

    char *p = (buff + strlen(buff) - 1);
    while((p >= buff) && (*p == '\n'))
    {
        *p = 0;
        p--;
    }

    return true;
}

void showHelp()
{
    g_wq.submit(([]()
    {
        std::cout << "****************** HELP ************************" << std::endl;
        std::cout << "q  ......................... quit" << std::endl;
        std::cout << "l  ......................... toggle verbose logging" << std::endl;
        std::cout << "s  ......................... show status" << std::endl;
        std::cout << "g  ......................... show groups" << std::endl;
        std::cout << "n  ......................... next group" << std::endl;
        std::cout << "p  ......................... previous group" << std::endl;
        std::cout << "t  ......................... transmit on" << std::endl;
        std::cout << "x  ......................... transmit off" << std::endl;
        std::cout << "r  ......................... switch to rallypoint connection" << std::endl;
        std::cout << "m  ......................... switch to multicast connection" << std::endl;
        std::cout << "************************************************" << std::endl;
    }));
}

void showStatus()
{
    g_wq.submit(([]()
    {
        std::string id = g_groups[g_currentState.currentChannel].at("id");
        std::string nm = g_groups[g_currentState.currentChannel].at("name");
        std::cout << std::endl
                    << "****************** STATUS ************************" << std::endl
                    << "Mode   : " << (g_useRp ? "unicast (rallypoint)" : "multicast") << std::endl
                    << "Group  : " << nm << " (" << id << ")" << std::endl
                    << "Logging: " << (g_verboseLogging ? "verbose" : "limited") << std::endl
                    << "**************************************************" << std::endl
                    << std::endl;
    }));
}

void showGroups()
{
    g_wq.submit(([]()
    {
        std::cout << std::endl << "****************** GROUPS ************************" << std::endl;

        for(size_t i = 0; i < g_groups.size(); i++)
        {
            std::cout << g_groups[i].at("name") << " (" << g_groups[i].at("id") << ")";
            
            std::string id = g_groups[i].at("id");
            std::string comp = g_groups[g_currentState.currentChannel].at("id");
            if( id.compare(comp) == 0 )
            {
                std::cout <<  " <--- ACTIVE GROUP";
            }
            std::cout << std::endl;
        }

        std::cout << "**************************************************" << std::endl;
    }));
}

void showUnrecognizedCommand()
{
    g_wq.submit(([]()
    {
        std::cout << "error: not a recognized command" << std::endl;
    }));
}

void beginTransmitOnActiveGroup()
{
    g_wq.submit(([]()
    {
        std::string groupID = g_groups[g_currentState.currentChannel].at("id");
        engageBeginGroupTx(groupID.c_str(),0,0);
    }));
}

void endTransmitOnActiveGroup()
{
    g_wq.submit(([]()
    {
        std::string groupID = g_groups[g_currentState.currentChannel].at("id");
        engageEndGroupTx(groupID.c_str());
    }));
}

void switchToRpMode()
{
    g_wq.submit(([]()
    {
        g_useRp = true;
        g_reload = true;
        deleteAllGroups();
    }));
}

void switchToMcMode()
{
    g_wq.submit(([]()
    {
        g_useRp = false;
        g_reload = true;
        deleteAllGroups();
    }));
}

void goToNextGroup()
{        
    g_wq.submit(([]()
    {
        std::string group = g_groups[g_currentState.currentChannel].at("id");
        std::string name = g_groups[g_currentState.currentChannel].at("name");    

        g_currentState.currentChannel++;

        if((size_t)g_currentState.currentChannel >= g_groups.size())
        {
            g_currentState.currentChannel = 1;
        }

        std::string newGroup = g_groups[g_currentState.currentChannel].at("id");
        std::string newName = g_groups[g_currentState.currentChannel].at("name");

        std::cout << "leaving '" << name << "', going to '" << newName << std::endl;

        engageLeaveGroup(group.c_str());
        engageJoinGroup(newGroup.c_str());
    }));
}

void goToPreviousGroup()
{        
    g_wq.submit(([]()
    {
        std::string group = g_groups[g_currentState.currentChannel].at("id");
        std::string name = g_groups[g_currentState.currentChannel].at("name");    

        g_currentState.currentChannel--;

        if((size_t)g_currentState.currentChannel == 0)
        {
            g_currentState.currentChannel = g_groups.size() - 1;
        }

        std::string newGroup = g_groups[g_currentState.currentChannel].at("id");
        std::string newName = g_groups[g_currentState.currentChannel].at("name");

        std::cout << "leaving '" << name << "', going to '" << newName << std::endl;

        engageLeaveGroup(group.c_str());
        engageJoinGroup(newGroup.c_str());
    }));
}

void toggleVerboseLogging()
{
    g_wq.submit(([]()
    {
        g_verboseLogging = !g_verboseLogging;
        if(g_verboseLogging)
        {
            std::cout << "verbose logging enabled" << std::endl;
            engageSetLogLevel(ENGAGE_LOG_LEVEL_DEBUG);
        }
        else
        {
            std::cout << "verbose logging disabled" << std::endl;
            engageSetLogLevel(ENGAGE_LOG_LEVEL_INFORMATIONAL);
        }
    }));
}

// --------------- MAIN ---------------
int main(int argc, char *argv[])
{
    char *policyJson = readFile(POLICY_FILE_NAME); 
	char *identityJson = readFile(IDENTITY_FILE_NAME); 

    if(!registerCallbacks())
    {
        std::cerr << "Could not register callbacks" << std::endl;
        abort();
    }

    g_wq.start();
    
    loadMission();
    
    // We will start out at informatiob logging
    engageSetLogLevel(ENGAGE_LOG_LEVEL_INFORMATIONAL);

    engageInitialize(policyJson, identityJson, "");
    engageStart();
    
    while( true )
    {
        char buff[MAX_CMD_BUFF_SIZE];
        std::cout << "mingage > ";

        memset(buff, 0, sizeof(buff));
        if( !readInput(buff, sizeof(buff)) )
        {
            continue;
        }

        if( buff[0] == 'q' )
        {
            break;
        }
        else if( buff[0] == 'h' || buff[0] == '?' )
        {
            showHelp();
        }
        else if( buff[0] == 'l' )
        {
            toggleVerboseLogging();
        }
        else if( buff[0] == 's' )
        {
            showStatus();
        }
        else if( buff[0] == 'n' )
        {
            goToNextGroup();
        }
        else if( buff[0] == 'p' )
        {
            goToPreviousGroup();
        }
        else if( buff[0] == 't' )
        {
            beginTransmitOnActiveGroup();
        }
        else if( buff[0] == 'x' )
        {
            endTransmitOnActiveGroup();
        }
        else if( buff[0] == 'r' )
        {
            switchToRpMode();
        }
        else if( buff[0] == 'm' )
        {
            switchToMcMode();
        }
        else if( buff[0] == 'g' )
        {
            showGroups();
        }
        else if( buff[0] != '\0' )
        {
            showUnrecognizedCommand();
        }
    }

    engageStop();
    engageShutdown();

	delete[] policyJson;
	delete[] identityJson;

    g_wq.stop();
    
    return 0;
}
