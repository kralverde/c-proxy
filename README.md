A proxy of sorts, I don't know what I would call this and google didn't turn anything up, so I just whipped this up.

General Idea is:

Service <-> Client <-> Server <-> Connections

Where the client initializes an out going request to the server. The client and server route connections to the service and back. Useful if you have a public server and a service behind a NAT or something. Definately not production viable, but I use it for my minecraft server(s) if that means anything...
