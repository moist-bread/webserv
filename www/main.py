import sys
import os

content_length = int(os.environ.get("CONTENT_LENGTH", 0))
request = os.environ.get("REQUEST_METHOD", "(empty)")
query_string = os.environ.get("QUERY_STRING", "(empty)")
body = sys.stdin.read(content_length)

# A Resposta a sério: O Python gera o próprio cabeçalho HTTP!
html_response = f"""<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="/index.css">
    <title>Cgi</title>
</head>
<body>
    <div class="text green-mark">Sucesso total do CGI!</div>
    <p>request type: {request}</p>
    <p>O C++ enviou este body: {body}</p>
    <p>O C++ enviou esta query string: {query_string}</p>
</body>
</html>"""

print("HTTP/1.1 200 OK\r")
print(f"Content-Length: {len(html_response)}\r")
print(os.environ)
print("\r")
print(html_response, end="")