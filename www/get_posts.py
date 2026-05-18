
# step 1: see if database file exists
# step 2: open it and convert to json
# step 3: in a for loop for each elem of json that has 
# step 4: if no database or correct elem set print EMPTY

import sys
import os
from urllib.parse import unquote_plus
import time
import json

content_length = int(os.environ.get("CONTENT_LENGTH", 0))
request = os.environ.get("REQUEST_METHOD", "(empty)")
query_string = unquote_plus(os.environ.get("QUERY_STRING", "(empty)"))
body = unquote_plus(sys.stdin.read(content_length))

# urllib.parse.parse_qs(query) will parse it into a dictionary

html_response_head = f"""<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="/index.css">
    <title>get_posts</title>
</head>
<body>
"""

html_response_end = f"""
</body>
</html>"""

def print_posts():
	database = []
	with open("www/database", mode="r", encoding="utf-8") as read_file:
	    for line in read_file:
       		database.append(json.loads(line))
	for dict_elem in database:
		if "notes" in dict_elem and "username" in dict_elem and "title" in dict_elem:
			post_list_item = f"""	<li class="post-it">
		<h2>{unquote_plus(dict_elem["title"])}</h2>
		<span class="subtitle">by: {unquote_plus(dict_elem["username"])}</span>
		<p>{unquote_plus(dict_elem["notes"])}</p>
	</li>"""
			print(post_list_item)

print("Content-Type: text/html\r")
print("\r")
print(html_response_head, end="")
try:
	print_posts()
except:
	print("Error")
print(html_response_end, end="")
