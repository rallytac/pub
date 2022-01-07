# Certificates

___
# IMPORTANT
The X.509 certificates and related keys provided here are the factory defaults meant for development and testing purposes.  They are *not* intended for production use.

Rally Tactical Systems, Inc. makes no representation of suitability to any purpose of these documents other than for internal development and testing.
___

This directory contains RTS factory certificates and keys used for development and testing.  You will find references to them throughout our document, inside source code examples, and part of baselione installation packages where the expectation is that you will replace the factory defaults with your own certificates.

## Files 
* **rtsCA.pem** is the X.509 certificate for our own Certificate Authority (CA).  This is a CA we created inside Rally Tactical in order to create our own certificates.  We are providing this certificate here so that you may validate factory default certificates issued by RTS.  Note that you cannot use this CA certificate to create your own certificates.  If you'd like to do that, see below.

* ***rtsFactoryDefaultEngage.pem*** is the factory default certificate used by Engage Engines.  This is an Elliptic Curve (EC) certificate.

* ***rtsFactoryDefaultEngage.key*** is the private key for *rtsFactoryDefaultEngage.pem*.

* ***rtsFactoryDefaultRpSrv.pem*** is the factory default certificate used by Engage's Rallypoints.  This is an Elliptic Curve (EC) certificate.

* ***rtsFactoryDefaultRpSrv.key*** is the private key for *rtsFactoryDefaultRpSrv.pem*.

* ***all-rts-certs.certstore*** contains all the above certificates.

* ***engagebridged.certstore*** contains the above certificates used by `engagebridged`.

* ***rallypointd.certstore*** contains the above certificates used by `rallypointd`.

### Android Files
We use Android Studio for development of apps for Android, including the sample applications in this repository.  Now those sample applications need certificates as well - and those files have to be in a particular directory structure layout and need to be named in a particular fashion as per requirements for Android Studio.  So, you'll see a directory named *android* and, within that *raw*.  This is the direcrory where Android Studio points to for the certificate files.

Then, in that *raw* directory, are the files we use.  They are simply copies of *rtsCA.pem*, *rtsFactoryDefaultEngage.pem*, and *rtsFactoryDefaultEngage.key*.  But they have been renamed to meet Android Studio's requirements.  Henceforth, in that directory:

* **rtsCA.pem** == **android_rts_ca_certificate.pem**
* **rtsFactoryDefaultEngage.pem** == **android_rts_factory_default_engage_certificate.pem**
* **rtsFactoryDefaultEngage.key** == **android_rts_factory_default_engage_private_key.key**

