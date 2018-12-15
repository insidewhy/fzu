**fzu** is a fork of fzu with configurable keybindings and support for selecting multiple files.

See [fzy](https://github.com/jhawthorn/fzy).

## Configuration

fzu loads configuration from `~/.config/fzu` when it is available. This example configuration file shows the default key bindings:

```
key-bindings = {
	up        = "prev"
	c-k       = "prev"
	c-p       = "prev"
	down      = "next"
	c-j       = "next"
	c-n       = "next"
	c-c       = "exit"
	c-d       = "exit"
	delete    = "del char"
	c-h       = "del char"
	c-w       = "del word"
	c-u       = "del all"
	c-m       = "emit"
	tab       = "autocomplete"
	page-up   = "page up"
	page-down = "page down"
}
