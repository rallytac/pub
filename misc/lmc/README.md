# Engage Linguistics Lingo File Meta Compiler (lmc)
This little script (`lmc.sh` or just simply `lmc`) reads standard input in the format of a *meta* files, producing a JSON file to be used with the Engage Linguistics Service.  The JSON that is output consists of a `voiceToVoiceSession` JSON object as well as a `groups` object - both needed by ELS.

The meta file has a simple YAML-like syntax as follows:

A line starting with `+` denotes a session identifier which must be a unique string.
e.g.
   +session1
   +myownsession
   +ABC-1-2-3

Lines starting with `-` denote an Engage group/channel within the preceding
session and are formatted as follows:

  <group_id>:<language_code>[:<crypto_password>]

  Both <group_id> and <language_code> are required.  The <crypo_password> value is only
  needed if the group is encrypted.  As with all Engage group identifiers, the ID must
  simply be a unique string, so use whatever group IDs your system is configured for.

## Using LMC
Because lmc reads its input meta file from standard input, you need to pipe in your file.  This can be done in a few ways:

```shell
cat lingo.meta | ./lmc.sh

...or

./lmc.sh < lingo.meta
```

That will give you JSON output to standard output.  So, if you want to save the output to a file - say `lingo.json`; you'd redirect is as follows:

```shell
cat lingo.meta | ./lmc.sh > lingo.json

...or

./lmc.sh < lingo.meta > lingo.json
```

### Language Codes
The language code is (obviously) very important so `lmc` will make sure you entered the right stuff.  It does so by verifying against a JSON file (`lmc.json`) that needs to accompany `lmc.sh`.  You can open that JSON file to view it if you'd like or simply have `lmc` print it for you as follows:

```shell
./lmc.sh --langs
```

### ****IMPORTANT****

- Each session that you define must have a unique ID.  If `lmc` finds duplicates, it'll yell at you and quit immediately.
- A group can only be in one session at a time.  If `lmc` finds the same group ID being used in more than one session it'll be very unhappy.

## Examples
A voiceToVoiceSession with an ID of `my-first-session` containing two Engage groups - `group1` being English (United States) and `group2` being French (France); would look as follows:

```yaml
+my-first-session
	-group1:en-US
	-group2:fr-FR
```

The resulting JSON looks as follows:
```json
{
  "voiceToVoiceSessions": [
    {
      "id": "my-first-session",
      "groups": [
        "group1"
      ]
    }
  ],
  "groups": [
    {
      "id": "group1",
      "type": 1,
      "languageCode": "en-US",
      "cryptoPassword": "",
      "txAudio": {
        "encoder": 25
      },
      "rallypoints": [
        {
          "id": "local",
          "host": {
            "address": "127.0.0.1",
            "port": 7443
          }
        }
      ]
    }
  ]
}
```

Another session with 3 groups (English, Polish, German) would look as follows:

```yaml
+another-session
	-971306c8-21df-4d20-a1bc-720d4fe35daf:en-US
	-e7f3f119-ad8a-4758-bfc3-ee599638a736:pl-PL
    -f1829595-eaa0-457a-8138-0e9e9078dcac:de-DE
```

The resulting JSON looks as follows:
```json
{
  "voiceToVoiceSessions": [
    {
      "id": "another-session",
      "groups": [
        "971306c8-21df-4d20-a1bc-720d4fe35daf",
        "e7f3f119-ad8a-4758-bfc3-ee599638a736"
      ]
    }
  ],
  "groups": [
    {
      "id": "971306c8-21df-4d20-a1bc-720d4fe35daf",
      "type": 1,
      "languageCode": "en-US",
      "cryptoPassword": "",
      "txAudio": {
        "encoder": 25
      },
      "rallypoints": [
        {
          "id": "local",
          "host": {
            "address": "127.0.0.1",
            "port": 7443
          }
        }
      ]
    },
    {
      "id": "e7f3f119-ad8a-4758-bfc3-ee599638a736",
      "type": 1,
      "languageCode": "pl-PL",
      "cryptoPassword": "",
      "txAudio": {
        "encoder": 25
      },
      "rallypoints": [
        {
          "id": "local",
          "host": {
            "address": "127.0.0.1",
            "port": 7443
          }
        }
      ]
    }
  ]
}
```

Finally, one more session but, this time, the groups are all encrypted (you can mix encrypted and unencrypted groups if desired): 

```yaml
+yet-another-session
	-an_encrypted_english_group:en-US:99909157501D483290DD4B1E5ABA77DA
	-an_encrypted_french_group:fr-FR:D162CD361AAC491D9F63E51136A35D90
    -an_encrypted_ukrainian_group:uk-UA:15D45BF3F35B4995B88A59C4BC24AC61
```

