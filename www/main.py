import sys
import os

content_length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(content_length)

# A Resposta a sério: O Python gera o próprio cabeçalho HTTP!
html_response = f"""<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="index.css">
    <title>Cgi</title>
</head>
<body>
    <div class="text green-mark">Sucesso total do CGI!</div>
    <p>O C++ enviou este body: {body}</p>
</body>
</html>"""

print("HTTP/1.1 200 OK\r")
print(f"Content-Length: {len(html_response)}\r")
print("\r")
print(html_response, end="")