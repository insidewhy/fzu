**fzu** is a fast, simple fuzzy text selector for the terminal with an advanced [scoring
algorithm](#sorting).

## Why use this over fzf, pick, selecta, ctrlp, ...?

fzu is faster and shows better results than other fuzzy finders.

Most other fuzzy matchers sort based on the length of a match. fzu tries to
find the result the user intended. It does this by favouring matches on
consecutive letters and starts of words. This allows matching using acronyms or
different parts of the path.

A gory comparison of the sorting used by fuzzy finders can be found in [ALGORITHM.md](ALGORITHM.md)

fzu is designed to be used both as an editor plugin and on the command line.
Rather than clearing the screen, fzu displays its interface directly below the current cursor position, scrolling the screen if necessary.

## Installation

### From source

    make
    sudo make install

The `PREFIX` environment variable can be used to specify the install location,
the default is `/usr/local`.

## Usage

fzu is a drop in replacement for [selecta](https://github.com/garybernhardt/selecta) and [fzy](https://github.com/jhawthorn/fzy), and can be used with [selecta's usage examples](https://github.com/garybernhardt/selecta#usage-examples).

### Use with Vim

fzu can be easily integrated with vim via the neovim-fuzzy project.

## Sorting

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
```
