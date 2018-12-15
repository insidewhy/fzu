#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "match.h"
#include "tty_interface.h"
#include "configuration.h"
#include "../config.h"

static int isprint_unicode(char c) {
	return isprint(c) || c & (1 << 7);
}

static int is_boundary(char c) {
	return ~c & (1 << 7) || c & (1 << 6);
}

static void clear(tty_interface_t *state) {
	tty_t *tty = state->tty;

	tty_setcol(tty, 0);
	size_t line = 0;
	while (line++ < state->options->num_lines) {
		tty_newline(tty);
	}
	tty_clearline(tty);
	if (state->options->num_lines > 0) {
		tty_moveup(tty, line - 1);
	}
	tty_flush(tty);
}

static void draw_match(tty_interface_t *state, const char *choice, int selected) {
	tty_t *tty = state->tty;
	options_t *options = state->options;
	char *search = state->last_search;

	int n = strlen(search);
	size_t positions[n + 1];
	for (int i = 0; i < n + 1; i++)
		positions[i] = -1;

	score_t score = match_positions(search, choice, &positions[0]);

	if (options->show_scores) {
		if (score == SCORE_MIN) {
			tty_printf(tty, "(     ) ");
		} else {
			tty_printf(tty, "(%5.2f) ", score);
		}
	}

	if (selected)
#ifdef TTY_SELECTION_UNDERLINE
		tty_setunderline(tty);
#else
		tty_setinvert(tty);
#endif

	tty_setnowrap(tty);
	for (size_t i = 0, p = 0; choice[i] != '\0'; i++) {
		if (positions[p] == i) {
			tty_setfg(tty, TTY_COLOR_HIGHLIGHT);
			p++;
		} else {
			tty_setfg(tty, TTY_COLOR_NORMAL);
		}
		tty_printf(tty, "%c", choice[i]);
	}
	tty_setwrap(tty);
	tty_setnormal(tty);
}

static void draw(tty_interface_t *state) {
	tty_t *tty = state->tty;
	choices_t *choices = state->choices;
	options_t *options = state->options;

	unsigned int num_lines = options->num_lines;
	size_t start = 0;
	size_t current_selection = choices->selection;
	if (current_selection + options->scrolloff >= num_lines) {
		start = current_selection + options->scrolloff - num_lines + 1;
		size_t available = choices_available(choices);
		if (start + num_lines >= available && available > 0) {
			start = available - num_lines;
		}
	}
	tty_setcol(tty, 0);
	tty_printf(tty, "%s%s", options->prompt, state->search);
	tty_clearline(tty);
	for (size_t i = start; i < start + num_lines; i++) {
		tty_printf(tty, "\n");
		tty_clearline(tty);
		const char *choice = choices_get(choices, i);
		if (choice) {
			draw_match(state, choice, i == choices->selection);
		}
	}
	if (num_lines > 0) {
		tty_moveup(tty, num_lines);
	}

	tty_setcol(tty, 0);
	fputs(options->prompt, tty->fout);
	for (size_t i = 0; i < state->cursor; i++)
		fputc(state->search[i], tty->fout);
	tty_flush(tty);
}

static void update_search(tty_interface_t *state) {
	choices_search(state->choices, state->search);
	strcpy(state->last_search, state->search);
}

static void update_state(tty_interface_t *state) {
	if (strcmp(state->last_search, state->search)) {
		update_search(state);
		draw(state);
	}
}

void action_emit(tty_interface_t *state) {
	update_state(state);

	/* Reset the tty as close as possible to the previous state */
	clear(state);

	/* ttyout should be flushed before outputting on stdout */
	tty_close(state->tty);

	const char *selection = choices_get(state->choices, state->choices->selection);
	if (selection) {
		/* output the selected result */
		printf("%s\n", selection);
	} else {
		/* No match, output the query instead */
		printf("%s\n", state->search);
	}

	state->exit = EXIT_SUCCESS;
}

void action_emit_all(tty_interface_t *state) {
	update_state(state);

	/* Reset the tty as close as possible to the previous state */
	clear(state);

	/* ttyout should be flushed before outputting on stdout */
	tty_close(state->tty);

	int end = choices_available(state->choices);
	for (int i = 0; i < end; ++i) {
		const char *selection = choices_get(state->choices, i);
		if (selection) {
			/* output the selected result */
			printf("%s\n", selection);
		} else {
			/* No match, output the query instead */
			printf("%s\n", state->search);
		}
	}

	state->exit = EXIT_SUCCESS;
}

void action_del_char(tty_interface_t *state) {
	size_t length = strlen(state->search);
	if (state->cursor == 0) {
		return;
	}
	size_t original_cursor = state->cursor;

	do {
		state->cursor--;
	} while (!is_boundary(state->search[state->cursor]) && state->cursor);

	memmove(&state->search[state->cursor], &state->search[original_cursor], length - original_cursor + 1);
}

void action_del_word(tty_interface_t *state) {
	size_t original_cursor = state->cursor;
	size_t cursor = state->cursor;

	while (cursor && isspace(state->search[cursor - 1]))
		cursor--;

	while (cursor && !isspace(state->search[cursor - 1]))
		cursor--;

	memmove(&state->search[cursor], &state->search[original_cursor], strlen(state->search) - original_cursor + 1);
	state->cursor = cursor;
}

