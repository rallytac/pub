//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

using Newtonsoft.Json.Linq;
using System;
using System.IO;
using System.Threading;
using System.Collections.Generic;
using System.Text;

namespace engage_sample_cs_console
{
    class Program : Engage.IEngineNotifications, 
                    Engage.IRallypointNotifications, 
                    Engage.IGroupNotifications,
                    Engage.ILicenseNotifications,
                    Engage.ILoggingNotifications,
                    Engage.IAudioDeviceControlNotifications
    {
        public class Adad
        {
            static public int SAMPLING_RATE = 8000;
            static public int CHANNELS = 1;
            static public int MS_PER_FRAME = 10;
            static public int SAMPLES_PER_FRAME = (((SAMPLING_RATE / 1000) * MS_PER_FRAME) * CHANNELS);

            static private int _nextAdadInstanceId = 0;

            public Adad()
            {
                _nextAdadInstanceId++;
                isRunning = false;
                direction = Engage.AdadDirection.UNDEFINED;
                instanceId = _nextAdadInstanceId;
                engageDeviceId = 0;
            }

            public bool isRunning;
            public Engage.AdadDirection direction;
            public int instanceId;
            public int engageDeviceId;
        }


        private class GroupDescriptor
        {
            public JObject baseObject;
            public string id;
            public int type;
            public string name;
            public bool isEncrypted;
            public bool allowsFullduplex;
            public Adad speakerAdad;
            public Adad microphoneAdad;
            public Engage.AdadAudioTransferProxy adadAudioTransferProxy;
            public Int16[] speakerAudioBuffer;
            public Int16[] microphoneAudioBuffer;

            public GroupDescriptor()
            {
                type = 0;
                isEncrypted = false;
                allowsFullduplex = false;
                speakerAdad = null;
                microphoneAdad = null;
                adadAudioTransferProxy = null;
                speakerAudioBuffer = null;
                microphoneAudioBuffer = null;
            }

            public string finalizeCreationJson()
            {
                if (speakerAdad != null && speakerAdad.engageDeviceId != 0)
                {
                    if(!baseObject.ContainsKey("audio"))
                    {
                        baseObject["audio"] = new JObject();
                    }

                    baseObject["audio"]["outputId"] = speakerAdad.engageDeviceId;
                }
                else
                {
                    try
                    {
                        baseObject["audio"]["outputId"].Remove();
                    }
                    catch(Exception)
                    {
                    }                    
                }

                if (microphoneAdad != null && microphoneAdad.engageDeviceId != 0)
                {
                    if (!baseObject.ContainsKey("audio"))
                    {
                        baseObject["audio"] = new JObject();
                    }

                    baseObject["audio"]["inputId"] = microphoneAdad.engageDeviceId;
                }
                else
                {
                    try
                    {
                        baseObject["audio"]["inputId"].Remove();
                    }
                    catch (Exception)
                    {
                    }
                }

                return baseObject.ToString();
            }
        }

        private Engage _engage = new Engage();
        private GroupDescriptor[] _groups = null;
        private int _txPriority = 0;
        private int _txFlags = 0;
        private bool _useAdad = false;
        private bool _autoCreateAndJoin = false;

        private bool _runAdadSpeakerThread = false;
        private Thread _adadSpeakerThreadHandle = null;
        private bool _runAdadMicrophoneThread = false;
        private Thread _adadMicrophoneThreadHandle = null;

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

        bool loadMission(string missionFn, string rpFn, ref JObject mission)
        {
            bool rc = false;

            try
            {
                String jsonData;
                JObject rp = null;

                using (StreamReader sr = new StreamReader(missionFn))
                {
                    jsonData = sr.ReadToEnd();                    
                }

                if(rpFn != null)
                {
                    String rpJsonData;
                    using (StreamReader sr = new StreamReader(rpFn))
                    {
                        rpJsonData = sr.ReadToEnd();
                    }

                    rp = JObject.Parse(rpJsonData);
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

                        if(rp != null)
                        {
                            if(group.ContainsKey("rallypoints"))
                            {
                                group.Remove("rallypoints");
                            }

                            group["rallypoints"] = new JArray();
                            ((JArray)group["rallypoints"]).Add(rp);
                        }

                        gd.baseObject = group;                        
                        gd.id = (string)group["id"];
                        gd.type = (int)group["type"];
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
                          + ", type=" + gd.type
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
            Console.WriteLine("usage: engage-sample-cs-console -ep:<policy_file> -mi:<mission_file> [-rp:<rp_file>] [-cs:<cert_store_file> [-csp:<cert_store_password_hex>]] [-fcpath:<full_path_to_fips_crypto_module>] -useadad");
        }

        void doCreateGroup(int idx)
        {
            int rc = Engage.ENGAGE_RESULT_GENERAL_FAILURE;

            if (idx == -1)
            {
                foreach (GroupDescriptor gd in _groups)
                {
                    rc = _engage.createGroup(gd.finalizeCreationJson());
                    if (rc != Engage.ENGAGE_RESULT_OK)
                    {
                        break;
                    }
                }
            }
            else
            {
                rc = _engage.createGroup(_groups[idx].finalizeCreationJson());
            }

            if (rc != Engage.ENGAGE_RESULT_OK)
            {
                Console.WriteLine("request failed");
            }
        }

        void doDeleteGroup(int idx)
        {
            int rc = Engage.ENGAGE_RESULT_GENERAL_FAILURE;

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

        void doJoinGroup(int idx)
        {
            int rc = Engage.ENGAGE_RESULT_GENERAL_FAILURE;

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

        void doLeaveGroup(int idx)
        {
            int rc = Engage.ENGAGE_RESULT_GENERAL_FAILURE;

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

        private GroupDescriptor getGroupDescriptorForEngageDeviceIdd(int deviceId)
        {
            foreach (GroupDescriptor gd in _groups)
            {
                if (gd.speakerAdad != null && gd.speakerAdad.engageDeviceId == deviceId)
                {
                    return gd;
                }
                else if (gd.microphoneAdad != null && gd.microphoneAdad.engageDeviceId == deviceId)
                {
                    return gd;
                }
            }

            return null;
        }

        private Adad getAdadForEngageDeviceId(int deviceId)
        {
            GroupDescriptor gd = getGroupDescriptorForEngageDeviceIdd(deviceId);
            if (gd != null)
            {
                if (gd.speakerAdad != null && gd.speakerAdad.engageDeviceId == deviceId)
                {
                    return gd.speakerAdad;
                }
                else if (gd.microphoneAdad != null && gd.microphoneAdad.engageDeviceId == deviceId)
                {
                    return gd.microphoneAdad;
                }
            }

            return null;
        }

        private void devTest1()
        {
            Console.WriteLine("devtest1");

            byte[] rawBuffer = new byte[32];
            for(int x = 0; x < rawBuffer.Length; x++)
            {
                rawBuffer[x] = (byte)x;
            }

            _engage.sendGroupRaw("g2", rawBuffer, rawBuffer.Length, "");
        }

        private void devTest2()
        {
            Console.WriteLine("devtest2");
        }

        void run(string[] args)
        {
            string policyFile = null;
            string missionFile = null;
            string certStoreFile = null;
            string certStorePwd = null;
            string rpFile = null;
            string fipsPath = null;

            for (int x = 0; x < args.Length; x++)
            {
                if(args[x].StartsWith("-mission:"))
                {
                    missionFile = args[x].Substring(9);
                }
                if (args[x].StartsWith("-mi:"))
                {
                    missionFile = args[x].Substring(4);
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
                else if (args[x].StartsWith("-rp:"))
                {
                    rpFile = args[x].Substring(4);
                }
                else if (args[x].Equals("-useadad"))
                {
                    _useAdad = true;
                }
                else if (args[x].Equals("-acj"))
                {
                    _autoCreateAndJoin = true;
                }
                else if (args[x].StartsWith("-fcpath:"))
                {
                    fipsPath = args[x].Substring(8);
                }                
            }

            if (policyFile == null || missionFile == null)
            {
                showUsage();
                return;
            }

            if(fipsPath != null)
            {
                JObject fipsCryptoSettings = new JObject();
                fipsCryptoSettings["enabled"] = true;
                fipsCryptoSettings["path"] = fipsPath;
                fipsCryptoSettings["debug"] = false;

                if(_engage.setFipsCrypto(fipsCryptoSettings.ToString()) != Engage.ENGAGE_RESULT_OK)
                {
                    showUsage();
                    return;
                }
            }

            if (certStoreFile != null)
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

            if (!loadMission(missionFile, rpFile, ref mission))
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
            //_engage.subscribe((Engage.ILoggingNotifications)this);
            _engage.subscribe((Engage.IEngineNotifications)this);
            _engage.subscribe((Engage.IRallypointNotifications)this);
            _engage.subscribe((Engage.IGroupNotifications)this);
            _engage.subscribe((Engage.ILicenseNotifications)this);
            _engage.subscribe((Engage.IAudioDeviceControlNotifications)this);

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

            // Are we using ADADs?
            if(_useAdad)
            {
                foreach (GroupDescriptor gd in _groups)
                {
                    gd.adadAudioTransferProxy = new Engage.AdadAudioTransferProxy();
                    gd.speakerAudioBuffer = new Int16[Adad.SAMPLES_PER_FRAME];
                    gd.microphoneAudioBuffer = new Int16[Adad.SAMPLES_PER_FRAME];

                    gd.speakerAdad = new Adad();
                    gd.speakerAdad.direction = Engage.AdadDirection.OUTPUT;
                    gd.speakerAdad.engageDeviceId = _engage.audioDeviceRegister(gd.speakerAdad.direction, Adad.SAMPLING_RATE, Adad.CHANNELS);

                    gd.microphoneAdad = new Adad();
                    gd.microphoneAdad.direction = Engage.AdadDirection.INPUT;
                    gd.microphoneAdad.engageDeviceId = _engage.audioDeviceRegister(gd.microphoneAdad.direction, Adad.SAMPLING_RATE, Adad.CHANNELS);
                }
            }

            // Auto create & join?
            if(_autoCreateAndJoin)
            {
                doCreateGroup(-1);
                doJoinGroup(-1);
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
                else if (s.StartsWith("`"))
                {
                    try
                    {
                        int idx = s.Substring(1) == "`" ? -1 : Int32.Parse(s.Substring(1));
                        if(idx == 1)
                        {
                            devTest1();
                        }
                        else if (idx == 2)
                        {
                            devTest2();
                        }
                        else
                        {
                            throw new Exception();
                        }
                    }
                    catch (Exception)
                    {
                        Console.WriteLine("invalid command");
                    }
                }
                else if (s.StartsWith("c"))
                {
                    try
                    {
                        int idx = s.Substring(1) == "a" ? -1 : Int32.Parse(s.Substring(1));
                        doCreateGroup(idx);
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
                        doDeleteGroup(idx);
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
                        doJoinGroup(idx);
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
                        doLeaveGroup(idx);
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
            _engage.unsubscribe((Engage.ILoggingNotifications)this);

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
            Console.WriteLine("Copyright (c) 2019 Rally Tactical Systems, Inc.");
            Console.WriteLine("---------------------------------------------------------------------------------");

            new Program().run(args);
        }

        private string hexStringOf(byte[] buffer, int size, string sep)
        {
            StringBuilder hex = new StringBuilder(buffer.Length * 2);

            foreach (byte b in buffer)
            {
                if(sep != null)
                {
                    if (hex.Length > 0)
                    {
                        hex.Append(sep);
                    }
                }

                hex.AppendFormat("{0:x2}", b);
            }

            return hex.ToString();
        }

        private void adadSpeakerThread()
        {
            while (_runAdadSpeakerThread)
            {
                Thread.Sleep(10);

                if (!_runAdadSpeakerThread)
                {
                    break;
                }

                foreach (GroupDescriptor gd in _groups)
                {
                    if (gd.speakerAdad.engageDeviceId != 0)
                    {
                        if (gd.speakerAdad.isRunning)
                        {
                            if (gd.adadAudioTransferProxy != null)
                            {
                                int samplesRead = gd.adadAudioTransferProxy.readSpeakerAudio(ref gd.speakerAudioBuffer, Adad.SAMPLES_PER_FRAME, gd.speakerAdad.engageDeviceId, gd.speakerAdad.instanceId);
                                if (samplesRead > 0)
                                {
                                    bool isPureSilence = true;
                                    double d = 0;
                                    for (int x = 0; x < samplesRead; x++)
                                    {
                                        // NOTE: This is a hackey piece of logic.  The Engine will keep the speaker going for a little while
                                        // even after all audio RX has completed.  This is to allow for jitter buffer drainage as well as keep 
                                        // the speaker "warm" in case a new transmission comes in within a second or two.  A better strategy
                                        // is to track whether the group still has RX going and, when it stops, prevent the code below from 
                                        // processing potentially silent audio.

                                        d += gd.speakerAudioBuffer[x];
                                        if(gd.speakerAudioBuffer[x] != 0)
                                        {                                            
                                            isPureSilence = false;
                                        }
                                    }

                                    if (!isPureSilence)
                                    {
                                        Console.WriteLine("adadSpeakerThread(" + gd.id + "): samplesRead=" + samplesRead + ", avg=" + (d / 80.0));

                                        // TODO: do something with the speaker audio ...
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        private void adadMicrophoneThread()
        {
            Random rnd = new Random();

            while (_runAdadMicrophoneThread)
            {
                Thread.Sleep(10);

                if (!_runAdadMicrophoneThread)
                {
                    break;
                }

                foreach (GroupDescriptor gd in _groups)
                {
                    if (gd.microphoneAdad.engageDeviceId != 0)
                    {
                        if (gd.microphoneAdad.isRunning)
                        {
                            if (gd.adadAudioTransferProxy != null)
                            {
                                // NOTE: The real audio for the ADAD needs to be in gd.microphoneAudioBuffer

                                // For our demo purposes, we'll just create some random noise in our microphone buffer
                                for (int x = 0; x < Adad.SAMPLES_PER_FRAME; x++)
                                {
                                    gd.microphoneAudioBuffer[x] = (short)rnd.Next(-32000, 32000);
                                }

                                gd.adadAudioTransferProxy.writeMicrophoneAudio(gd.microphoneAudioBuffer, Adad.SAMPLES_PER_FRAME, gd.microphoneAdad.engageDeviceId, gd.microphoneAdad.instanceId);
                            }
                        }
                    }
                }
            }
        }

        private void determineIfAdadThreadsAreNeeded()
        {
            determineIfAdadSpeakerThread();
            determineIfAdadMicrophoneThread();
        }

        private void determineIfAdadSpeakerThread()
        {
            bool isNeeded = false;

            foreach (GroupDescriptor gd in _groups)
            {
                if (gd.speakerAdad != null && gd.speakerAdad.isRunning)
                {
                    isNeeded = true;
                    break;
                }
            }

            if (isNeeded)
            {
                if (_adadSpeakerThreadHandle == null)
                {
                    Console.WriteLine("one or more speaker adads have gone active - starting thread");
                    _runAdadSpeakerThread = true;
                    _adadSpeakerThreadHandle = new Thread(new ThreadStart(adadSpeakerThread));
                    _adadSpeakerThreadHandle.Start();
                }
            }
            else
            {
                if (_adadSpeakerThreadHandle != null)
                {
                    Console.WriteLine("all speaker adads have gone inactive - stopping thread");
                    _runAdadSpeakerThread = false;
                    _adadSpeakerThreadHandle.Join();
                    _adadSpeakerThreadHandle = null;
                }
            }
        }

        private void determineIfAdadMicrophoneThread()
        {
            bool isNeeded = false;

            foreach (GroupDescriptor gd in _groups)
            {
                if (gd.microphoneAdad != null && gd.microphoneAdad.isRunning)
                {
                    isNeeded = true;
                    break;
                }
            }

            if (isNeeded)
            {
                if (_adadMicrophoneThreadHandle == null)
                {
                    Console.WriteLine("one or more microphone adads have gone active - starting thread");
                    _runAdadMicrophoneThread = true;
                    _adadMicrophoneThreadHandle = new Thread(new ThreadStart(adadMicrophoneThread));
                    _adadMicrophoneThreadHandle.Start();
                }
            }
            else
            {
                if (_adadMicrophoneThreadHandle != null)
                {
                    Console.WriteLine("all microphone adads have gone inactive - stopping thread");
                    _runAdadMicrophoneThread = false;
                    _adadMicrophoneThreadHandle.Join();
                    _adadMicrophoneThreadHandle = null;
                }
            }
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
            int sizeToDisplay = rawSize;
            if (sizeToDisplay > 64)
            {
                sizeToDisplay = 64;
            }

            string hexString = hexStringOf(raw, sizeToDisplay, ":");

            Console.WriteLine("C#: onGroupRawReceived: " + id + ", displaying " + sizeToDisplay + " of " + rawSize + " bytes\n[" + hexString + "]");
        }

        void Engage.IGroupNotifications.onGroupReconfigured(string id, string eventExtraJson)
        {  
            Console.WriteLine("C#: onGroupReconfigured: " + id);          
        }

        void Engage.IGroupNotifications.onGroupReconfigurationFailed(string id, string eventExtraJson)
        {  
            Console.WriteLine("C#: onGroupReconfigurationFailed: " + id);          
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

        public void onGroupRxVolumeChanged(string id, int leftLevelPerc, int rightLevelPerc, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupRxVolumeChanged: " + id);
        }

        public void onGroupRxDtmf(string id, string dtmfJson, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupRxDtmf: " + id);
        }

        public void onEngageLogMessage(int level, string tag, string message)
        {
            Console.WriteLine("C#: onEngageLogMessage: " + level + ", " + tag + ", " + message);
        }

        void Engage.IEngineNotifications.onEngineAudioDevicesRefreshed(string eventExtraJson)
        {
            Console.WriteLine("C#: onEngineAudioDevicesRefreshed: " + eventExtraJson);
        }

        public void onGroupAudioRecordingStarted(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupAudioRecordingStarted: " + id + ", " + eventExtraJson);
        }

        public void onGroupAudioRecordingFailed(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupAudioRecordingFailed: " + id + ", " + eventExtraJson);
        }

        public void onGroupAudioRecordingEnded(string id, string eventExtraJson)
        {
            Console.WriteLine("C#: onGroupAudioRecordingEnded: " + id + ", " + eventExtraJson);
        }

        public int onAudioDeviceCreateInstance(int deviceId)
        {
            Console.WriteLine("C#: onAudioDeviceCreateInstance: " + deviceId);

            Adad adad = getAdadForEngageDeviceId(deviceId);
            if(adad != null)
            {
                return adad.instanceId;
            }

            return Engage.ENGAGE_AUDIO_DEVICE_INVALID_DEVICE_ID;
        }

        public int onAudioDeviceDestroyInstance(int deviceId, int instanceId)
        {
            Console.WriteLine("C#: onAudioDeviceDestroyInstance: " + deviceId + ", " + instanceId);

            Adad adad = getAdadForEngageDeviceId(deviceId);
            if (adad != null)
            {
                return Engage.ENGAGE_AUDIO_DEVICE_RESULT_OK;
            }

            return Engage.ENGAGE_AUDIO_DEVICE_INVALID_OPERATION;
        }

        public int onAudioDeviceStartInstance(int deviceId, int instanceId)
        {
            Console.WriteLine("C#: onAudioDeviceStartInstance: " + deviceId + ", " + instanceId);

            Adad adad = getAdadForEngageDeviceId(deviceId);
            if (adad != null)
            {
                adad.isRunning = true;
                determineIfAdadThreadsAreNeeded();
                return Engage.ENGAGE_AUDIO_DEVICE_RESULT_OK;
            }

            return Engage.ENGAGE_AUDIO_DEVICE_INVALID_OPERATION;
        }

        public int onAudioDeviceStopInstance(int deviceId, int instanceId)
        {
            Console.WriteLine("C#: onAudioDeviceStopInstance: " + deviceId + ", " + instanceId);

            Adad adad = getAdadForEngageDeviceId(deviceId);
            if (adad != null)
            {
                adad.isRunning = false;
                determineIfAdadThreadsAreNeeded();
                return Engage.ENGAGE_AUDIO_DEVICE_RESULT_OK;
            }

            return Engage.ENGAGE_AUDIO_DEVICE_INVALID_OPERATION;
        }

        public int onAudioDevicePauseInstance(int deviceId, int instanceId)
        {
            Console.WriteLine("C#: onAudioDevicePauseInstance: " + deviceId + ", " + instanceId);

            Adad adad = getAdadForEngageDeviceId(deviceId);
            if (adad != null)
            {
                adad.isRunning = false;
                determineIfAdadThreadsAreNeeded();
                return Engage.ENGAGE_AUDIO_DEVICE_RESULT_OK;
            }

            return Engage.ENGAGE_AUDIO_DEVICE_INVALID_OPERATION;
        }

        public int onAudioDeviceResumeInstance(int deviceId, int instanceId)
        {
            Console.WriteLine("C#: onAudioDeviceResumeInstance: " + deviceId + ", " + instanceId);

            Adad adad = getAdadForEngageDeviceId(deviceId);
            if (adad != null)
            {
                adad.isRunning = true;
                determineIfAdadThreadsAreNeeded();
                return Engage.ENGAGE_AUDIO_DEVICE_RESULT_OK;
            }

            return Engage.ENGAGE_AUDIO_DEVICE_INVALID_OPERATION;
        }

        public int onAudioDeviceRestartInstance(int deviceId, int instanceId)
        {
            Console.WriteLine("C#: onAudioDeviceRestartInstance: " + deviceId + ", " + instanceId);

            Adad adad = getAdadForEngageDeviceId(deviceId);
            if (adad != null)
            {
                adad.isRunning = false;
                determineIfAdadThreadsAreNeeded();
                adad.isRunning = true;
                determineIfAdadThreadsAreNeeded();
                return Engage.ENGAGE_AUDIO_DEVICE_RESULT_OK;
            }

            return Engage.ENGAGE_AUDIO_DEVICE_INVALID_OPERATION;
        }

        public int onAudioDeviceResetInstance(int deviceId, int instanceId)
        {
            Console.WriteLine("C#: onAudioDeviceResetInstance: " + deviceId + ", " + instanceId);

            Adad adad = getAdadForEngageDeviceId(deviceId);
            if (adad != null)
            {
                adad.isRunning = false;
                determineIfAdadThreadsAreNeeded();
                adad.isRunning = true;
                determineIfAdadThreadsAreNeeded();
                return Engage.ENGAGE_AUDIO_DEVICE_RESULT_OK;
            }

            return Engage.ENGAGE_AUDIO_DEVICE_INVALID_OPERATION;
        }
        #endregion
    }
}
