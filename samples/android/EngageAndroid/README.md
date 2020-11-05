# EngageAndroid

This is a reference application for Android using the Engage Engine encapsulated in an AAR file.  The Engine is written in C++ using the Android NDK and is wrapped with a Java interface/shim that makes using the API calls in the Engine a little easier.  That same interface also provides a more Java-like experience for callback events, Java enumerations, Java data types, and so.

Inside the AAR are binaries for all the ARM and Intel target ABIs supported by Android (32- and 64-bit).  The Java wrapper exposes a Java class named "Engine" through which you call into the underlying NDK library.  If you're interested is putting together your own AAR perhaps only for some of the ABIs so as to reduce APK size, or if you simply want to to see or modify the source code for the Java wrapper, go to the "bin/xxx.xxx.xxx/android" directory in this repository.

This reference app support multiple "flavors" or "build variants".  The default variant is named "generic" and is included in the gradle file.  You can define additional variants of your own if you'd like - each with their own business logic, behavior, and UI.  To do so, simply define the gradle Groovy language elements in a file in the "flavors" directory and have at it.  Obviously you should be familiar with how build variants work, how resources and manifests, are setup and merged, and so on.

Also, and most importantly, the AAR is NOT included in this repo - even though the project looks for the AAR in the "libs" directory.  We've purposefully excluded the binary from this subdirectory because the Engage binaries already reside in the "bin/xxx.xxx.xxx/android" directory.  So, to do your build, copy the AAR version you want into "libs" and then modify the gradle file accordingly to match the version number.  After that you should be able to build just fine

