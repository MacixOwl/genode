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

`Response`

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
* msg len (uint32): Size of `msg` in Bytes.
* msg (byte array): Data returned.

Response can be used to transfer data. When `code` is not 0, `msg` should be treated as error log. When `code` is 0, `msg`'s meaning differs to their type.

### 0x1001: Auth

`Auth`

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

Receiver should encrypt it using its RC4 key and send it back using a `Response` message, putting it inside the `msg` field.

The value of `msg len` should equal to challenge's length (also `Auth`'s `length` in header) accroding to RC4 algorithm.

### 0x2001: Memory Node Clock In

### 0x2002: Memory Node Clock Out

### 0x2003: Memory Node Handover

Not supported yet.

### 0x2004: Locate Memory Nodes

### 0x3001: Try Alloc

### 0x3002: Read Block

### 0x3003: Write Block

### 0x4001: Ping Pong
