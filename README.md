# Multi-Threaded Client Transaction Server


## Introduction

The goal of this project is to become familiar with low-level POSIX
threads, multi-threading safety, concurrency guarantees, and networking.
The overall objective is to implement a simple multi-threaded
transactional object store.

### Takeaways

* Have a basic understanding of socket programming
* Understand thread execution, locks, and semaphores
* Have an advanced understanding of POSIX threads
* Have some insight into the design of concurrent data structures
* Have enhanced your C programming abilities


## The Xacto Server and Protocol: Overview

The "Xacto" server implements a simple transactional object store,
designed for use in a network environment.  For the purposes of this
assignment, an object store is essentially a hash map that maps
**keys** to **values**, where both keys and values can be arbitrary data.
There are two basic operations on the store: `PUT` and `GET`.
As you might expect, the `PUT` operation associates a key with a value,
and the `GET` operation retrieves the value associated with a key.
Although values may be null, keys may not.  The null value is used as
the default value associated with a key if no `PUT` has been performed
on that key.

A client wishing to make use of the object store first makes a network
connection to the server.  Upon connection, a **transaction** is
created by the server.  All operations that the client performs during
this session will execute in the scope of that transaction.
The client issues a series of `PUT` and `GET` requests to the server,
receiving a response from each one.  Upon completing the series of
requests, the client issues a `COMMIT` request.  If all is well, the
transaction **commits** and its effects on the store are made permanent.
Due to interference of transactions being executed concurrently by
other clients, however, it is possible for a transaction to **abort**
rather than committing.  The effects of an aborted transaction are
erased from the store and it is as if the transaction had never happened.
A transaction may abort at any time up until it has successfully committed;
in particular the response from any `PUT` or `GET` request might
indicate that the transaction has aborted.  When a transaction has
aborted, the server closes the client connection.  The client may
then open a new connection and retry the operations in the context of
a new transaction.

The Xacto server architecture is that of a multi-threaded network
server.  When the server is started, a **master** thread sets up a
socket on which to listen for connections from clients.  When a
connection is accepted, a **client service thread** is started to
handle requests sent by the client over that connection.  The client
service thread executes a service loop in which it repeatedly receives
a **request packet** sent by the client, performs the request, and
sends a **reply** packet that indicates the result.
Besides request and reply packets, there are also **data** packets
that are used to send key and value data between the client and
server.

> :nerd: One of the basic tenets of network programming is that a
> network connection can be broken at any time and the parties using
> such a connection must be able to handle this situation.  In the
> present context, the client's connection to the Xacto server may
> be broken at any time, either as a result of explicit action by the
> client or for other reasons.  When disconnection of the client is
> noticed by the client service thread, the client's transaction
> is aborted and the client service thread terminates.

### The Base Code

Here is the structure of the base code:

```
.
├── .gitlab-ci.yml
└── hw5
    ├── include
    │   ├── client_registry.h
    │   ├── data.h
    │   ├── debug.h
    │   ├── protocol.h
    │   ├── server.h
    │   ├── store.h
    │   └── transaction.h
    ├── lib
    │   ├── xacto.a
    │   └── xacto_debug.a
    ├── Makefile
    ├── src
    │   └── main.c
    ├── tests
    │   └── xacto_tests.c
    └── util
        └── client
```

The `util` directory contains an executable for a simple test client.
When you run this program it will print a help message that
summarizes the commands.  The client program understands command-line
options `-h <hostname>`, `-p <port>`, and `-q`.  The `-p` option is
required; the others are optional.  The `-q` flag tells the client to
run without prompting; this is useful if you want to run the client
with the standard input redirected from a file.
