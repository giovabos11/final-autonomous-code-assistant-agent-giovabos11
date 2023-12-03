import openai
from openai import OpenAI
import argparse

# Take arguments
argParser = argparse.ArgumentParser()
argParser.add_argument("-i", "--ip", required=True, help="IP address of LLM host")
argParser.add_argument("-p", "--prompt", required=True, help="Prompt for the LLM")
argParser.add_argument("-m", "--model", required=True, help="Model to use (local models only)")
argParser.add_argument("-k", "--key", help="API key")
args = argParser.parse_args()
if not args.key:
    args.key = "not needed for a local LLM"

client = OpenAI(
    #base_url = "https://api.openai.com/v1"
    #base_url = "http://localhost:4891/v1",
    base_url = args.ip,
    api_key = args.key,
)

# model = "gpt4all-falcon-q4_0"
# model = "mistral-7b-instruct-v0.1.Q4_0"

# Make the API request
try:
    completion = client.completions.create(
        model=args.model,
        prompt=args.prompt,
        max_tokens=500,
    )
except openai.APIConnectionError as e:
    print("Connection error!")
    exit()

# Print the generated completion
#print(completion)
#print("\n\n\n")
#print(completion.choices[0].text)
#print("\n\n\n")
#print(dict(completion).get('usage'))
#print("\n\n\n")
print(completion.model_dump_json(indent=2))
#print("\n\n\n")
