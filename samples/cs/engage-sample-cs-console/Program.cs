//
//  Copyright (c) 2018 Rally Tactical Systems, Inc.
//  All rights reserved.
//

using Newtonsoft.Json.Linq;
using System;
using System.IO;

namespace engage_sample_cs_console
{
    class Program : Engage.IEngineNotifications, 
                    Engage.IRallypointNotifications, 
                    Engage.IGroupNotifications,
                    Engage.ILicenseNotifications
    {
        private class GroupDescriptor
        {
            public string jsonConfiguration;
            public string id;
            public string name;
            public bool isEncrypted;
            public bool allowsFullduplex;
        }

        private Engage _engage = new Engage();
        private GroupDescriptor[] _groups = null;
        private int _txPriority = 0;
        private int _txFlags = 0;

        bool loadPolicy(string fn, ref JObject policy)
        {
            bool rc = false;

            try
            {
                String jsonData;

                using (StreamReader sr = new StreamReader(fn))
                {
                    jsonData = sr.ReadToEnd();
                }

                policy = JObject.Parse(jsonData);

                rc = true;
            }
            catch (Exception e)
            {
                rc = false;
                Console.WriteLine("The file '" + fn + "' could not be read or parsed: ");
                Console.WriteLine(e.Message);
            }

            return rc;
        }

        bool loadMission(string fn, ref JObject mission)
        {
            bool rc = false;

            try
            {
                String jsonData;

                using (StreamReader sr = new StreamReader(fn))
                {
                    jsonData = sr.ReadToEnd();                    
                }

                mission = JObject.Parse(jsonData);
                JArray groups = (JArray)mission["groups"];
                int count = 0;
                foreach (JObject group in groups)
                {
                    count++;
                }

                if (count > 0)
                {
                    _groups = new GroupDescriptor[count];

                    int index = 0;
                    foreach (JObject group in groups)
                    {
                        JToken opt;
                        string tmp;

                        GroupDescriptor gd = new GroupDescriptor();
                        gd.jsonConfiguration = group.ToString();
                        gd.id = (string)group["id"];
                        gd.name = (string)group["name"];

                        opt = group["cryptoPassword"];
                        tmp = ((opt == null) ? "" : (string)opt);
                        gd.isEncrypted = !(tmp.Trim().Length == 0);

                        opt = group["fdx"];
                        gd.allowsFullduplex = ((opt == null) ? false : (bool)opt);

                        _groups[index] = gd;
                        index++;
                    }
                }

                rc = true;
            }
            catch (Exception e)
            {
                _groups = null;
                rc = false;
                Console.WriteLine("The file '" + fn + "' could not be read or parsed: ");
                Console.WriteLine(e.Message);
            }

            return rc;
        }

        void showHelp()
        {
            Console.WriteLine("q.............quit");
            Console.WriteLine("sg............show group list");
            Console.WriteLine("c<N>..........create group index N");
            Console.WriteLine("d<N>..........delete group index N");
            Console.WriteLine("ca............create all groups");
            Console.WriteLine("da............delete all groups");
            Console.WriteLine("j<N>..........join group index N");
            Console.WriteLine("ja............join all groups");
            Console.WriteLine("l<N>..........leave group index N");
            Console.WriteLine("la............leave all groups");
            Console.WriteLine("b<N>..........begin tx on group index N");
            Console.WriteLine("ba............begin tx on all groups");
            Console.WriteLine("e<N>..........end tx on group index N");
            Console.WriteLine("ea............end tx on all groups");
            Console.WriteLine("m<N>..........mute rx on group index N");
            Console.WriteLine("ma............mute rx on all groups");
            Console.WriteLine("u<N>..........unmute rx on group index N");
            Console.WriteLine("ua............unmute rx on all groups");
        }

        void showGroups()
        {
            Console.WriteLine("Groups:");
            int x = 0;
            foreach(GroupDescriptor gd in _groups)
            {
                Console.WriteLine("index=" + x
                          + ", id=" + gd.id
                          + ", name=" + gd.name
                          + ", encrypted=" + (gd.isEncrypted ? "yes" : "no")
                          + ", fullDuplex=" + (gd.allowsFullduplex ? "yes" : "no")
                         );

                x++;
            }
        }

        void showUsage()
        {
            Console.WriteLine("usage: engage-sample-cs-console -ep:<policy_file> -mission:<mission_file> [-cs:<cert_store_file> [-csp:<cert_store_password_hex>]]");
        }

        void run(string[] args)
        {
            string policyFile = null;
            string missionFile = null;
            string certStoreFile = null;
            string certStorePwd = null;

            for (int x = 0; x < args.Length; x++)
            {
                if(args[x].StartsWith("-mission:"))
                {
                    missionFile = args[x].Substring(9);
                }
                else if (args[x].StartsWith("-ep:"))
                {
                    policyFile = args[x].Substring(4);
                }
                else if (args[x].StartsWith("-cs:"))
                {
                    certStoreFile = args[x].Substring(4);
                }
                else if (args[x].StartsWith("-csp:"))
                {
                    certStorePwd = args[x].Substring(5);
                }
            }

            if (policyFile == null || missionFile == null)
            {
                showUsage();
                return;
            }

            if(certStoreFile != null)
            {
                if(certStorePwd == null)
                {
                    certStorePwd = "";
                }

                _engage.openCertStore(certStoreFile, certStorePwd);
            }

            int rc;
            JObject policy = new JObject();
            JObject mission = new JObject();

            if (!loadPolicy(policyFile, ref policy))
            {
                return;
            }

            if (!loadMission(missionFile, ref mission))
            {
                return;
            }

            if(_groups == null || _groups.Length == 0)
            {
                Console.WriteLine("no groups found in the configuration");
                return;
            }
            else
            {
                Console.WriteLine("found the following groups: ");
                foreach(GroupDescriptor g in _groups)
                {
                    Console.WriteLine("id='" + g.id + "'"
                                      + ", name='" + g.name + "'"
                                      + ", encrypted=" + g.isEncrypted
                                      + ", allowsFullDuplex=" + g.allowsFullduplex);
                }
            }

            // Subscribe for all notifications
            _engage.subscribe((Engage.IEngineNotifications)this);
            _engage.subscribe((Engage.IRallypointNotifications)this);
            _engage.subscribe((Engage.IGroupNotifications)this);
            _engage.subscribe((Engage.ILicenseNotifications)this);

            rc = _engage.initialize(policy.ToString(), "{}", null);
            if (rc != Engage.ENGAGE_RESULT_OK)
            {
                Console.WriteLine("initialize failed");
                return;
            }

            rc = _engage.start();
            if (rc != Engage.ENGAGE_RESULT_OK)
            {
                Console.WriteLine("start failed");
                return;
            }

            #region Command processor
            while (true)
            {
                Console.Write("engage-c#: ");
                string s = Console.ReadLine();
                if (s.Equals("q"))
                {
                    break;
                }
                else if (s.Equals("?"))
                {
                    showHelp();
                }
                else if (s.Equals("sg"))
                {
                    showGroups();
                }
                else if (s.StartsWith("c"))
                {
                    try
                    {
                        int idx = s.Substring(1) == "a" ? -1 : Int32.Parse(s.Substring(1));
                        if(idx == -1)
                        {
                            foreach(GroupDescriptor gd in _groups)
                            {
                                rc = _engage.createGroup(gd.jsonConfiguration);
                                if(rc != Engage.ENGAGE_RESULT_OK)
                                {
                                    break;
                                }
                            }
                        }
                        else
                        {
                            rc = _engage.createGroup(_groups[idx].jsonConfiguration);
                        }
                        
                        if (rc != Engage.ENGAGE_RESULT_OK)
                        {
                            Console.WriteLine("request failed");
                        }

                    }
                    catch (Exception)
                    {
                        Console.WriteLine("invalid command");
                    }
                }
                else if (s.StartsWith("d"))
                {
                    try
                    {
                        int idx = s.Substring(1) == "a" ? -1 : Int32.Parse(s.Substring(1));
                        if (idx == -1)
                        {
                            foreach (GroupDescriptor gd in _groups)
                            {
                                rc = _engage.deleteGroup(gd.id);
                                if (rc != Engage.ENGAGE_RESULT_OK)
                                {
                                    break;
                                }
                            }
                        }
                        else
                        {
                            rc = _engage.deleteGroup(_groups[idx].id);
                        }

                        if (rc != Engage.ENGAGE_RESULT_OK)
                        {
                            Console.WriteLine("request failed");
                        }

                    }
                    catch (Exception)
                    {
                        Console.WriteLine("invalid command");
                    }
                }
                else if (s.StartsWith("j"))
                {
                    try
                    {
                        int idx = s.Substring(1) == "a" ? -1 : Int32.Parse(s.Substring(1));
                        if (idx == -1)
                        {
                            foreach (GroupDescriptor gd in _groups)
                            {
                                rc = _engage.joinGroup(gd.id);
                                if (rc != Engage.ENGAGE_RESULT_OK)
                                {
                                    break;
                                }
                            }
                        }
                        else
                        {
                            rc = _engage.joinGroup(_groups[idx].id);
                        }

                        if (rc != Engage.ENGAGE_RESULT_OK)
                        {
                            Console.WriteLine("request failed");
                        }

                    }
                    catch (Exception)
                    {
                        Console.WriteLine("invalid command");
                    }
                }
                else if (s.StartsWith("l"))
                {
                    try
                    {
                        int idx = s.Substring(1) == "a" ? -1 : Int32.Parse(s.Substring(1));
                        if (idx == -1)
                        {
                            foreach (GroupDescriptor gd in _groups)
                            {
                                rc = _engage.leaveGroup(gd.id);
                                if (rc != Engage.ENGAGE_RESULT_OK)
                                {
                                    break;
                                }
                            }
                        }
                        else
                        {
                            rc = _engage.leaveGroup(_groups[idx].id);
                        }

                        if (rc != Engage.ENGAGE_RESULT_OK)
                        {
                            Console.WriteLine("request failed");
                        }

                    }
                    catch (Exception)
                    {
                        Console.WriteLine("invalid command");
                    }
                }
                else if (s.StartsWith("b"))
                {
                    try
                    {
                        int idx = s.Substring(1) == "a" ? -1 : Int32.Parse(s.Substring(1));
                        if (idx == -1)
                        {
                            foreach (GroupDescriptor gd in _groups)
                            {
                                rc = _engage.beginGroupTx(gd.id, _txPriority, _txFlags);
                                if (rc != Engage.ENGAGE_RESULT_OK)
                                {
                                    break;
                                }
                            }
                        }
                        else
                        {
                            rc = _engage.beginGroupTx(_groups[idx].id, _txPriority, _txFlags);
                        }

                        if (rc != Engage.ENGAGE_RESULT_OK)
                        {
                            Console.WriteLine("request failed");
                        }

                    }
                    catch (Exception)
                    {
                        Console.WriteLine("invalid command");
                    }
                }
                else if (s.StartsWith("e"))
                {
                    try
                    {
                        int idx = s.Substring(1) == "a" ? -1 : Int32.Parse(s.Substring(1));
                        if (idx == -1)
                        {
                            foreach (GroupDescriptor gd in _groups)
                            {
                                rc = _engage.endGroupTx(gd.id);
                                if (rc != Engage.ENGAGE_RESULT_OK)
                                {
                                    break;
                                }
                            }
                        }
                        else
                        {
                            rc = _engage.endGroupTx(_groups[idx].id);
                        }

                        if (rc != Engage.ENGAGE_RESULT_OK)
                        {
                            Console.WriteLine("request failed");
                        }

                    }
                    catch (Exception)
                    {
                        Console.WriteLine("invalid command");
                    }
                }
                else if (s.StartsWith("m"))
                {
                    try
                    {
                        int idx = s.Substring(1) == "a" ? -1 : Int32.Parse(s.Substring(1));
                        if (idx == -1)
                        {
                            foreach (GroupDescriptor gd in _groups)
                            {
                                rc = _engage.muteGroupRx(gd.id);
                                if (rc != Engage.ENGAGE_RESULT_OK)
                                {
                                    break;
                                }
                            }
                        }
                        else
                        {
                            rc = _engage.muteGroupRx(_groups[idx].id);
                        }

                        if (rc != Engage.ENGAGE_RESULT_OK)
                        {
                            Console.WriteLine("request failed");
                        }

                    }
                    catch (Exception)
                    {
                        Console.WriteLine("invalid command");
                    }
                }
                else if (s.StartsWith("u"))
                {
                    try
                    {
                        int idx = s.Substring(1) == "a" ? -1 : Int32.Parse(s.Substring(1));
                        if (idx == -1)
                        {
                            foreach (GroupDescriptor gd in _groups)
                            {
                                rc = _engage.unmuteGroupRx(gd.id);
                                if (rc != Engage.ENGAGE_RESULT_OK)
                                {
                                    break;
                                }
                            }
                        }
                        else
                        {
                            rc = _engage.unmuteGroupRx(_groups[idx].id);
                        }

                        if (rc != Engage.ENGAGE_RESULT_OK)
                        {
                            Console.WriteLine("request failed");
                        }

                    }
                    catch (Exception)
                    {
                        Console.WriteLine("invalid command");
                    }
                }
            }
            #endregion

            // Unsubscribe
            _engage.unsubscribe((Engage.IEngineNotifications)this);
            _engage.unsubscribe((Engage.IRallypointNotifications)this);
            _engage.unsubscribe((Engage.IGroupNotifications)this);
            _engage.unsubscribe((Engage.ILicenseNotifications)this);

            rc = _engage.shutdown();
            if (rc != Engage.ENGAGE_RESULT_OK)
            {
                Console.WriteLine("shutdown failed");
                return;
            }
        }

        static void Main(string[] args)
        {
            Console.WriteLine("---------------------------------------------------------------------------------");
            Console.WriteLine("engage-sample-cs-console");
            Console.WriteLine("Copyright (c) 2018 Rally Tactical Systems, Inc.");
            Console.WriteLine("---------------------------------------------------------------------------------");

            new Program().run(args);
        }

        #region Notification handlers
        void Engage.IEngineNotifications.onEngineStarted(string eventExtraJson)
        {
            Console.WriteLine("C#: onEngineStarted");
        }

        void Engage.IEngineNotifications.onEngineStopped(string eventExtraJson)
        {
            Console.WriteLine("C#: onEngineStopped");
        }

        void Engage.IRallypointNotifications.onRallypointPausingConnectionAttempt(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onRallypointPausingConnectionAttempt: " + id);
        }

        void Engage.IRallypointNotifications.onRallypointConnecting(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onRallypointConnecting: " + id);
        }

        void Engage.IRallypointNotifications.onRallypointConnected(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onRallypointConnected: " + id);
        }

        void Engage.IRallypointNotifications.onRallypointDisconnected(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onRallypointDisconnected: " + id);
        }

        void Engage.IRallypointNotifications.onRallypointRoundtripReport(string id, int rtMs, int rtRating, string eventExtraJson)
        {
            Console.WriteLine("C#: onRallypointRoundtripReport: " + id);
        }

        void Engage.IGroupNotifications.onGroupCreated(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupCreated: " + id);
        }

        void Engage.IGroupNotifications.onGroupCreateFailed(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupCreateFailed: " + id);
        }

        void Engage.IGroupNotifications.onGroupDeleted(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupDeleted: " + id);
        }

        void Engage.IGroupNotifications.onGroupConnected(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupConnected: " + id);
        }

        void Engage.IGroupNotifications.onGroupConnectFailed(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupConnectFailed: " + id);
        }

        void Engage.IGroupNotifications.onGroupDisconnected(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupDisconnected: " + id);
        }

        void Engage.IGroupNotifications.onGroupJoined(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupJoined: " + id);
        }

        void Engage.IGroupNotifications.onGroupJoinFailed(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupJoinFailed: " + id);
        }

        void Engage.IGroupNotifications.onGroupLeft(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupLeft: " + id);
        }

        void Engage.IGroupNotifications.onGroupMemberCountChanged(string id, int newCount, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupMemberCountChanged: " + id + ", newCount=" + newCount);
        }

        void Engage.IGroupNotifications.onGroupRxStarted(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupRxStarted: " + id);
        }

        void Engage.IGroupNotifications.onGroupRxEnded(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupRxEnded: " + id);
        }

        void Engage.IGroupNotifications.onGroupRxSpeakersChanged(string id, string groupTalkerJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupRxSpeakersChanged: " + id + ", [" + groupTalkerJson + "]");
        }

        void Engage.IGroupNotifications.onGroupTxStarted(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupTxStarted: " + id);
        }

        void Engage.IGroupNotifications.onGroupTxEnded(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupTxEnded: " + id);
        }

        void Engage.IGroupNotifications.onGroupTxFailed(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupTxFailed: " + id);
        }

        void Engage.IGroupNotifications.onGroupTxUsurpedByPriority(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupTxUsurpedByPriority: " + id);
        }

        void Engage.IGroupNotifications.onGroupMaxTxTimeExceeded(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupMaxTxTimeExceeded: " + id);
        }

        void Engage.IGroupNotifications.onGroupRxMuted(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupRxMuted: " + id);
        }

        void Engage.IGroupNotifications.onGroupRxUnmuted(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupRxUnmuted: " + id);
        }

        void Engage.IGroupNotifications.onGroupNodeDiscovered(string id, string nodeJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupNodeDiscovered: " + id + ", " + nodeJson);
        }

        void Engage.IGroupNotifications.onGroupNodeRediscovered(string id, string nodeJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupNodeRediscovered: " + id + ", " + nodeJson);
        }

        void Engage.IGroupNotifications.onGroupNodeUndiscovered(string id, string nodeJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupNodeUndiscovered: " + id + ", " + nodeJson);
        }

        void Engage.IGroupNotifications.onGroupAssetDiscovered(string id, string nodeJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupAssetDiscovered: " + id + ", " + nodeJson);
        }

        void Engage.IGroupNotifications.onGroupAssetRediscovered(string id, string nodeJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupAssetRediscovered: " + id + ", " + nodeJson);
        }

        void Engage.IGroupNotifications.onGroupAssetUndiscovered(string id, string nodeJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupAssetUndiscovered: " + id + ", " + nodeJson);
        }
                
        void Engage.IGroupNotifications.onGroupBlobSent(string id, string eventExtraJson)
        {  
            Console.WriteLine("C#: onGroupBlobSent: " + id);          
        }

        void Engage.IGroupNotifications.onGroupBlobSendFailed(string id, string eventExtraJson)
        {  
            Console.WriteLine("C#: onGroupBlobSendFailed: " + id);          
        }

        void Engage.IGroupNotifications.onGroupBlobReceived(string id, string blobInfoJson, byte[] blob, int blobSize, string eventExtraJson)
        {  
            Console.WriteLine("C#: onGroupBlobReceived: " + id + ", " + blobInfoJson);          
        }

        void Engage.IGroupNotifications.onGroupRtpSent(string id, string eventExtraJson)
        {  
            Console.WriteLine("C#: onGroupRtpSent: " + id);          
        }

        void Engage.IGroupNotifications.onGroupRtpSendFailed(string id, string eventExtraJson)
        {  
            Console.WriteLine("C#: onGroupRtpSendFailed: " + id);          
        }

        void Engage.IGroupNotifications.onGroupRtpReceived(string id, string rtpInfoJson, byte[] payload, int payloadSize, string eventExtraJson)
        {  
            Console.WriteLine("C#: onGroupRtpReceived: " + id + ", " + rtpInfoJson);          
        }

        void Engage.IGroupNotifications.onGroupRawSent(string id, string eventExtraJson)
        {  
            Console.WriteLine("C#: onGroupRawSent: " + id);          
        }

        void Engage.IGroupNotifications.onGroupRawSendFailed(string id, string eventExtraJson)
        {  
            Console.WriteLine("C#: onGroupRawSendFailed: " + id);          
        }

        void Engage.IGroupNotifications.onGroupRawReceived(string id, byte[] raw, int rawSize, string eventExtraJson)
        {  
            Console.WriteLine("C#: onGroupRawReceived: " + id);          
        }

        void Engage.ILicenseNotifications.onLicenseChanged(string eventExtraJson)
        {
            Console.WriteLine("C#: onLicenseChanged");
        }
        
        void Engage.ILicenseNotifications.onLicenseExpired(string eventExtraJson)
        {
            Console.WriteLine("C#: onLicenseExpired");
        }
                
        void Engage.ILicenseNotifications.onLicenseExpiring(double secondsLeft, string eventExtraJson)
        {
            Console.WriteLine("C#: onLicenseExpiring: " + secondsLeft);
        }

        public void onGroupTimelineEventStarted(string id, string eventJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupTimelineEventStarted: " + id + ", event=" + eventJson);
        }

        public void onGroupTimelineEventUpdated(string id, string eventJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupTimelineEventUpdated: " + id + ", event=" + eventJson);
        }

        public void onGroupTimelineEventEnded(string id, string eventJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupTimelineEventEnded: " + id + ", event=" + eventJson);
        }

        public void onGroupTimelineReport(string id, string reportJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupTimelineReport: " + id + ", event=" + reportJson);
        }

        public void onGroupTimelineReportFailed(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupTimelineReportFailed: " + id);
        }

        public void onGroupTxMuted(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupTxMuted: " + id);
        }

        public void onGroupTxUnmuted(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupTxUnmuted: " + id);
        }

        public void onGroupTimelineGroomed(string id, string eventListJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupTimelineGroomed: " + id);
        }

        public void onGroupHealthReport(string id, string healthReportJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupHealthReport: " + id);
        }

        public void onGroupHealthReportFailed(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupHealthReportFailed: " + id);
        }

        public void onGroupStatsReport(string id, string statsReportJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupStatsReport: " + id);
        }

        public void onGroupStatsReportFailed(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupStatsReportFailed: " + id);
        }
        #endregion
    }
}
