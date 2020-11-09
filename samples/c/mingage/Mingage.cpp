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

typedef struct
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
            std::cout << "****************** HELP ************************" << std::endl;
            std::cout << "quit/q    ......................... quit" << std::endl;
            std::cout << "status/s  ......................... show status" << std::endl;
            std::cout << "next/n    ......................... next channel" << std::endl;
            std::cout << "txon/t    ......................... transmit on" << std::endl;
            std::cout << "txoff/x   ......................... transmit off" << std::endl;
            std::cout << "rp/r      ......................... switch to rallypoint connection" << std::endl;
            std::cout << "mc/m      ......................... switch to multicast connection" << std::endl;
            std::cout << "************************************************" << std::endl;
        }
        else if( buff[0] == 's' )
        {
            g_wq.submit(([]()
            {
                std::string id = g_groups[g_currentState.currentChannel].at("id");
                std::string nm = g_groups[g_currentState.currentChannel].at("name");
                std::cout << std::endl
                          << "****************** STATUS ************************" << std::endl
                          << "Mode : " << (g_useRp ? "unicast (rallypoint)" : "multicast") << std::endl
                          << "Group: " << nm << " (" << id << ")" << std::endl
                          << "**************************************************" << std::endl
                          << std::endl;
            }));
        }
        else if( buff[0] == 'n' )
        {
            g_wq.submit(([]()
            {
                std::string group = g_groups[g_currentState.currentChannel].at("id");
                std::string name = g_groups[g_currentState.currentChannel].at("name");    
                engageLeaveGroup(group.c_str());

                g_currentState.currentChannel++;

                if((size_t)g_currentState.currentChannel >= g_groups.size())
                {
                    g_currentState.currentChannel = 1;
                }

                group = g_groups[g_currentState.currentChannel].at("id");
                engageJoinGroup(group.c_str());
            }));
        }
        else if( buff[0] == 't' )
        {
            g_wq.submit(([]()
            {
                std::string groupID = g_groups[g_currentState.currentChannel].at("id");
                engageBeginGroupTx(groupID.c_str(),0,0);
            }));
        }
        else if( buff[0] == 'x' )
        {
            g_wq.submit(([]()
            {
                std::string groupID = g_groups[g_currentState.currentChannel].at("id");
                engageEndGroupTx(groupID.c_str());
            }));
        }
        else if( buff[0] == 'r' )
        {
            g_wq.submit(([]()
            {
                g_useRp = true;
                g_reload = true;
                deleteAllGroups();
            }));
        }
        else if( buff[0] == 'm' )
        {
            g_wq.submit(([]()
            {
                g_useRp = false;
                g_reload = true;
                deleteAllGroups();
            }));
        }
    }

    engageStop();
    engageShutdown();

	delete[] policyJson;
	delete[] identityJson;

    g_wq.stop();
    
    return 0;
}