## Obtaining commercially-issued certificates
Your best bet for production purposes is to use certificates issued by a trusted third-party such as a commercial Certificate Authority.  (A benefit to this is that the CA's certificate will most likely be installed as part of your operating system anyway.)  

Failing that, though, you can issue your own certificates - either by self-signing them (not advised!) or by becoming a CA yourself, creating certificates using your CA certificate, and installing your CA certificates on machines that will be validating your certificates.  We're going to cover that procedure in the next section.

## Generating your own certificates (the easy way)
In the next section we'll explain how to create your own certificates the hard way.  But there's an easier way - use the `mkcert.sh` script in this directory.

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
0 drwxr-xr-x   7 sbotha  wheel   224 Jan  7 13:18 .
0 drwxr-xr-x  17 sbotha  wheel   544 Jan  7 13:12 ..
8 -rw-r--r--   1 sbotha  wheel  1395 Jan  7 13:17 MyCA.cert
8 -rw-r--r--   1 sbotha  wheel  1675 Jan  7 13:16 MyCA.key
8 -rw-r--r--   1 sbotha  wheel  1411 Jan  7 13:18 MyCoolCertificate.cert
8 -rw-r--r--   1 sbotha  wheel  1675 Jan  7 13:17 MyCoolCertificate.key
```

That;'s it!  All done.

If you need to make another certificate signed by `MyCA` - let's call it `YetAnotherCertificate`; your command-line would look as follows:
```shell
./mkcert.sh MyCA YetAnotherCertificate
```

This time, though, you won't need to create `MyCA` as it'll already be present.

>While it really shouldn't be necessary to say this, we will anyway ... the answers in the examples above to the prompts for things `Country Name`, `State or Province Name`, `Locality`, etc, etc are examples only.  Enter values that are specific to your organization.  Yes, it's a "duh!" statement to make; but we're just being sure :)

## Generating your own certificates (the more difficult way)
Creating your own certificates is actually pretty straightforward once you get past all the scary-looking command-line options offered by tools such as OpenSSL (which we're going to use here). OpenSSL is likely already installed on your machine but, if not, grab it from https://www.openssl.org.

There are two main steps to this.  (A) Create your CA certificate and, (B) Use your CA certificate to create subsequent certificates.

### A. Create your CA certificate
Let's create the certificate which is going to be used to issue subsequent certificates.  It will also be used to verify those certificates by entities in your organization.  This is your Certificate Authority Certicate - or "CA Certificate".

#### Step 1 : Generate a private key
We're going to use the *secp521r1* elliptic curve.  This will produce a file named *myCA.key*.  (You can use any file name you'd like.)

```
# openssl ecparam -name secp521r1 -genkey -out myCA.key
```

The contents of the file will look something like this - an unencrypted/unprotected private key.
```
-----BEGIN EC PARAMETERS-----
BgUrgQQAIw==
-----END EC PARAMETERS-----
-----BEGIN EC PRIVATE KEY-----
MIHcAgEBBEIBfNKYn1aQZVOlH+/mJHuG8vyWpnlE+FoNzaLDnjVxWgaKzBkprynq
K0zAsWaClqmXLQlx9oJRyn9dN6Ol/Js2r2CgBwYFK4EEACOhgYkDgYYABABpE6gh
C41k8E8UsngDEP40sedsr3TvTVNR6zKSfO0bq0jo6fdnaBicbRPqieqhj24g7oc4
3KsVGYCLZ2ARyn5tfADBl9R0HN6x8BJVBovnARKj+imPOy9tleEJQ4PfV+zb1W+x
iX30wv8n1HA0hnLgEwdaJgRC4sDbcKSjVderlL38bQ==
-----END EC PRIVATE KEY-----
```

#### Step 2: Encrypt the key
Next, encrypt this key using AES256.  You will be asked for a passphrase/password that will protect your key.  Don't forget this password or you won't be able to recover your private key.
```
# openssl ec -in myCA.key -out myCA.key -aes256
```

Your key file is now protected with your password and will look something like this:
```
-----BEGIN EC PRIVATE KEY-----
Proc-Type: 4,ENCRYPTED
DEK-Info: AES-256-CBC,FA4261614DC5EFC555324BAA3864D405

FOS0j7wxYKzdoujnMYqATkixqf3hXmOYr7u5Vn4dfrat6xZbTP9WzECANxcgvne+
S9+eDRffn6zFSnvI5860SG6J1pViMRGXcdRyCC2Rwyc1ozFzS9hBsg2K5vS9meyx
IWre7lbMwSCWchkzljYcODJz/9sdtU8hEKNc7sDyW+ELg2l16leim1pDycOujHDt
3ouHpCC+fX46FIgziZPEWjg3eWcw0clbVULdKvbGsaRs5TO6n8pPixWVakPgWQPk
kNEECZ0we85FfIu3y0idadUAVyCXW/d5RzgyN5Dy5H4=
-----END EC PRIVATE KEY-----
```

**Make sure you keep this key file safe.  If it get's out into the wild, your CA certificate will become worthless.**

#### Step 3: Create your CA certificate
Now we'll use this key to create the CA certificate - which we'll store in *myCA.pem*.  Notice how in the example below, we specify a *-days* value of *3650*.  This means that the certificate will expire in 3650 days.  You can make this value whatever you'd like.

```
# openssl req -x509 -new -nodes -key myCA.key -sha256 -days 3650 -out myCA.pem
```

There's going to be quite a few pieces of information that OpenSSL will prompt you for during this - starting with the password/passphrase for the key file you created above.  (Hopefully you remembered it).  You will also be asked for information concerning the location of your organization (such as country, city, and other locality) as well as organization name and organizational unit name.  The unit name is often used to describe the use of the certificate - in this case, the fact that it is a CA certificate - and not an "ordinary" certificate.  Make sure you provide correct information for all this stuff.

When you're done, you'll have a certificate - *myCA.pem* in this case - which looks something like this:

```
-----BEGIN CERTIFICATE-----
MIIDKjCCAosCCQDA9yyGXvBI2TAKBggqhkjOPQQDAjCB2DELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCldhc2hpbmd0b24xEDAOBgNVBAcMB1NlYXR0bGUxJTAjBgNVBAoM
HFJhbGx5IFRhY3RpY2FsIFN5c3RlbXMsIEluYy4xSDBGBgNVBAsMPyhjKSAyMDE5
IFJhbGx5IFRhY3RpY2FsIFN5c3RlbXMsIEluYy4gLSBGb3IgYXV0aG9yaXplZCB1
c2Ugb25seTEMMAoGA1UEAwwDYWFhMSMwIQYJKoZIhvcNAQkBFhRzdXBwb3J0QHJh
bGx5dGFjLmNvbTAeFw0xOTA5MzAxNzM2NDVaFw0yOTA5MjcxNzM2NDVaMIHYMQsw
CQYDVQQGEwJVUzETMBEGA1UECAwKV2FzaGluZ3RvbjEQMA4GA1UEBwwHU2VhdHRs
ZTElMCMGA1UECgwcUmFsbHkgVGFjdGljYWwgU3lzdGVtcywgSW5jLjFIMEYGA1UE
Cww/KGMpIDIwMTkgUmFsbHkgVGFjdGljYWwgU3lzdGVtcywgSW5jLiAtIEZvciBh
dXRob3JpemVkIHVzZSBvbmx5MQwwCgYDVQQDDANhYWExIzAhBgkqhkiG9w0BCQEW
FHN1cHBvcnRAcmFsbHl0YWMuY29tMIGbMBAGByqGSM49AgEGBSuBBAAjA4GGAAQA
aROoIQuNZPBPFLJ4AxD+NLHnbK90701TUesyknztG6tI6On3Z2gYnG0T6onqoY9u
IO6HONyrFRmAi2dgEcp+bXwAwZfUdBzesfASVQaL5wESo/opjzsvbZXhCUOD31fs
29VvsYl99ML/J9RwNIZy4BMHWiYEQuLA23Cko1XXq5S9/G0wCgYIKoZIzj0EAwID
gYwAMIGIAkIArL2oepL/1uv4oKFkFluCUK2SE38OZgh9hj3CIWOnjOQoa9HuS/Mw
rnXOCjWZ83h5lq9XiGb4p1HthMBuvEbIQcoCQgGtNevhB1grSrbiPsqZfRs+diae
ndT2dU2mK1qe0+JjMYoDZaElidiAxC24oQMXmgYGYKm+ArYnRa1M0u+ClNvGvA==
-----END CERTIFICATE-----
```

Congratulations!  You are now a Certificate Authority!

### B. Create your "ordinary' certficates

Now that we have our CA Certificate, we'll use it to create the certificates and keys we'll use for our various Engage entities.  In the steps below, we're going to create a certificate named *myClientCert* (name it whatever you'd like of course) which consists of the public portion stored in *myClientCert.pem* and the private key stored in *myClientCert.key*.

Note that we're not going to encrypt the private key in these examples as these keys are generally going to be used inside applications which, when they need to decrypt the key, would need to know the encryption password.  That is, in of itself, a security risk, so we'll assume that the private key file is stored on end-user devices and server systems in secure areas that only the application itself and other authorized entities have access to.  If your security needs or software distribution mechanisms dictate otherwise, modify accordingly.

#### Step 1: Generate a key
We'll follow the same procedure here to generate a private key as we did for our CA certificate.

```
# openssl ecparam -name secp521r1 -genkey -out myClientCert.key
```

Your (unencryted) private key file will look something like this:
```
-----BEGIN EC PARAMETERS-----
BgUrgQQAIw==
-----END EC PARAMETERS-----
-----BEGIN EC PRIVATE KEY-----
MIHcAgEBBEIBg5DsT/lYSe5w13oq+Wi7dDEVuk6XnIvSqXX4jOrdpxnblhv0p3tI
Z5PozCSNWKdX0ZDtzQrLac243H/lVavxzNugBwYFK4EEACOhgYkDgYYABABHsDY+
NEEBRgw/1CxQ+yc3XEhmz/GrldbZu/ppmpxRc46Y5AIGa2rIAuBPopOaoJXNWyDG
CarNZ7oKN2pKrikx0QC2La5L7xN9JJ1FncVwa5zp5WkKjoMZXbp4JkqbgGse1rcy
SKNVHQ/KHc0baBlEpyVQHxG75VXJODG0ip3Zqyd3eQ==
-----END EC PRIVATE KEY-----
```

#### Step 2: Generate a CSR
A CSR is a "Certificate Signing Request" which is simply a simply file based on a private key that is handed to a Certificate Authority for purposes of creating a certificate. In addition to goodies from the private key, the CSR also contains information to be used in the creation of the certificate - such as locality, organization, etc.  Make sure you enter correct values for these prompts.

So, using the *myClientCert.key* created in Step 1:

```
# openssl req -new -key myClientCert.key -out myClientCert.csr
```

This will give you a CSR file something like this:
```
-----BEGIN CERTIFICATE REQUEST-----
MIICHTCCAX4CAQAwgdgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApXYXNoaW5ndG9u
MRAwDgYDVQQHDAdTZWF0dGxlMSUwIwYDVQQKDBxSYWxseSBUYWN0aWNhbCBTeXN0
ZW1zLCBJbmMuMUgwRgYDVQQLDD8oYykgMjAxOSBSYWxseSBUYWN0aWNhbCBTeXN0
ZW1zLCBJbmMuIC0gRm9yIGF1dGhvcml6ZWQgdXNlIG9ubHkxDDAKBgNVBAMMA3h4
eDEjMCEGCSqGSIb3DQEJARYUc3VwcG9ydEByYWxseXRhYy5jb20wgZswEAYHKoZI
zj0CAQYFK4EEACMDgYYABABHsDY+NEEBRgw/1CxQ+yc3XEhmz/GrldbZu/ppmpxR
c46Y5AIGa2rIAuBPopOaoJXNWyDGCarNZ7oKN2pKrikx0QC2La5L7xN9JJ1FncVw
a5zp5WkKjoMZXbp4JkqbgGse1rcySKNVHQ/KHc0baBlEpyVQHxG75VXJODG0ip3Z
qyd3eaAAMAoGCCqGSM49BAMCA4GMADCBiAJCAInLo1tLp/AhQZTSVQiTXjeQVjr/
HOHrGUEoDOHiBCN8hKwUfF7prix6yybE+emC5rGkuIBFNmShymlmgOR3wThNAkIA
jAuFgsSJXYXd+tKlHhPt7xRWrkNf4/k1JajvatecnCjlUq5YkFhDhjWeWpwlHRE0
Ob+l9w7dfs6jjxQF4D6sIqU=
-----END CERTIFICATE REQUEST-----
```

#### Step 3: Create the certificate from the CSR
We're almost there.  The last thing we have to do is to actually create the certificate.  Normally this is something a commercial certificate authority would do using its CA Certificate and associated private key.  But we're our own CA, so we use our CA Certificate goodies we created earlier to create (actually "issue" is the correct term) the certificate.

Here's the command:
```
# openssl x509 -req -in myClientCert.csr -CA myCA.pem -CAkey myCA.key -CAcreateserial -out myClientCert.pem -days 3650 -sha256
```

As this command will be using the private key for our CA Certificate (which we encrypted - remember?), you will have to enter the password for that CA private key.

If all goes well, you'll end up with a certificate file name *myClientCert.pem* which is valid for 3650 days (or whatever time period you choose).  Something like this:

```
-----BEGIN CERTIFICATE-----
MIIDKDCCAosCCQDXOpTfl+DxJTAKBggqhkjOPQQDAjCB2DELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCldhc2hpbmd0b24xEDAOBgNVBAcMB1NlYXR0bGUxJTAjBgNVBAoM
HFJhbGx5IFRhY3RpY2FsIFN5c3RlbXMsIEluYy4xSDBGBgNVBAsMPyhjKSAyMDE5
IFJhbGx5IFRhY3RpY2FsIFN5c3RlbXMsIEluYy4gLSBGb3IgYXV0aG9yaXplZCB1
c2Ugb25seTEMMAoGA1UEAwwDYWFhMSMwIQYJKoZIhvcNAQkBFhRzdXBwb3J0QHJh
bGx5dGFjLmNvbTAeFw0xOTA5MzAxNzU3MTVaFw0yOTA5MjcxNzU3MTVaMIHYMQsw
CQYDVQQGEwJVUzETMBEGA1UECAwKV2FzaGluZ3RvbjEQMA4GA1UEBwwHU2VhdHRs
ZTElMCMGA1UECgwcUmFsbHkgVGFjdGljYWwgU3lzdGVtcywgSW5jLjFIMEYGA1UE
Cww/KGMpIDIwMTkgUmFsbHkgVGFjdGljYWwgU3lzdGVtcywgSW5jLiAtIEZvciBh
dXRob3JpemVkIHVzZSBvbmx5MQwwCgYDVQQDDAN4eHgxIzAhBgkqhkiG9w0BCQEW
FHN1cHBvcnRAcmFsbHl0YWMuY29tMIGbMBAGByqGSM49AgEGBSuBBAAjA4GGAAQA
R7A2PjRBAUYMP9QsUPsnN1xIZs/xq5XW2bv6aZqcUXOOmOQCBmtqyALgT6KTmqCV
zVsgxgmqzWe6CjdqSq4pMdEAti2uS+8TfSSdRZ3FcGuc6eVpCo6DGV26eCZKm4Br
Hta3MkijVR0Pyh3NG2gZRKclUB8Ru+VVyTgxtIqd2asnd3kwCgYIKoZIzj0EAwID
gYoAMIGGAkFSXGmlWe6TW027Z4bX6DDzgzyfSL88aA/CLf7zrOojO3KXKdaG46M+
Z9tiF1gsv7Fz7AoBgEN+9P6fZpzRZx5ufgJBMTbEdezkl93blpcL1KMCQ7kGDZZU
4WOvoi7GjfnUYDrslgDtiXxB1TuB+6ZxnXeQ/O5dDLFPDSujkhAhAsni0Sk=
-----END CERTIFICATE-----
```

There you have it!  You can now create certificates using your CA cert to your heart's content by repeating steps 1 through 3 in this section.

# Finally
**Remember that, because your certificates have been issued by *your* CA, that CA certificate (just the PEM file, not the key) needs to be provided to entities validating the certificates you create.**