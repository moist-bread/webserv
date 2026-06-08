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
<body>\n"""

html_response_end = f"""\n</body>\n</html>"""

# Parse the query string into a dictionary
params = parse_qs(query_string)
# Print the extracted parameters

def get_param(key):
	if key in params:
		return params[key][0]
	else:
		return key

def print_convo():
	convo = f"""\n\t<div class="text green-mark">Your TomoServ has replied!</div>
\t<div class="paper">"""
	
	personality_type = choice(['hater', 'uninterested', 'casual', 'hyped'])

	convo += "\n\t\t<p><b>"

	match personality_type:
		case 'hater':
			convo += f"""EWWWW. How dare you even MENTION {get_param("topic")} out loud. You're a complete freak, {get_param("name")}!"""
		case 'uninterested':
			convo += f"""Hum? What did you say, {get_param("name")}? Talking about {get_param("topic")} AGAIN? Couldn't care less..."""
		case 'casual':
			convo += f"""Good evening, {get_param("name")}. I'd never heard about {get_param("topic")} before. Care to explain more about it?"""
		case 'hyped':
			convo += f"""FINALLY! I've waited for days like this where I could talk about {get_param("topic")} with someone. THANK YOU, {get_param("name").upper()}!"""
		
	convo += f"""</b></p>\n"""
	convo += f"""\t\t<p>website personality type: <b>{personality_type}</b></p>\n"""
	
	convo += f"""\t</div>\n"""
	print(convo)

print("Content-Type: text/html\r")
print("\r")
print(html_response_head, end="")
try:
	print_convo()
except:
	print("I don't feel like talking today...")
print(html_response_end, end="")
