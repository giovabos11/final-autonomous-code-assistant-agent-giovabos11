import requests
import argparse
import json

# Take arguments
argParser = argparse.ArgumentParser()
argParser.add_argument("-i", "--ip", required=True, help="IP address of LLM host")
argParser.add_argument("-p", "--prompt", required=True, help="Prompt for the LLM")
argParser.add_argument("-m", "--model", required=True, help="Model to use (local models only)")
argParser.add_argument("-k", "--key", help="API key")
args = argParser.parse_args()

# If an API key is not provided, add custom GPT4All arguments
if not args.key:
    args.key = "not needed for a local LLM"
    data = {
        'model': args.model,
        'messages': [
            {
                'role': 'system',
                'content': ''
            },
            {
                'role': 'user',
                'content': args.prompt,
            },
        ],
        'max_tokens': 1024,
        'response_format': {
            'type': 'json_object'
        },
        'temperature': 0.7,
    }
# Otherwise, pass basic LLM arguments
else:
    data = {
        'model': args.model,
        'messages': [
            {
                'role': 'system',
                'content': ''
            },
            {
                'role': 'user',
                'content': args.prompt,
            },
        ],
        'max_tokens': 1024,
    }

headers = {
    'Content-Type': 'application/json',
    'Authorization': 'Bearer ' + args.key,
}

try :
    response = requests.post(args.ip, headers = headers, json = data)
except requests.exceptions.ConnectionError:
    print("Connection error")
    exit()

if not response.status_code == 200:
    print("Connection error")
    exit()

print(json.dumps(response.json()))
