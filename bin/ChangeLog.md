# Change Log

## October 3, 2024 - 1.249.9089
- Corrects an urgent issue on Linux platforms when using the ALSA audio input in stereo mode.

## October 2, 2024 - 1.248.9088
- Adds stricter compliance with FIPS 140-2 guidelines with regards to CA processing and advertised ciphers.
- Adds experimental active and passive IGMP snooping to Rallypoints to optimize inter-RP traffic.
- Corrects an issue on Linux platforms whereby thread names were not included in log output.
- Corrects an abortive race condition in internal timers when processes are shutting down.

## August 30, 2024 - 1.246.9086

>PLEASE NOTE: All updates from 1.238.9078 onward to date have been rolled up in this single release.

- Adds support for multi-element X.509 PEM files, certificate bundles, and intermediate issuer certificates.
- Adds support for G.729 Annex A audio encoding/decoding.
- Adds support for IPv6.
- Adds UDP streaming between Engage Engines & Rallypoints.
- Adds support for the ChaCha20 encryption algorithm.
- Adds checks for Cisco LMR gateway presence packets.
- Adds dynamic alias generation based on SSRC for anonymous RTP streams.
- Adds checking for valid crypto passwords.
- Adds standard Docker containers for AMD64, ARM64, and ARM32 architectures.
- Adds display of certificate descriptor JSON in ecstool.
- Adds engageGenerateSignature() and engageVerifySignature() API calls.
- Adds the ability to explicitly disable audio transmission on a group.
- Adds naming of the Linux ALSA audio device from environment variables.
- Adds forcing of a Rallypoint peer to leaf status at an upstream Rallypoint.
- Adds display of unknown RTP payload type in error logging.
- Adds a payload type of "mime" for group blob packets.
- Adds support for sovereign Azure clouds for Speech and Translation services.
- Adds transcription metadata for transcribed audio streams.
- Adds accompanying metadata for translation target audio streams.
- Adds reporting of Azure Cognitive Services usage in CSV format for Engage Linguistics Service.
- Adds transcription-only sessions to Engage Linguistics Service.
- Adds customizable execution timeouts to the Azure Cognitive Services ELS proxy.
- Adds the ability to disable probing and/or use of the local machine's audio hardware.
- Adds memory tuning settings for object caching.
- Adds initial support for producing a status report from the Engine - similar to that of RP, EBS, EAR, and ELS.
- Re-enables Rallypoint mesh loop detection.
- Improves performance and reliability of the Android OS-level audio layer.
- Improves performance and resource utilization for internal object caching.
- Improves database and timeline I/O performance.
- Improves performance of discarding of too-old audio jitter buffer elements.
- Reduces system-wide audio processing memory utilization by almost 80%.
- Replaces OpenSSL 1.1.1f with OpenSSL 3.0.8 and it's support for FIPS 140-2.
- Removes dependency on the wolfSSL FIPS 140-2 cryptography engine.
- Increases the logger's queue size to accommodate heavy load situations losing messages.
- Corrects a state-related problem with multiple talkers on a group.
- Corrects a Linux ALSA problem related to automatic switch to the default device.
- Corrects a problem with Rallypoints not reliably setting priority or TTL for transmitted packets.
- Corrects a problem with random failures to verify signed requests to a Rallypoint.
- Corrects a problem with the Rallypoint's link graph for host names containing spaces.
- Corrects an issue in ELS that randomly caused the process to not properly initialize.
- Corrects a race-condition bug in timelines where the target directory is not created prior to saving of event files.
- Corrects a few issues with Windows setup packages.
- Corrects an issue with semaphore naming conventions under Windows.
- Corrects random Rallypoint connection race conditions in the WASM build.

## July 29, 2023 - 1.238.9078
- Adds group-level firewalling capability in Rallypoints based on X.509 certificates.
- Adds Docker containers for RP, EBS, EAR, and ELS published to Docker Hub.
- Enhances logging for inbound RTP packet processing. 
- Enhances logging in the Android thunking layer.
- Updates NPM-publishing information and versioning.
- Corrects a featureset-related licensing issue.
- Corrects multi-threading issues with Android audio capture and playout.
- Corrects a directory processing bug in the timeline manager.
- Corrects an XCode-related compilation issue for iOS.

