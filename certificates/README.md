# Certificates

___
# IMPORTANT
The X.509 certificates and related keys provided here are the factory defaults meant for development and testing purposes.  They are *not* intended for production use.

Rally Tactical Systems, Inc. makes no representation of suitability to any purpose of these documents other than for internal development and testing.
___

This directory contains RTS factory certificates and keys used for development and testing.  You will find references to them throughout our document, inside source code examples, and part of baselione installation packages where the expectation is that you will replace the factory defaults with your own certificates.

## Files 
* **rtsCA.cert** is the X.509 certificate for our own Certificate Authority (CA).  This is a CA we created inside Rally Tactical in order to create our own certificates.  We are providing this certificate here so that you may validate factory default certificates issued by RTS.  Note that you cannot use this CA certificate to create your own certificates.  If you'd like to do that, see below.

* ***rtsFactoryDefaultEngage.cert*** is the factory default certificate used by Engage Engines.

* ***rtsFactoryDefaultEngage.key*** is the private key for *rtsFactoryDefaultEngage.cert*.

* ***rtsFactoryDefaultRpSrv.cert*** is the factory default certificate used by Engage's Rallypoints.

* ***rtsFactoryDefaultRpSrv.key*** is the private key for *rtsFactoryDefaultRpSrv.cert*.

* ***all-rts-certs.certstore*** contains all the above certificates.

### Android Files
We use Android Studio for development of apps for Android, including the sample applications in this repository.  Now those sample applications need certificates as well - and those files have to be in a particular directory structure layout and need to be named in a particular fashion as per requirements for Android Studio.  So, you'll see a directory named *android* and, within that *raw*.  This is the direcrory where Android Studio points to for the certificate files.

Then, in that *raw* directory, are the files we use.  They are simply copies of *rtsCA.cert*, *rtsFactoryDefaultEngage.cert*, and *rtsFactoryDefaultEngage.key*.  But they have been renamed to meet Android Studio's requirements.  Henceforth, in that directory:

* **rtsCA.cert** == **android_rts_ca_certificate.cert**
* **rtsFactoryDefaultEngage.cert** == **android_rts_factory_default_engage_certificate.cert**
* **rtsFactoryDefaultEngage.key** == **android_rts_factory_default_engage_private_key.key**

## Obtaining commercially-issued certificates
Your best bet for production purposes is to use certificates issued by a trusted third-party such as a commercial Certificate Authority.  (A benefit to this is that the CA's certificate will most likely be installed as part of your operating system anyway.)  

Failing that, though, you can issue your own certificates - either by self-signing them (not advised!) or by becoming a CA yourself, creating certificates using your CA certificate, and installing your CA certificates on machines that will be validating your certificates.  We're going to cover that procedure in the next section.

## Generating your own certificates
Creating certificates is not a particularly complicated task.  But it can be daunting if you haven't done it before (it certainly was for us!) - especially understanding the hairy command-line syntax needed for tools like OpenSSL.  But there's an easier way - use the `mkcert.sh` script in this directory.

> The `mkcert.sh` script works well for us and others.  But if your needs are different, feel free to make your own version, using `mkcert.sh` as a baseline.

Before we get going, understand that in order to create a certificate (and the private key that goes with it) you need a CA certificate.  So, `mkcert.sh` is going to need that CA certificate before it can do anything.

Now, `mkcert.sh` is smart enough to know that if you don't already have the CA cert you're telling it to use; it will create that CA cert for you.  Subsequent certificate creation operations will then use that cert.  Neat huh!

OK, let's get going...

Assume we want to create a certificate named `MyCoolCertificate`.  And we want to use a CA cert named `MyCA`.  Our command-line looks like this

```shell
./mkcert.sh MyCA MyCoolCertificate
```

Now, remember, we don't yet have `MyCA`, so `mkcert.sh` will tell you that it is creating a CA certificate named `MyCA.cert` and will prompt you for information for it.  Here's what that looks like:
```shell
-------------------------------------------------------------------------
CREATING CA CERTIFICATE MyCA.cert
-------------------------------------------------------------------------


Generating RSA private key, 2048 bit long modulus
..........................................................................................................+++
........+++
e is 65537 (0x10001)
You are about to be asked to enter information that will be incorporated
into your certificate request.
What you are about to enter is what is called a Distinguished Name or a DN.
There are quite a few fields but you can leave some blank
For some fields there will be a default value,
If you enter '.', the field will be left blank.
-----
Country Name (2 letter code) []:US
State or Province Name (full name) []:Washington
Locality Name (eg, city) []:Seattle
Organization Name (eg, company) []:My Company
Organizational Unit Name (eg, section) []:My Business Unit
Common Name (eg, fully qualified host name) []:My Company CA Certificate
Email Address []:support@mycompany.com
```

At this point the `MyCA` certificate has been created and `mkcert.sh` moves on to the next step of creating `MyCoolCertificate.cert` as follows:
```shell
-------------------------------------------------------------------------
CREATING CERTIFICATE MyCoolCertificate.cert
-------------------------------------------------------------------------


Generating RSA private key, 2048 bit long modulus
........+++
.....+++
e is 65537 (0x10001)
You are about to be asked to enter information that will be incorporated
into your certificate request.
What you are about to enter is what is called a Distinguished Name or a DN.
There are quite a few fields but you can leave some blank
For some fields there will be a default value,
If you enter '.', the field will be left blank.
-----
Country Name (2 letter code) []:US
State or Province Name (full name) []:Washington
Locality Name (eg, city) []:Seattle
Organization Name (eg, company) []:My Company
Organizational Unit Name (eg, section) []:My Business Unit
Common Name (eg, fully qualified host name) []:My Super Mega Ultra Cool Certificate
Email Address []:support@mycompany.com

Please enter the following 'extra' attributes
to be sent with your certificate request
A challenge password []:
Signature ok
subject=/C=US/ST=Washington/L=Seattle/O=My Company/OU=My Business Unit/CN=My Super Mega Ultra Cool Certificate/emailAddress=support@mycompany.com
Getting CA Private Key
```

Once this is all done, a directory listing looks as follows:
```shell
ls -lsa
total 40
0 drwxr-xr-x   7 somebody  wheel   224 Jan  7 13:18 .
0 drwxr-xr-x  17 somebody  wheel   544 Jan  7 13:12 ..
8 -rw-r--r--   1 somebody  wheel  1395 Jan  7 13:17 MyCA.cert
8 -rw-r--r--   1 somebody  wheel  1675 Jan  7 13:16 MyCA.key
8 -rw-r--r--   1 somebody  wheel  1411 Jan  7 13:18 MyCoolCertificate.cert
8 -rw-r--r--   1 somebody  wheel  1675 Jan  7 13:17 MyCoolCertificate.key
```

That's it!  All done.

If you need to make another certificate signed by `MyCA` - let's call it `YetAnotherCertificate`; your command-line would look as follows:
```shell
./mkcert.sh MyCA YetAnotherCertificate
```

This time, though, you won't need to create `MyCA` as it'll already be present.

>While it really shouldn't be necessary to say this, we will anyway ... the answers in the examples above to the prompts for things like `Country Name`, `State or Province Name`, `Locality`, etc, are **EXAMPLES ONLY**.  Enter values that are specific to your organization.  Yes, it's a "*duh*!" statement to make; but we're just being sure :)

>Oh, one more thing ... protect that private key (the `.key` file) with your life.  If that thing gets into the wild your security is down the drain.

# Finally
**Remember that, because your certificates have been issued by *your* CA, that CA certificate (just the PEM file, not the key) needs to be provided to entities validating the certificates you create.**