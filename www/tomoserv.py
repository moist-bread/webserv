import sys
import os
import json
from urllib.parse import unquote_plus, urlparse, parse_qs
from random import choice

# step 1: take in the name and topic query (or put a default one)
# step 2: randomly select a personality type
# step 3: match case to pick the sentence
# step 4: create the html

query_string = unquote_plus(os.environ.get("QUERY_STRING", "(empty)"))

html_response_head = f"""<html>
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<link rel="stylesheet" href="/index.css">
	<title>Tomoserv</title>
</head>
<body>
"""

html_response_end = f"""
</body>
</html>"""


# Parse the query string into a dictionary
params = parse_qs(query_string)
# Print the extracted parameters

def get_param(key):
	if key in params:
		return params[key][0]
	else:
		return key

def print_convo():
	convo = f"""
    <div class="text green-mark">Tomoserv!</div>
    <div class="paper">"""
	
	personality_type = choice(['fan', 'hater', 'uninterested', 'casual', 'hyped'])
	

	convo += f"""
        <p>website personality type: <b>{personality_type}</b></p>
	{get_param("name")}<br>
	{get_param("topic")}<br>
	{get_param("bruh")}<br>
	{params}<br>
	"""


	convo += f"""
    </div>
	"""
	print(convo)

print("Content-Type: text/html\r")
print("\r")
print(html_response_head, end="")
try:
	print_convo()
except:
	print("Error")
print(html_response_end, end="")