void action_del_all(tty_interface_t *state) {
	memmove(state->search, &state->search[state->cursor], strlen(state->search) - state->cursor + 1);
	state->cursor = 0;
}

void action_prev(tty_interface_t *state) {
	update_state(state);
	choices_prev(state->choices);
}

static void action_ignore(tty_interface_t *state) {
	(void)state;
}

void action_next(tty_interface_t *state) {
	update_state(state);
	choices_next(state->choices);
}

static void action_left(tty_interface_t *state) {
	if (state->cursor > 0) {
		state->cursor--;
		while (!is_boundary(state->search[state->cursor]) && state->cursor)
			state->cursor--;
	}
}

static void action_right(tty_interface_t *state) {
	if (state->cursor < strlen(state->search)) {
		state->cursor++;
		while (!is_boundary(state->search[state->cursor]))
			state->cursor++;
	}
}

static void action_beginning(tty_interface_t *state) {
	state->cursor = 0;
}

static void action_end(tty_interface_t *state) {
	state->cursor = strlen(state->search);
}

void action_pageup(tty_interface_t *state) {
	update_state(state);
	for (size_t i = 0; i < state->options->num_lines && state->choices->selection > 0; i++)
		choices_prev(state->choices);
}

void action_pagedown(tty_interface_t *state) {
	update_state(state);
	for (size_t i = 0; i < state->options->num_lines && state->choices->selection < state->choices->available - 1; i++)
		choices_next(state->choices);
}

void action_autocomplete(tty_interface_t *state) {
	update_state(state);
	const char *current_selection = choices_get(state->choices, state->choices->selection);
	if (current_selection) {
		strncpy(state->search, choices_get(state->choices, state->choices->selection), SEARCH_SIZE_MAX);
		state->cursor = strlen(state->search);
	}
}

void action_exit(tty_interface_t *state) {
	clear(state);
	tty_close(state->tty);

	state->exit = EXIT_FAILURE;
}

static void append_search(tty_interface_t *state, char ch) {
	char *search = state->search;
	size_t search_size = strlen(search);
	if (search_size < SEARCH_SIZE_MAX) {
		memmove(&search[state->cursor+1], &search[state->cursor], search_size - state->cursor + 1);
		search[state->cursor] = ch;

		state->cursor++;
	}
}

static configuration_t configuration;

void tty_interface_init(tty_interface_t *state, tty_t *tty, choices_t *choices, options_t *options) {
	configuration_init(&configuration);
	state->tty = tty;
	state->choices = choices;
	state->options = options;
	state->ambiguous_key_pending = 0;

	strcpy(state->input, "");
	strcpy(state->search, "");
	strcpy(state->last_search, "");

	state->exit = -1;

	if (options->init_search)
		strncpy(state->search, options->init_search, SEARCH_SIZE_MAX);

	state->cursor = strlen(state->search);

	update_search(state);
}

static void handle_input(tty_interface_t *state, const char *s, int handle_ambiguous_key) {
	state->ambiguous_key_pending = 0;

	char *input = state->input;
	strcat(state->input, s);

	/* Figure out if we have completed a keybinding and whether we're in the
	 * middle of one (both can happen, because of Esc). */
	int found_keybinding = -1;
	int in_middle = 0;
	for (int i = 0; configuration.keybindings[i].key; i++) {
		if (!strcmp(input, configuration.keybindings[i].key))
			found_keybinding = i;
		else if (!strncmp(input, configuration.keybindings[i].key, strlen(state->input)))
			in_middle = 1;
	}

	/* If we have an unambiguous keybinding, run it.  */
	if (found_keybinding != -1 && (!in_middle || handle_ambiguous_key)) {
		configuration.keybindings[found_keybinding].action(state);
		strcpy(input, "");
		return;
	}

	/* We could have a complete keybinding, or could be in the middle of one.
	 * We'll need to wait a few milliseconds to find out. */
	if (found_keybinding != -1 && in_middle) {
		state->ambiguous_key_pending = 1;
		return;
	}

	/* Wait for more if we are in the middle of a keybinding */
	if (in_middle)
		return;

	/* No matching keybinding, add to search */
	for (int i = 0; input[i]; i++)
		if (isprint_unicode(input[i]))
			append_search(state, input[i]);

	/* We have processed the input, so clear it */
	strcpy(input, "");
}

int tty_interface_run(tty_interface_t *state) {
	draw(state);

	for (;;) {
		do {
			while(!tty_input_ready(state->tty, -1, 1)) {
				/* We received a signal (probably WINCH) */
				draw(state);
			}

			char s[2] = {tty_getchar(state->tty), '\0'};
			handle_input(state, s, 0);

			if (state->exit >= 0)
				return state->exit;

			draw(state);
		} while (tty_input_ready(state->tty, state->ambiguous_key_pending ? KEYTIMEOUT : 0, 0));

		if (state->ambiguous_key_pending) {
			char s[1] = "";
			handle_input(state, s, 1);

			if (state->exit >= 0)
				return state->exit;
		}

		update_state(state);
	}

	configuration_free(&configuration);
	return state->exit;
}