## May 30, 2023 - 1.236.9076
- Adds support for Web Assembly (WASM), allowing Engage to run at near native speed inside modern web browsers.
- Adds MELPe support under iOS.
- Adds support for "_attached" element in Group configuration objects.
- Adds the ability to capture RX/TX packets in WireShark PCAP format directly inside the Engine.
- Adds developer-level optional debugging of low-level packet details.
- Adds simulated RX impairment on a group-by-group basis.
- Adds Web Socket support to Rallypoints.
- Improves performance of low-level Engage-managed object pools.
- Improves support for deployment of Engage-based software inside Docker containers.
- Corrects an issue with FIP140-2 support on some OS platforms.
- Corrects an issue with Alias Specializers in RTP headers.
- Corrects an issue regarding invalid entitlement keys.
- Corrects an error related to end-of-rx detection.
- Corrects an issue related to silence appending when streaming to Microft Azure Cognitive Services.
- Corrects an issue related to the creation of temporary files on some OS platforms.

## January 19, 2023 - 1.234.9074
- Adds the new Engage Linguistics Service (ELS).
- Corrects an issue whereby groups not associated with an output audio device print debug messages every second after RX has concluded.

## October 14, 2022 - 1.232.9072
- Adds correct thread name for Sys:SignalListener.
- Adds correct thread name for Sys:ProcessMonitor.
- Adds correct thread name for PeriodicTimer.
- Adds Rallypoint peer roundtrip requests as a method to perform application-level keepalives.
- Adds Windows setup packages for RP, EBS, and EAR.
- Adds whitelisting/blacklisting of streams registered by RP peers.
- Adds engageGetDeviceId() to the non-C interfaces.
- Adds the product version number to the RP, EBS, and EAR status reports.
- Adds support for alias specializers and, through that, group cross-mute support.
- Adds missing thread names in logging.
- Adds optional production of the RP route map in JSON format.
- Adds the ability to ignore traffic from designated sources on a group-by-group basis.
- Adds an optional application-provided audio interface for Java-based platforms.
- Adds file protection safeguards to prevent simultaneous reading/writing of dynamic configuration files.
- Adds the engageSetCertStore() API call that allows for setting the certstore buffer directly instead of loading from a file.
- Adds support for linear PCM (i.e. uncompressed) as a RTP payload.
- Adds the getDeviceId API call to the Node.js interface layer.
- Changes the MELPe default block count from 3 to 1 to simplify interoperability with systems that do not support MELPe superframes.
- Standardizes default configuration location of "\ProgramData\Rally Tactical Systems" for RP, EBS, and EAR.
- Corrects an issue in Rallypoints related to an outbound mesh leaf not being properly recognized.
- Corrects a bug in ecstool that prevented operation on Windows.
- Corrects an issue with microphone handling on Android after an audio file is played to a group.
- Corrects a number of issues with the wolfSSL library on Windows for FIPS140-2 support.
- Corrects an issue with EBS when operating on codecs that are framed in blocks rather than milliseconds.
- Corrects an issue with RTP timestamps for Opus-encoded payloads.
- Corrects an issue when processing dynamic configuration files that change more than once in one second.
- Corrects an delay in notification of group RX ended for transmissions shorter than the jitter buffer hang time.

## June 6, 2022 - 1.228.9068
- Adds a security classification level setting to group definitions that allows implementors to determine communication types appropriate for a group.
- Adds "--version" command-line parameter to engage-cmd, rallypointd, engagebridged, and eard.
- Corrects a race-condition bug on shutdown that caused periodic memory corruption.
- Corrects exported symbol clashes with additional SQLite instances in the same process.
- Corrects a hung operation on engageStop.
- Corrects a bug causing random crashes when packets are sent on a group that has failed over to multicast.

