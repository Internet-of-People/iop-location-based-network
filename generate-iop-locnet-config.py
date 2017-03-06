#! /usr/bin/python

import uuid
nodeId = str( uuid.uuid4() )

import hashlib
nodeId = hashlib.sha256( nodeId.encode("utf-8") ).hexdigest()

# Find more geoLocation APIs at https://www.iplocation.net/
import requests
response = requests.get("http://ipinfo.io/json")
if response.status_code != 200:
    print("Failed to automatically fetch GPS coordinates")
    quit()

import json
geoInfo = json.loads( response.content.decode("utf-8") )
(latitude, longitude) = geoInfo["loc"].split(",", 1)

print("--nodeid " + nodeId)
print("--latitude " + latitude)
print("--longitude " + longitude)