The resulting JSON looks as follows:
```json
{
  "voiceToVoiceSessions": [
    {
      "id": "yet-another-session",
      "groups": [
        "an_encrypted_english_group",
        "an_encrypted_french_group"
      ]
    }
  ],
  "groups": [
    {
      "id": "an_encrypted_english_group",
      "type": 1,
      "languageCode": "en-US",
      "cryptoPassword": "99909157501D483290DD4B1E5ABA77DA",
      "txAudio": {
        "encoder": 25
      },
      "rallypoints": [
        {
          "id": "local",
          "host": {
            "address": "127.0.0.1",
            "port": 7443
          }
        }
      ]
    },
    {
      "id": "an_encrypted_french_group",
      "type": 1,
      "languageCode": "fr-FR",
      "cryptoPassword": "D162CD361AAC491D9F63E51136A35D90",
      "txAudio": {
        "encoder": 25
      },
      "rallypoints": [
        {
          "id": "local",
          "host": {
            "address": "127.0.0.1",
            "port": 7443
          }
        }
      ]
    }
  ]
}
```

If we were to combine all the above into a single meta file as follows:

```yaml
# This is a comment line
+my-first-session
	-group1:en-US
	-group2:fr-FR

# This is another comment line
+another-session
	-971306c8-21df-4d20-a1bc-720d4fe35daf:en-US
	-e7f3f119-ad8a-4758-bfc3-ee599638a736:pl-PL
    -f1829595-eaa0-457a-8138-0e9e9078dcac:de-DE

# And this is yet another comment line
+yet-another-session
	-an_encrypted_english_group:en-US:99909157501D483290DD4B1E5ABA77DA
	-an_encrypted_french_group:fr-FR:D162CD361AAC491D9F63E51136A35D90
    -an_encrypted_ukrainian_group:uk-UA:15D45BF3F35B4995B88A59C4BC24AC61
```

Our JSON would look as follows:
```json
{
  "voiceToVoiceSessions": [
    {
      "id": "my-first-session",
      "groups": [
        "group1",
        "group2"
      ]
    },
    {
      "id": "another-session",
      "groups": [
        "971306c8-21df-4d20-a1bc-720d4fe35daf",
        "e7f3f119-ad8a-4758-bfc3-ee599638a736",
        "f1829595-eaa0-457a-8138-0e9e9078dcac"
      ]
    },
    {
      "id": "yet-another-session",
      "groups": [
        "an_encrypted_english_group",
        "an_encrypted_french_group"
      ]
    }
  ],
  "groups": [
    {
      "id": "group1",
      "type": 1,
      "languageCode": "en-US",
      "cryptoPassword": "",
      "txAudio": {
        "encoder": 25
      },
      "rallypoints": [
        {
          "id": "local",
          "host": {
            "address": "127.0.0.1",
            "port": 7443
          }
        }
      ]
    },
    {
      "id": "group2",
      "type": 1,
      "languageCode": "fr-FR",
      "cryptoPassword": "",
      "txAudio": {
        "encoder": 25
      },
      "rallypoints": [
        {
          "id": "local",
          "host": {
            "address": "127.0.0.1",
            "port": 7443
          }
        }
      ]
    },
    {
      "id": "971306c8-21df-4d20-a1bc-720d4fe35daf",
      "type": 1,
      "languageCode": "en-US",
      "cryptoPassword": "",
      "txAudio": {
        "encoder": 25
      },
      "rallypoints": [
        {
          "id": "local",
          "host": {
            "address": "127.0.0.1",
            "port": 7443
          }
        }
      ]
    },
    {
      "id": "e7f3f119-ad8a-4758-bfc3-ee599638a736",
      "type": 1,
      "languageCode": "pl-PL",
      "cryptoPassword": "",
      "txAudio": {
        "encoder": 25
      },
      "rallypoints": [
        {
          "id": "local",
          "host": {
            "address": "127.0.0.1",
            "port": 7443
          }
        }
      ]
    },
    {
      "id": "f1829595-eaa0-457a-8138-0e9e9078dcac",
      "type": 1,
      "languageCode": "de-DE",
      "cryptoPassword": "",
      "txAudio": {
        "encoder": 25
      },
      "rallypoints": [
        {
          "id": "local",
          "host": {
            "address": "127.0.0.1",
            "port": 7443
          }
        }
      ]
    },
    {
      "id": "an_encrypted_english_group",
      "type": 1,
      "languageCode": "en-US",
      "cryptoPassword": "99909157501D483290DD4B1E5ABA77DA",
      "txAudio": {
        "encoder": 25
      },
      "rallypoints": [
        {
          "id": "local",
          "host": {
            "address": "127.0.0.1",
            "port": 7443
          }
        }
      ]
    },
    {
      "id": "an_encrypted_french_group",
      "type": 1,
      "languageCode": "fr-FR",
      "cryptoPassword": "D162CD361AAC491D9F63E51136A35D90",
      "txAudio": {
        "encoder": 25
      },
      "rallypoints": [
        {
          "id": "local",
          "host": {
            "address": "127.0.0.1",
            "port": 7443
          }
        }
      ]
    }
  ]
}
```

