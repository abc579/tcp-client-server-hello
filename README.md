# tcp-client-server-hello

This is a simple project on which we have a server and a client; the server sends a message to the client after accepting their connection.

Naturally, the first thing to run is the server, that is, the server must be running before invoking the CLIENT program.

# Compilation

Run `make`; this will compile both the server and the client.

# Execution

1. Run the server.

    `./server.o`

2. Run the client.

    `./client.o`

# Things To Improve / Todo

+ Socket abstraction: this will be useful to avoid calling close() whenever something goes wrong but it's quite tricky.
+ Better code organization (.cpp and .hpp) files.
+ Tests. (catch, googletest...)
+ Run valgrind to check for leaks.

# Example

After executing ./server.o; it will output:

`Server: waiting for connections...`

Then, we execute ./client.o; it will output:

`Connecting to 127.0.0.1`
`Server responded with: Hello client!`

Finally, on server's terminal, we should see that someone connected:

`Server: got connection from 127.0.0.1`