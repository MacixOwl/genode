# Monkey Network Protocol

<center>feng.yt [at] sjtu [dot] edu [dot] cn</center>

<center>gongty [at] tongji [dot] edu [dot] cn</center>

## Protocol Model

Distribute KV Network Protocol is a special type of Vesper Control Protocol. You can check the design of [Vesper Control Protocol](https://github.com/FlowerBlackG/vesper/blob/main/doc/vesper-control-protocol.md).

## Header

Every message begins with a header like this:

```
    4B        4B
+---------+---------+
|  magic  |  type   |
+---------+---------+
|       length      |
+-------------------+
```

* magic (uint32): Fixed. ASCII: `mkOS`
* type (uint32): Command identifier. Continue reading to learn more.
* length (uint64): Size of the message without header.

## Protocol Magic

Protocol magic is set to `mkOS`

## Protocol Messages

### 0xA001: Common Response

From: Protocol Version 1

Like the Response in Vesper Control Protocol.

```
  8 Bytes
+-------------------+
|                   |
+       header      +
|                   |
+---------+---------+
|  code   | msg len |
+---------+---------+
|      msg          |
|      ...          |

```

* type (uint32): `0xA001`
* code (uint32): Status code. 0 for OK.
* msg len (uint32): Size of `msg` in Bytes. **If `code` is 0, this value should be ignored.**
* msg (byte array): Data returned.

Response can be used to transfer data. When `code` is not 0, `msg` should be treated as error log. When `code` is 0, `msg`'s meaning differs to their type.

### 0x1000: Hello

Supported by any protocol version.

For:

* Client to server

Used to tell other which protocol version it supports.

```
+-------------------+
|                   |
+       header      +
|                   |
+---------+---------+
| my protocol ver 1 |
+-------------------+
| my protocol ver 2 |
+-------------------+
|       ......      |
+-------------------+
| my protocol ver n |
+-------------------+
```

If server rejects this client, send a `Hello` with no protocol version.

If server accepts this client, send a `Hello` message with its protocol versions.

Then, client should response another `Hello` with only one protocol version chosen for this connection.

Any toxic behaviours could lead to server closing the connection.

Whole procedure:

```
Server                Client
----------------------------
== Connection Established ==
  
        Versions supported
           by client
         <----------

        Versions supported
           by server
         ---------> 

        Protocol version
        for this connection
         <----------
```

### 0x1001: Auth

From: Protocol Version 1

For:

* Any node to concierge
* App to Memory Nodes

```
  8 Bytes
+-------------------+
|                   |
+       header      +
|                   |
+---------+---------+
|      challenge    |
|        ...        |
```

`challenge` is a byte array (might be a text string but not promised).

**Step1**

Server send `Auth` to receiver.

**Step2**

Receiver should encrypt it using its RC4 key and send it back using a `Response` message, putting it inside the `msg` field.

The value of `msg len` should equal to challenge's length (also `Auth`'s `length` in header) accroding to RC4 algorithm.

**Step3**

If authorized, server should send a response with code 0.

If deny, server should send a response with code which is not 0.

```
Server                Client
----------------------------
== Connection Established ==

==     Hello Greeings     ==
  
            Auth
         ---------->

     Response with cipher
         <--------- 

 Response with accept or deny
         ---------->

==      Ready to Serve    ==

```

### 0x1100: Get Identity Keys

From: Protocol Version 1

For:

* Memory Node to Concierge
* Memory Node to Memory Node (Not supported yet)

No payload required for request.

Response like:

```
  8 Bytes
+-------------------+
|                   |
+       header      +
|                   |
+---------+---------+
|  code   | msg len |
+---------+---------+
|     key head 1    |
+-------------------+
|     key head 2    |
+-------------------+
|         ...       |
+-------------------+
|     key head n    |
+-------------------+
|     key      1    |
+-------------------+
|     key      2    |
+-------------------+
|         ...       |
+-------------------+
|     key      n    |
+-------------------+


```

Key header like:

```c
struct {
    int8_t nodeType;  // 0 for App, 1 for Memory
    int8_t keyType;  // 0 for RC4, 1 for RSA
    int8_t reserved[2];  // should set to 0

    int64_t offset;  // position in response message field.
    int32_t len;  // in bytes.

    int64_t appId;
    int8_t reserved[16];  // should be set to 0
} __packed;
```

