# EngageAndroid

This is a reference application for Android using the Engage Engine encapsulated in an AAR file.  The Engine is written in C++ using the Android NDK and is wrapped with a Java interface/shim that makes using the API calls in the Engine a little easier.  That same interface also provides a more Java-like experience for callback events, Java enumerations, Java data types, and so.

Inside the AAR are binaries for all the ARM and Intel target ABIs supported by Android (32- and 64-bit).  The Java wrapper exposes a Java class named "Engine" through which you call into the underlying NDK library.  If you're interested in putting together your own AAR perhaps only for some of the ABIs so as to reduce APK size, or if you simply want to see or modify the source code for the Java wrapper, go to the "bin" directory in this repository.

## Obtaining the AAR
Ideally we'd like to distribute the AAR through well-known and well-supported (and financially viable!) repositories such as Maven, Bintray, and so on.  But, we've so far not found something that suits all of our needs or those of our partners.  So, we've opted to provide the AAR in basic downloadable form from a public URL that can then be access from your command-line, build script, or internal artifact distribution mechanism.

The general URL format for the AAR is:  `http://artifacts.rallytac.com/artifacts/builds/<VERSION>/android/engage-engine-release-<VERSION>.aar` where `<VERSION>` is the version number of Engage.  For example, to download the AAR for version `1.210.9048`, your download URL would be: `http://artifacts.rallytac.com/artifacts/builds/1.210.9048/android/engage-engine-release-1.210.9048.aar`.


## Flavors
This reference app support multiple "flavors" or "build variants".  The default variant is named "reference" and is included in the gradle file.  You can define additional variants of your own if you'd like - each with their own business logic, behavior, and UI.  To do so, simply define the gradle Groovy language elements in a file in the "flavors" directory and have at it.  Obviously you should be familiar with how build variants work, how resources and manifests are setup and merged, and so on.

Take note that on a flavor-by-flavor basis, you will need to provide some code specific to that flavor.  For example, the reference code calls a Java method named `FlavorSpecific.applyGeneratedMissionModifications`.  This is in `EngageAndroid/app/src/reference/java/com/rallytac/engageandroid/FlavorSpecific.java` for the reference flavor.  

```java
package com.rallytac.engageandroid;

public class FlavorSpecific
{
    public static String applyGeneratedMissionModifications(String json, boolean isSampleMission)
    {
        // Nothing to be done here for the reference app
        return json;
    }
}
```

Now, let's say that your `flavors.gradle` file looks as follows:

```groovy
android {
    flavorDimensions "version"

    productFlavors {
        myspecialflavor {
            dimension "version"
            applicationId "com.special.flavor.something.whatever"
        }
   }
}
```

You're going to need this class in a path valid for the flavor.  In this case it's going to be `EngageAndroid/app/src/reference/java/com/special/flavor/something/whatever/FlavorSpecific.java`.

