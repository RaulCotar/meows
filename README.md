$${\color{tomato}M}$$iniscale $${\color{tomato}E}fficient $${\color{tomato}O}pen $${\color{tomato}W}eb $${\color{tomato}S}erver
===

MEOWS is a simple webserver for when few system resources must suffice, for example hosting websites on Raspberry PIs or running a local server while developping a website.

MEOWS is single-threaded, has a small memory footprint, and an even smaller binary.
Thanks to its simple design, it consumes very few system resources and can be quite fast when handling a small (<500) number of clients.

It is primarily meant to serve static content, offers an API that can be used to implement server-side scripting and alike.

MEOWS currenly comes as a single-source-file executable (as of version 1.0.1). In the future, this will change to a single-file library and demo application model.

## Build
Currently only tested on Linux. Simply run `make`.

## Command line usage:
- `$ meows-help`: show a help message
- `$ meows [PORT]`: start the server, by default on port 8080

More info: [license](LICENSE), [blog post](https://raulcotar.github.io/posts/meows).