Note: Only RC4 supported yet.

### 0x2000: Memory Node Show ID

From: Protocol Version ? (Not supported yet)

For:

* Memory Node to Concierge

```
  8 Bytes
+-------------------+
|                   |
+       header      +
|                   |
+---------+---------+
|         ID        |
+---------+---------+

```

### 0x2001: Memory Node Clock In

From: Protocol Version 1

For:

* Memory Node to Concierge

```
  8 Bytes
+-------------------+
|                   |
+       header      +
|                   |
+---------+---------+
| TCP Ver.|   Port  |
+---------+---------+
| TCP4 IP |
+---------+

```

TCP Version should be 4 or 6.

Port is set in a 32-bit field, but should be in \[0, 65536\).

TCP4 IP is encoded in 4 bytes like:

```
   0     1     2     3
+-----+-----+-----+-----+
| 192 | 168 |  10 |  30 |
+-----+-----+-----+-----+
```

Concierge should reply a Response, whose message is an 8-bit integer, as memory node's unique ID.

**Only TCP Version 4 is supported in this protocol version.**

### 0x2002: Memory Node Clock Out

Not supported yet.

### 0x2003: Memory Node Handover

Not supported yet.

### 0x2004: Locate Memory Nodes

From: Protocol Version 1

For:

* App to Concierge

Request comes with no payload.

Response like:

```
  8 Bytes
+-------------------+
|                   |
+       header      +
|                   |
+---------+---------+
| code: 0 | msg len |
+-------------------+
| Memory node 1 info|  // 32 bytes
+-------------------+
| Memory node 2 info|  // 32 bytes
+-------------------+
|       ......      |
+-------------------+
| Memory node n info|  // 32 bytes
+-------------------+

```

Each `Memory node info` is packed as follows (whose size is not 8 bytes like above graph drawn):

```cpp
struct {

    int64_t id;

    int32_t tcpVersion;  // 4 or 6
    int32_t port;

    union {
        struct {
            int8_t inet4Addr[4];
            int8_t padding[12];
        } __packed inet4Addr;

        int8_t inet6Addr[16];
    };

} __packed
```

### 0x3001: Try Alloc

From: Protocol Version 1

For:

* App to Memory Nodes

```
  8 Bytes
+-------------------+
|                   |
+       header      +
|                   |
+---------+---------+
|    Block Size     |
+---------+---------+
|     N Blocks      |
+-------------------+

```

Block Size should be 4096.

If failed, a Response with code other than 0 would be returned.

If success, Response shall be like:

```
  8 Bytes
+-------------------+
|                   |
+       header      +
|                   |
+---------+---------+
| code: 0 | msg len |
+---------+---------+
|    Block ID 1     |
+-------------------+
|    Block ID 2     |
+-------------------+
|                   |
|        ...        |
|                   |
+-------------------+
|    Block ID n     |
+-------------------+

```

### 0x3002: Read Block

From: Protocol Version 1

For:

* App to Memory Nodes

```
  8 Bytes
+-------------------+
|                   |
+       header      +
|                   |
+---------+---------+
|      Block ID     |
+---------+---------+

```

Response's msg is Block data.

### 0x3003: Write Block

From: Protocol Version 1

For:

* App to Memory Nodes

```
  8 Bytes
+-------------------+
|                   |
+       header      +
|                   |
+---------+---------+
|      Block ID     |
+---------+---------+
|                   |
|   data (binary)   |
|                   |
|      ......       |
|                   |
```

### 0x3004: Check Avail Mem

From: Protocol Version 1

For:

* App to Memory Nodes

This request comes with no payload.

Response msg is a 64-bit integer indicating the available mem in this node, in bytes.

### 0x3005: Free Block

From: Protocol Version 1

For:

* App to Memory Nodes

```
  8 Bytes
+-------------------+
|                   |
+       header      +
|                   |
+---------+---------+
|      Block ID     |
+---------+---------+

```

On success, Response's msg is empty.

### 0x4001: Ping Pong

For:

* Any client node to server node

Not designed yet.
