# Example
In this example, we have two groups - English and Korean.  The group IDs for these are `en-US-AE2040D7-4306-4B58-A3DF-4BD1F2A79876` and `ko-KR-E6E3D84F-A1B5-4314-83ED-611831A977F1` respectively

A user (`JOEBLOW`) speaks on the English group (`en-US-AE2040D7-4306-4B58-A3DF-4BD1F2A79876`) saying *"One cheeseburger, please."*  He does this from the Engage node `{0A4F0C03-3A45-4F7F-A61C-83B866A9547A}`, using a transmit ID of `7332`.

## On The Originator (English)
As soon as ELS recognizes what has been said on the English group, it sends a transcription back to the group as a blob wrapped in a RTP packet.  (The RTP packet has a payload type (`pt`) of `66` with `0` for the sequence, SSRC, and timestamp.)

The callback from the client's Engage Engine to the client application comes through `on_ENGAGE_GROUP_BLOB_RECEIVED` with a `blobinfo` structure populated with a *blob payload type* of `2`, indicating `JSON UTF8`.

```shell
on_ENGAGE_GROUP_BLOB_RECEIVED [JSON TEXT UTF8]: en-US-AE2040D7-4306-4B58-A3DF-4BD1F2A79876, blobSize=221, blobJson={"payloadType":2,"rtpHeader":{"marker":false,"pt":66,"seq":0,"ssrc":0,"ts":0},"size":221,"source":"{7e7e0fc6-2efe-9bb7-58f5-e3acabd3b81f}","target":"{00000000-0000-0000-0000-000000000000}"}
	src={7e7e0fc6-2efe-9bb7-58f5-e3acabd3b81f}
	tgt={00000000-0000-0000-0000-000000000000}
	jsn={"originator":{"gid":"en-US-AE2040D7-4306-4B58-A3DF-4BD1F2A79876","alias":"JOEBLOW","nodeId":"{0A4F0C03-3A45-4F7F-A61C-83B866A9547A}","txId":7332},"subtype":"transcription","text":"One cheeseburger, please.","transcription":{"languageCode":"en-US","processor":"microsoftCognitiveServices","stage":"final"},"type":"text"}
```

Knowing that the blob payload type is JSON, the application then parses the payload data into a JSON object as follows:

```json
{
    "originator": {
        "gid": "en-US-AE2040D7-4306-4B58-A3DF-4BD1F2A79876",
        "alias": "JOEBLOW",
        "nodeId": "{0A4F0C03-3A45-4F7F-A61C-83B866A9547A}",
        "txId": 7332
    },
    "subtype": "transcription",
    "text": "One cheeseburger, please.",
    "transcription": {
        "languageCode": "en-US",
        "processor": "microsoftCognitiveServices",
        "stage": "final"
    },
    "type": "text"
}
```
The `originator` element provides information (if available) about who transmitted the audio.  In this case `JOEBLOW` from node `{0A4F0C03-3A45-4F7F-A61C-83B866A9547A}`, with a transmission ID of `7332` on the group `en-US-AE2040D7-4306-4B58-A3DF-4BD1F2A79876`.  This is in contrast with the `src` and `tgt` fields of the blob wrapper which, for this example, is the ID of the ELS instance and the target node ID of whom the message is being to (`{00000000-0000-0000-0000-000000000000}` meaning all nodes).

As for the actual transcription itself; note that the `type` field is `text`, with a primary subtype of `transcription` and the `text` field containing the actual text - `One cheeseburger, please.` in this case.

Given that the subtype is `transcription`, the application can check if a `transcription` element is present and, if it is, can process it accordingly.  Showing, for example, the language transcribed from (`en-US` in this case), what stage the transcription is (always `final` for now), and what processor did the work - `microsoftCognitiveServices` in the example.

	
## On The Target (Korean)
The target Korean group will also receive a `on_ENGAGE_GROUP_BLOB_RECEIVED` callback with the blob wrapper the same as described above.  However, the JSON content in the payload is somewhat different in that the blob message contains a multi-element data structure: a `caption` and a `translation`.

```shell
on_ENGAGE_GROUP_BLOB_RECEIVED [JSON TEXT UTF8]: ko-KR-E6E3D84F-A1B5-4314-83ED-611831A977F1, blobSize=324, blobJson={"payloadType":2,"rtpHeader":{"marker":false,"pt":66,"seq":0,"ssrc":0,"ts":0},"size":324,"source":"{7e7e0fc6-2efe-9bb7-58f5-e3acabd3b81f}","target":"{00000000-0000-0000-0000-000000000000}"}
	src={7e7e0fc6-2efe-9bb7-58f5-e3acabd3b81f}
	tgt={00000000-0000-0000-0000-000000000000}
	jsn={"caption":{"languageCode":"ko-KR","processor":"microsoftCognitiveServices"},"originator":{"gid":"en-US-AE2040D7-4306-4B58-A3DF-4BD1F2A79876","alias":"JOEBLOW","nodeId":"{0A4F0C03-3A45-4F7F-A61C-83B866A9547A}","txId":7332},"subtype":"caption","text":"치즈버거 하나 주세요.","translation":{"source":{"languageCode":"en-US","processor":"microsoftCognitiveServices","text":"One cheeseburger, please."}},"type":"text"}
```

The parsed JSON is as follows:
```json
{
    "caption": {
        "languageCode": "ko-KR",
        "processor": "microsoftCognitiveServices"
    },
    "originator": {
        "gid": "en-US-AE2040D7-4306-4B58-A3DF-4BD1F2A79876",
        "alias": "JOEBLOW",
        "nodeId": "{0A4F0C03-3A45-4F7F-A61C-83B866A9547A}",
        "txId": 7332
    },
    "subtype": "caption",
    "text": "치즈버거 하나 주세요.",
    "translation": {
        "source": {
            "languageCode": "en-US",
            "processor": "microsoftCognitiveServices",
            "text": "One cheeseburger, please."
        }
    },
    "type": "text"
}
```

First, notice that the `type` is still `text` but that the `subtype` is now `caption`.  This indicates that the `text` of the message (`치즈버거 하나 주세요.`) is the textual version of the audio it accompanies.  In the same vein as above, the `subtype` of `caption` indicates the presence of a `caption` element which further desribes the caption.  In this case that the language is `ko-KR` and the back-end processor is `microsoftCognitiveServices`.

In addition, though, the JSON also contains a `translation` element telling the application that the caption came from a translation, that the language translated from is `en-US` and that the text of the original English is `One cheeseburger, please.`.

>Note the `gid` field of the `originator` here.  Whereas in the case of the transcription, the group ID matches that of the group being transcribed, when received on the Korean group (which has an entirely different group ID), the *original* group ID (`en-US-AE2040D7-4306-4B58-A3DF-4BD1F2A79876`) is given as the *originator* of the communication is from an entirely different group (English in this case).