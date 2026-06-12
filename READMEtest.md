READ.ME

The configuration file is heavily inspired by NGINX and is configured using a single, custom '.conf' file. The configuration is hierarchical, consisting of Server Blocks that define local hosts by ports, and Location Blocks that define specific routing rules for URIs.

The configuration parser is highly strict to prevent ambiguous behavior at runtime, therefore, it enforces strict logical validation to ensure the server always boots in a stable state. When writing or modifying the '.conf' file, ensure you follow these formatting rules:

# SYNTAX

Non-Linear Declaration:
	The order of directives inside a block does not matter. The parser will read and apply all rules correctly regardless of their sequence.

Tokens & Spacing:
	Directives and their arguments must be separated by whitespace (spaces, tabs or even newlines). Multiple spaces are treated as a single delimiter.

Semicolons (;):
	Every single-line directive must be terminated with a semicolon.

		✅ Valid: listen 8080;

		❌ Invalid: listen 8080 (Will throw a Syntax Error)

Block Contexts ({ }):
	Multi-line contexts like server and location must be enclosed in curly braces. The opening brace can be on the same line or the next line, but the block itself does not end with a semicolon.

Case Sensitivity:
	All directive keywords (e.g., server_name, allow_methods) are strictly lowercase and case-sensitive.

Paths & Extensions:
	* Folder paths should follow standard Unix formatting.
	File extensions for CGI must explicitly include the leading dot (e.g., .php, not php).

Comments:
	Any text following a # on a line is treated as a comment and completely ignored by the parser.

# Directives Description and Rules 

Server Context
	listen <IP:PORT | PORT>: Defines the network interface and port to bind to. (Collisions between different servers with the same port will result in a validation error).
		<Port: 1 <|> 65535>
		<Mandatory directive>
		<Unique directive>

	server_name <name>: The domain name associated with the server block.
		<Default - 'localhost'>
		<Unique directive>

	client_max_body_size <bytes>: Limits the maximum size of incoming HTTP request bodies. Adding 'M' in the end of the value represents it in Megabytes.
		<Max value = 524288000>
		<Default - 1048576>
		<Unique directive>

	root <path>: The default physical directory on the server's hard drive to serve files from.
		<If not defined it must be defined inside location>
		<Unique directive>

	error_page <status_code> <path>: Defines a custom HTML file to serve when the server encounters a specific HTTP error (e.g., 404, 500). This overrides the server's default error response. You can add multiple directives, they will be added to the list, if status code is repeated it will overwrite the path.
		<Status code: 400 <|> 599>
		<Multiple directive>

Location Context
Locations inherit global settings from their parent server, but can override them.

	location <route_or_extension>: Defines a dedicated configuration block for a specific URI path (e.g., /upload) or file type (e.g., *.php). Directives placed inside this block dictate exactly how the server should route and process requests that match this specific path, overriding global server settings where applicable. Route or extension can't be duplicated, URI and extensions must be unique.
		<Mandatory directive>
		<Multiple directive>

Standard Static Routing:

	allow_methods <GET | POST | PUT | DELETE | HEAD | PATCH>: Restricts which HTTP methods are permitted. If declared multiple allow_methods, it will be added to the config list.
		<Multiple directive>

	autoindex <on | off>: Enables directory listing if an index file is not found.
		<Unique directive>

	index <file>: The default file to serve when a directory is requested.
		<Unique directive>

	upload_store <path>: Required if POST is allowed. Defines where uploaded files are physically saved.
		<Unique directive>

	root <path>: The default physical directory on the server's hard drive to serve files from. <If not defined inherits from server>
		<Unique directive>

	cgi <extension> <executable_path>: Enables CGI execution for specific file types within this standard folder (e.g., cgi .py /usr/bin/python3).
		<Multiple directive>

HTTP Redirections:

	allow_methods <GET | POST | PUT | DELETE | HEAD | PATCH>: Restricts which HTTP methods are permitted. If declared multiple allow_methods, it will be added to the config list.
		<Multiple directive>

	return <status_code> <url>: Instantly redirects the client.
		<Unique directive>

	Strict Rule: A redirect block cannot contain root, index, autoindex, cgi, or upload_store directives.

CGI Execution (e.g., location *.php):

	cgi_pass <executable_path>: Maps the file extension to a specific binary (e.g., /usr/bin/php-cgi).
		<Unique directive>

	Strict Rule: CGI blocks cannot contain an upload_store. The web server pipes POST data directly to the CGI script's stdin, and it is the script's responsibility to handle the file.

# CONFIG TEMPLATE

server {
    listen 8080; # alternative - 127.0.0.1:8080
    server_name localhost;
    client_max_body_size 1000000;
    
    # Global root for all locations
    root ./www/html; 

	# Custom Error Pages
    error_page 404 /errors/404.html;
    error_page 500 502 503 504 /errors/50x.html;

    # 1. Standard Folder (With internal CGI support)
    location / {
        allow_methods GET POST;
        index index.html;
        upload_store ./www/uploads;
        cgi .py /usr/bin/python3; # Python scripts run here!
    }

    # 2. Dedicated CGI Endpoint (Extension matching)
    location *.php {
        allow_methods GET POST;
        cgi_pass /usr/bin/php-cgi;
        # Note: Inherits the global root, but does NOT define upload_store!
    }

    # 3. HTTP Redirect
    location /old-site {
        allow_methods GET;
        return 301 http://127.0.0.1:8080/new-site;
    }
}

# CGI upload store ???