## May 28, 2022 - 1.226.9066
- Adds connection timeout settings for Rallypoint connections to better handle high-latency transports.
- Adds preliminary detection of traffic loops on Rallypoint meshes.
- Adds support for 3rd-party FIPS 140-2 cryptographic engines to be loaded at runtime.
- Corrects a logging subsystem race-condition that caused some logging lines to be lost at shutdown.
- Corrects a bug that generated extraneous onEngineStopped events.

## April 20, 2022 - 1.224.9062
- Corrects an issue on Android where the Engine was hung during shutdown while receiving audio.

## April 5, 2022 - 1.222.9060
- Corrects an issue with EBS whereby the enablement of a bridge was being ignored.
- Adds a hang timer for smoothed audio transmission.
- Replaces the low-level Android audio interface with a more flexible and portable implementation that takes better advantage of built-in Android audio capabilities.

## January 26, 2022 - 1.218.9056
- Adds preliminary hardware echo cancellation on select Android devices and OS versions.
- Adds the ability for application-level provisioning of Android **audio** sessions.
- Adds an adaptation for obtaining machine identifiers on iOS devices.
- Adds Java method lookup caching to reduce CPU usage and battery drain.
- Adds the option for "Ranger Packets" sent after a no-TX hang time to "wake up" network pathways without sacrificing useful payloads.
- [EXPERIMENTAL-INTERNAL] Adds compression and decompression API calls that use the [Brotli](https://en.wikipedia.org/wiki/Brotli) algorithm.
- Corrects an issue with EAR whereby an invalid encoder value caused a failure to configure a group for listening.
- Corrects an issue with defaults for TLS & RP peer verifications and allowing of self-signed certificates.  
- Corrects an issue with HTTP/HTTPS downloads for Magellan Discovery & Interrogation.
- Corrects an issue with invalid receipt of IP multicast packets on different multicast addresses that share the same port.
- Restricts the TLS subsystem to a minimum TLS version of 1.2.
  
## October 7, 2021 - 1.216.9054
- Adds automatic restart of Android speaker and microphone devices when an error is encountered during audio playout or capture.
- Adds Android-specific audio settings for more fine-grained customization of the audio experience.
- Adds selected logging of API arguments to help with debugging.
- Adds logged warnings to the Rallypoint when system CPU utilization exceeds a configured threshold (default is 65%).
- Adds accommodation for new file descriptor tracking introduced with Android 11.
- Optimizes microphone open/close operations to reduce stress on a platform's audio subsystem.
- Optimizes Rallypoint logic to allow new client connections within 1.5 seconds of startup.
- Changes mission generation to require peer verification.
- Reclassifies some certificate logging messages from error to debug.
- Corrects a transmission-side issue that causes receiving entities to create multiple timeline events for a single transmission.
- Corrects a sporadic startup crash in Engage Activity Recorder and Engage Bridge.
- Corrects a race condition in the high-resolution timer on some CPU architectures.
  
## August 7, 2021 - 1.212.9050
- Adds per-group RTP profiles for network fine-tuning.
- Adds per-group discontinuous transmission settings as an option for supported CODECs.
- Adds (experimental) support for reverse subscription on leaf RP nodes for core-registered multicast reflectors.
- Adds settings for more fine-grained control over the internal watchdog logic.
- Adds timings for select internal tasks to better fine-tune platform-specific performance.
- Adds transmission smoothing for audio as the default to be more accomodating to naive 3rd-party jitter buffers.
- Adds optional per-reflector multicast interface names in the Rallypoint.
- Adds support for MIL-STD-3005/STANAG-4591 MELPe CODEC at 600, 1200, and 2400 bps.
- Adds engageGetActiveFeatureset() API that returns the currently active featureset (if any) - including active usage counts.
- Adds Java annotations in Engine.java to better cater for minification under newer Android build environments.
- Adds QoS options for Rallypoint TCP links.
- Adds serial number and fingerprint information to X.509 descriptor JSON.
- Adds Android-specific audio settings for sharing and performance modes, usage type, content type, and input preset.
- Enhances jitter calculation logic on RTP receivers.
- Enhances internal timers by switching from millisecond timing to nanosecond resolution.
- Enhances date validation for X.509 certificates.
- Corrects a random hanging issue when leaving a group on certain platforms.
- Corrects an invalid RX state in the jitter buffer under rare conditions.

## May 19, 2021 - 1.210.9048
- Adds (experimental) support for application-defined networking devices.
- Adds support for looped-back network packets into the same group for debugging and testing purposes.
- Corrects an issue with incorrect processing of Globally Unique Identifiers (GUIDs).
- Corrects a SEGV issue at shutdown of the Rallypoint on some Linux platforms.
- Corrects an issue related to static C++ global variable initialization on some platforms.
- Corrects a socket resource leak on meshed Rallypoints.
- Corrects an issue related to Windows binaries shipped without symbolic information.
- Corrects an issue with an invalid native method signature for engageLogMsg() in the Android Java module.
- Corrects an issue with an invalid return code in engage-cmd for the "getDeviceId" command-line option.

## May 1, 2021 - 1.209.9047
- Adds additional support for ultra-low bandwidth environments.
- Adds the ability to disable jitter buffer trimming for extreme jitter environments.
- Adds additional protection against over-queuing of inbound audio due to synchronization mismatches.
- Adds an ncurses-based status display to engage-cmd on supported platforms.
- Adds new functions in engage-cmd for Lua scripts.
- Improves Rallypoints' prioritization of media vs non-media traffic.
- Corrects an issue where audio from a previously registered transmitter periodically stops being delivered to the speaker.
- Corrects an issue related to the processing of invalid GUIDs.
- Corrects an issue related to unsynchronized RX started/ended events.
- Corrects a variety of edge-case memory leaks and corruption issues.

## April 8, 2021 - 1.208.9046
- Adds support for the [Speex](https://www.speex.org/) CODEC for interoperability with legacy systems.
- Adds experimental support for [Codec2](https://en.wikipedia.org/wiki/Codec_2) for ultra-low bandwidth environments (requires custom Engine build).
- Adds the engageRefreshAudioDevices() API to request the Engine to update it's internal audio device registry.
- Adds the engageReconfigureGroup() API to, at this time, change a group's audio input and/or output audio devices without restarting the group object.
- Adds experimental 'lbCrypto' group configuration option to reduce bandwidth utilization on encrypted groups.
- Adds payload processing in EBS such as audio payload transcoding and reframing, audio mixing, and alias insertions.
- Adds enhancements to minimize audio drop outs in ultra-high latency network environments (e.g. where one-way packet propagation delay is in excess of 10 seconds).
- Corrects a problem related to node discovery for the local node not occuring if presence groups were created *after* non-presence groups.

## March 12, 2021 - 1.206.9044
- Adds the ability to source input audio from a file rather than the microphone on a per-transmission basis.
- Adds automatic audio resamlping for audio resources loaded at runtime.
- Adds premliminary support for dynamically-loaded external CODECs.
- Adds more sophisticated support for RTP encoding/decoding mappings in the Engine policy.
- Adds premliminary support in engage-cmd for the Lua scripting language.
- Adds active-lambda tracking in the core task executor to aid in troubleshooting.

## March 3, 2021 - 1.204.9041
- Adds preliminary support for CUBIC/Vocality radio gateways.
- Adds additional transmit encoder parameter validation.
- Optimizes parsing of HTTP response headers.
- Optimizes X.509 certificate validation.
- Corrects a problem encountered when processing transmit parameters for UDP transports.
- Corrects an invalid return code when attempting to start an already-started Engine.
- Corrects an incorrecly named timeline event JSON field.
- Corrects an issue in parsing SSDP packets.

## February 26, 2021 - 1.202.9039
- Corrects a backward incompatibility issue introduced in 1.196.9033.
  
## February 25, 2021 - 1.200.9037 (SUPERCEDED BY 1.202.9039)
- Adds non-blocking DNS lookups for enhanced response times when switching link profiles.
- Adds the ability to disable Engine audio processing on headless systems.
- Reduces Rallypoint link establishment times to multi-homed RP hosts.
- Corrects an issue encountered during failback to Rallypoint links from temporary multicast links.
- Corrects an issue with the Linux installation package for the Engage Bridge Service.

## February 20, 2021 - 1.198.9035 (SUPERCEDED BY 1.202.9039)
- Adds app-level enablement of event notifications.
- Adds automatic silencing of low-priority audio streams in the presence of high-priority stream.
- Adds an optional Engine-assigned alias for anonymous RTP streams.
- Enhances timeline processing by moving database insertions & updates to a dedicated I/O thread.
- Optimizes the internal high-resolution timer to be less CPU-intensive.
- Optimizes the core task dispatcher with ~65% performance increase for queue insert/removal operations.
- Corrects an issue related to dropping of the audio contents of a stream's first packet due to audio resampling shortfalls.

## February 9, 2021 - 1.196.9033 (SUPERCEDED BY 1.202.9039)
- Adds optional transmission ID to RTP streams.
- Adds an application-defined hook to capture Engine log output.
- Adds the ability to configure the minimum presence descriptor transmit interval.
- Adds initial support for translation for transmission priority based on group address/port.
- Adds support for featureSets based on license key flags.
- Adds support for OpenSSL 1.1.1i.
- Changes device identification logic on Android to use the OS-provided machine identifier.
- Enhances support for Apple iOS platforms.
- Enhances the content of group membership information in presence descriptors.
- Corrects an issue with an uninitlized member variable in the licensing subsystem.
- Corrects a UDP socket resource leak on *nix platforms.
- Corrects a static library initialization issue on Windows platforms.

## November 29, 2020 - 1.192.9029
- Adds microphone input denoising on Android platforms.
- Adds Automatic Gain Control (AGC) for microphone input and speaker output.
- Adds support for defaulting of X.509 certificate elements from certificate stores.
- Adds support for X.509 certificate element tagging in certificate stores.
- Corrects an issue with transmit priority contention .
- Removes Engine-level process abort if policy security is incorrect.

## October 28, 2020 - 1.189.9026
- Adds rxFlags and txPriority to talker information notifications.
- Corrects an incorrect duration calculation for audio timeline events.

## October 17, 2020 - 1.186.9023
- Adds the ability to disable/enable the Engine watchdog.
- Adds support for iOS.
- Adds monitoring for licensing changes in policy file to engagebridged.
- Adds missing API calls for Java, C#, and Node interface layers.
- Improves performance of the timer subsystem.
- Improves TCP connectivity logic for Rallypoint connections.
- Improves TLS performance for Rallypoint connections.
- Corrects an issue with multicast failover.
- Corrects a bug related to RTP marker bit processing.
- Corrects a timeout issue related to Rallypoint clustering.
- Corrects an issue with overlapping calls to engageStart()/engageStop().
- Corrects an issue related to UDP multicast on UNIX(ish) platforms.
- Optimizations for the internal task executor.

## September 8, 2020 - 1.182.9019
- Corrects issues reported with audio RX.
- Corrects stereo panning issues.
- Corrects a bug in the RX-side jitter buffer.
- Optimizes audio processing to reduce CPU and battery utilization.

## August 1, 2020 - 1.178.9015
- Adds the initial release of the Engage Bridging Service.
- Adds symbolic information to binaries to aid in troubelshooting.

## July 22, 2020 - 1.176.9013
- Adds preliminary group bridging capabilities.
- Adds semaphore-based signalling of configuration changes for rallypointd.
- Corrects the Opus CODEC to prevent use of AVX instructions on older/crippled X86/X64 CPUs.
- Improves the RTP jitter buffer to further reduce latency.

## June 5, 2020 - 1.162.8938
- Adds Rallypoint clustering.
- Adds Acoustic Echo Cancellation.
- Fixes for Portugese-language code pages on Windows.

## May 12, 2020 - 1.153.8924
- Adds group restrictions to Rallypoints.
- Replaces multicast white and blacklists with multicast resrictions in Rallypoints.

## May 4, 2020 - 1.151.8922
- Adds notification of timeline grooming.
- Removes the option to disable the system database.

## May 3, 2020 - 1.150.8921
- Adds ability for the Engage Engine library to run side-by-side with the OS-provided distribution of OpenSSL.

## May 1, 2020 - 1.148.8914
- Adds timeline grooming based on occupied disk storage.

## April 28, 2020 - 1.147.8913
- Adds static multicast reflector configuration for Rallypoints.
- Adds the ability to import X.509 elements into a certificate store from another.
- Adds display of certificate information by Rallypoints.
- Adds publishing of additional headers to ease 3rd-party C++ development.
- Removes Rallypoint reciprocal subscription - replaced by static multicast reflectors.
- Corrects a namespace naming issue for ConfigurationObjects under certain GCC compiler versions.
- Corrects a syntax error in Rallypoint post-installation script.

## April 10, 2020 - 1.145.8908
- Adds information to TX event notifications.
- Improves transmit contention (glaring) for talkers with the same priority.
- Exposes the mission generation for the node.js binding.

## April 7, 2020 - 1.143.8906
- Adds sourcing of Rallypoint mesh configuration from an executed command.
- Adds command execution after updates to Rallypoint status file.
- Adds command execution after updates to Rallypoint links file.
- Adds thread name to Win32 logging output.
- Adds process termination logic in case of Rallypoint hung shutdown.

## April 2, 2020 - 1.142.8905
- Corrects a TLS packet buffer overflow on Rallypoint peer registration/deregistration messages.
- Adds the ability to throttle Rallypoint inbound connections to guard against Denial Of Service attacks.
- Adds the ability to throttle Rallypoint inbound connections based on system CPU utilization.
- Adds safeguards against Rallypoint congestion collapse due to CPU pressure.
- Adds additional Rallypoint and core Engine worker queue metrics.
- Adds Rallypoint process uptime to the status file.
- Adds Rallypoint system CPU usage to the status file.
- Adds support for importing certificates and keys from PKCS #12 archives.
- Optimizes logging in Rallypoint to increase performance.
- Offloads realtime timeline event ECDSA signing to a seperate thread so as not to impact core user experience.

## March 27, 2020 - 1.140.8903
- Corrects an Engine shutdown issue when running under node.js.
- Corrects an X.509 certificate exchange issue Rallypoint peer links under certain Linux distros.
- Adds the ability to disable multicast failover at the Engine policy level.

## March 22, 2020 - 1.133.8893
- Corrects an issue on some Windows network drivers related to multicast receive on a QoS-enabled socket.

## March 9, 2020 - 1.132.8892
- Adds multicast failover and failback of downed Rallypoint links as a standard feature.
- Adds JSON clarifier objects to all events fired by the Engine.

## March 5, 2020 - 1.126.8887
- Adds Engage-managed certificate stores
- Adds tx mute/unmute for groups - including the ability to begin tx in a muted state
- Improved handling of QoS settings for TX
- Addition of QoS and TTL for multicast reflectors in Rallypoints
- Corrects a bug related to the TTL value for multicast traffic

## February 18, 2020 - 1.121.8880
- Corrects audio issues related to the new multiplexing speaker logic
- Corrects scratchy G.711 audio
- Addresses FDX/HDX inconsistencies
- Adds whitelisted multicast groups on Rallypoints
- Adds Rallypoint reciprocal subscription (--beta--) 
- Adds Visual C/C++ runtimes to the npm module

## January 26, 2020 - 1.116.8874
- Corrects an issue with audio on certain versions of Android
- Corrects a random crash on all platforms during shutdown.  
- Corrects minor memory leaks
- Adds performance enhancements for networking

## December 17, 2019 - 1.109.8864
- Improved support for application-defined audio devices

## December 10, 2019 - 1.108.8863
- Corrects a number of minor bugs
- Adds ability to disable audio record but retain timeline metadata
- Improves performance in the audio resampler

## November 21, 2019 - 1.102.8857
- Corrects a buffer overflow in the audio resampler for G.711 framed larger than 40ms
- Corrects a jitter buffer miscalculation
- Corrects a problem with unknown talker(s) associated with audio

## November 7, 2019 - 1.95.8851
- Corrects blocked socket issues
- Corrects a crash on channel leave during RX
