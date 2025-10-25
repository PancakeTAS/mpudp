# mpudp

MultiPort UDP is a solution for a problem I doubt many people have. If you live in a student dorm, or similar, where the router you are connected to has multiple uplinks and decides to choose one for every new connection or socket opened, then keep reading.

If you're like me and running `curl https://ipinfo.io/what-is-my-ip` gives two different results depending on.. something that isn't in your control and the speed from speedtests does not correlate to your actual speeds, then you might be suffering from an existential problem called: "your home owner is so rich that they actually bought two internet connections and not one /s".

This project, mpudp, works around this limitation by repeatedly opening UDP sockets with a remote server, until two (or more) sockets with separate IP addresses have been opened. You may think UDP is connectionless.. and you'd be right, but that doesn't mean each individual hop doesn't store where from and where to each packet should be forwarded. As such an established socket pair will never switch to another IP and several pairs on different ports will never interfere with each other.

## Compiling mpudp

This is a fairly straight forward process, you'll need `clang++`, `clang-tidy`, `cmake` and `ninja`. The C++ compiler needs to be able to compile C++20 or newer and ideally you'd also have the C and C++ standard library on your system.

Building is as simple as this:
```bash
$ cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=On
$ cmake --build build
```

Grab `build/mpudp` and put in your favorite bin folder. Or _folders_, because you need a server for this project.

## Configuring mpudp

On the client, create a file called `mpudp.toml` in the same directory as where you're executing `mpudp` from:
```toml
[client]
peer = "1.1.1.1"
baseport = 5000
listenport = 51820

[[client.connection]]
weight = 156

[[client.connection]]
weight = 138
```

In this file, `peer` corresponds to the server you're trying to connect to, `baseport` corresponds to the lowest port from which connections will be opened (with each `[[client.connection]]` entry adding +1 to this), and `listenport` corresponds to the port on your local system, from which connections will be taken and fed through the tunnel.

If your goal is to forward a WireGuard connection to your server through both uplinks, then your `listenport` should be 51820 and your WireGuard network device should be configured to connect to `127.0.0.1:51820`.

Finally, each connection is assigned a `weight` (and no this is not optional, but feel free to set `1`) for weighted round-robin distribution. The idea is, if one uplink is faster than the other (e.g. 156 mbps vs 138 mbps), then the weighted round-robin algorithm will distribute slightly more packets on one uplink, compared to the other. The algorithm used it intelligent enough not to overload one uplink for a second too!

Next up let's configure the server, create a `mpudp.toml` file:
```toml
[server]
baseport = 5000
sendport = 51280

[[server.connection]]
peer = "80.150.90.20"
weight = 156

[[server.connection]]
peer = "50.170.20.30"
weight = 134
```

Again here you have a `baseport`, which connections will be opened from, but instead of a `listenport` you have a `sendport`. They're basically the same, so data received through the tunnel will be sent to the port specified (and returned back of course).

Aside from `weight`, each connection also needs a `peer` field, so the server can check if the connection was made from the right IP address and otherwise rejected and reopened.

Make sure your firewall is configured properly.

## Running mpudp

Once you're ready, start the server, then the client... you can start the client first, but then you'll have to wait 5 seconds for a timeout. Speaking of which, what output does mpudp provide?

### Troubleshooting the client

#### "connection handshake failed on port ..."

This message simply indicates that the server replied 'N' to the handshake, indicating that the connection was opened from the wrong IP address. The client will simply wait a few milliseconds and retry.

#### "connection timeout on port ..."

If the handshake did not yet succeed, it may be because the server is offline, or because the handshake packet was dropped. The client will catch this and retry after a few seconds.

#### "dropping packet due to invalid connection on port ..."

This warning is shown when a client (such as WireGuard) connected to `listenport`, without the tunnel being fully established yet. It's safe to ignore and shouldn't appear after the handshakes are complete.

### Troubleshooting the server

#### "invalid handshake on port ..."

This message is the counterpart to the client handshake failed. The server got a packet that was either malformed or simply not from the right IP address and return back 'N' to the client.

#### "dropped invalid packet on port ..."

This one is important: Should traffic be received from an IP address not agreed upon during the handshake, this warning will be shown and the single packet will be dropped.

If your UDP port is open, it makes sense for bots to knock on it some time so in that case you can ignore it. If that isn't the case and your client has changed IP addresses mid-socket, then you should probably restart the connection.

### Other notes

When restarting the connection, you always have to restart the client *and* the server. If you don't, then the server will think the tunnel is already set up and blatantly forward the client handshake to the end application.
