/*
 *  Utility to find module dependencies from Modules.dep
 *  Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no>,
 *		     Martin Dahl <dahlm@vf.telia.no>,
 *		     Jaroslav Kysela <perex@suse.cz>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define WARNINGS 		1	// Output warnings to stderr

// Output methods
#define METHOD_ACINCLUDE	1
#define METHOD_MAKECONF		2
#define METHOD_INCLUDE		3

// Dependency type
typedef enum {
	TYPE_TOPLEVEL = 1,
	TYPE_LIBRARY = 2
} Type;

typedef struct depStruct dep;

typedef struct subdepStruct {
	dep *dep;
} subdep;

#define OPT_PNP_ONLY	(1 << 0)

struct depStruct {
	Type type;
	char *dir;
	char *name;
	int numdeps;
	char **depnames;
	dep **deps;
	struct depStruct *link;
	int hitflag;
	int printflag;
	int options;
	char *printed;
};

typedef struct makefileMacroStruct {
	char *ignore_in;
	char *name;
	char *header;
	char *footer;
	int indent;
	struct makefileMacroStruct *link;
} makefileMacro;

// Prototypes

static int read_file(char *filename);
static void parse_dir(char *line, char **dir);
static void add_dep(char *line, const char *dir, short type);
static dep *alloc_mem_for_dep(Type type);
static char *get_word(char *line, char *word);
static dep *find_dep(char *parent, char *depname);
static void del_all_from_list(void);

int main(int argc, char *argv[]);
static void usage(char *programname);
static char *convert_to_config_uppercase(const char *pre, const char *line);
// static char *convert_to_escape(const char *line);
static char *get_card_name(const char *line);

// Globals
static dep *Deps = NULL;			// All other modules 

static int read_file(char *filename)
{
	char *buffer, *newbuf, *dir = NULL;
	FILE *file;
	int c, prev, idx, size, result = 0;

	if (filename && strcmp(filename, "-")) {
		if ((file = fopen(filename, "r")) == NULL)
			return -errno;
	} else {
		file = stdin;
	}

	size = 512;
	buffer = (char *) malloc(size);
	if (!buffer) {
		fclose(file);
		return -ENOMEM;
	}
	while (!feof(file)) {
		buffer[idx = 0] = prev = '\0';
		while (1) {
			if (idx + 1 >= size) {
				newbuf = (char *) realloc(buffer, size += 256);
				if (newbuf == NULL) {
					result = -ENOMEM;
					goto __end;
				}
				buffer = newbuf;
			}
			c = fgetc(file);
			if (c == EOF)
				break;
			if (c == '\n') {
				if (prev == '\\') {
					idx--;
					continue;
				}
				break;
			}
			buffer[idx++] = prev = c;
		}
		buffer[idx] = '\0';
		if (buffer[0] == '%') {
			if (!strncmp(buffer, "%dir ", 5))
				parse_dir(buffer + 5, &dir);
			continue;
		} else if (buffer[0] == '|')
			add_dep(buffer + 1, dir, TYPE_TOPLEVEL);	// Toplevel modules (skip |)
		else if (isalpha(buffer[0]))
			add_dep(buffer, dir, TYPE_LIBRARY);		// Other modules
	}
      __end:
	free(buffer);
	if (dir)
		free(dir);
	if (file != stdin)
		fclose(file);
	return result;
}

// Change current directory
static void parse_dir(char *line, char **dir)
{
	char *word;
	
	if (*dir)
		free(*dir);
	word = malloc(strlen(line) + 1);
	if (word == NULL) {
		fprintf(stderr, "No enough memory\n");
		exit(EXIT_FAILURE);
	}
	get_word(line, word);
	*dir = strdup(word);
	if (*dir == NULL) {
		fprintf(stderr, "No enough memory\n");
		exit(EXIT_FAILURE);
	}
}

// Add a new dependency or soundcard to the list
static void add_dep(char *line, const char *dir, short type)
{
	dep *new_dep;
	char *word = NULL;
	int numdeps = 0;

	if (dir == NULL) {
		fprintf(stderr, "No %%dir keyword found before first dependency\n");
		exit(EXIT_FAILURE);
	}
	new_dep = alloc_mem_for_dep(type);
	new_dep->dir = strdup(dir);
	if (new_dep->dir == NULL) {
		fprintf(stderr, "No enough memory\n");
		exit(EXIT_FAILURE);
	}
	word = malloc(strlen(line) + 1);
	if (word == NULL) {
		fprintf(stderr, "No enough memory\n");
		exit(EXIT_FAILURE);
	}
	get_word(line, word);
	new_dep->name = strdup(word);	// Fill in name of dependency

	while (get_word(line, word)) {
		new_dep->depnames = realloc(new_dep->depnames, sizeof(char *) * (numdeps + 1));
		new_dep->deps = realloc(new_dep->deps, sizeof(dep *) * (numdeps + 1));
		if (new_dep->depnames == NULL || new_dep->deps == NULL) {
			fprintf(stderr, "No enough memory\n");
			exit(EXIT_FAILURE);
		}
		new_dep->depnames[numdeps] = strdup(word);
		if (new_dep->depnames[numdeps] == NULL) {
			fprintf(stderr, "No enough memory\n");
			exit(EXIT_FAILURE);
		}
		new_dep->deps[numdeps++] = NULL;
	}
	new_dep->numdeps = numdeps;
	free(word);
	return;
}

static dep *alloc_mem_for_dep(Type type)
{
	dep * firstdep = Deps, * ndep;

	ndep = (dep *) calloc(1, sizeof(dep));
	if (ndep == NULL) {
		fprintf(stderr, "No enough memory\n");
		exit(EXIT_FAILURE);
	}
	ndep->type = type;
	if (!firstdep)
		return Deps = ndep;
	while (firstdep->link)
		firstdep = firstdep->link;
	return firstdep->link = ndep;
}

// Put the first word in "line" in "word". Put the rest back in "line"
static char *get_word(char *line, char *word)
{
	int i, j, c;
	char *full_line;

	if (strlen(line) == 0)
		return NULL;

	i = 0;
	while (line[i] == ' ' || line[i] == '\t')
		i++;
	c = line[i];
	if (c != '\'' && c != '"') {
		c = ' ';
	} else {
		i++;
	}

	if (strlen(line) == i)
		return NULL;

	full_line = malloc(strlen(line + i) + 1);
	if (full_line == NULL) {
		fprintf(stderr, "No enough memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(full_line, line + i);
	for (i = 0; i < strlen(full_line); i++) {
		if ((c != ' ' && full_line[i] != c) ||
		    (c == ' ' && full_line[i] != '\t'
		     && full_line[i] != ' '))
			word[i] = full_line[i];
		else {
			// We got the whole word
			word[i++] = '\0';
			while (full_line[i] != '\0' &&
			       (full_line[i] == ' ' || full_line[i] == '\t'))
				i++;
			for (j = 0; i < strlen(full_line); i++, j++)
				line[j] = full_line[i];
			line[j] = '\0';
			free(full_line);
			return word;
		}
	}
	// This was the last word
	word[i] = '\0';
	line[0] = '\0';
	free(full_line);
	return word;
}

// Find the dependency named "depname"
static dep *find_dep(char *parent, char *depname)
{
	dep *temp_dep = Deps;

	while (temp_dep) {
		// fprintf(stderr, "depname = '%s', name = '%s'\n", depname, temp_dep->name);
		if (!strcmp(depname, temp_dep->name))
			return temp_dep;
		temp_dep = temp_dep->link;
	}
#ifdef WARNINGS
	fprintf(stderr, "Warning: Unsatisfied dep for %s: %s\n", parent,
		depname);
#endif
	return NULL;
}

// Resolve all dependencies
static void resolve_dep(dep * parent)
{
	int idx;

	while (parent) {
		for (idx = 0; idx < parent->numdeps; idx++)
			parent->deps[idx] = find_dep(parent->name, parent->depnames[idx]);
		parent = parent->link;
	}
}

// Fill list[] with all deps for dependency
static int make_list_of_deps_for_dep1(dep * parent, dep * dependency, subdep **list, int num)
{
	int i, j;
	int add;
	dep *dep;
	subdep *new_dep, *old_dep;

	if (dependency->hitflag) {
		fprintf(stderr, "endless dependency for %s parent %s\n", dependency->name, parent ? parent->name : "(none)");
		exit(EXIT_FAILURE);
	}
	dependency->hitflag = 1;
	for (i = 0; i < dependency->numdeps; i++) {
		dep = dependency->deps[i];
		if (dep) {
			add = 1;
			for (j = 0; j < num; j++) {
				old_dep = &(*list)[j];
				if (!strcmp(old_dep->dep->name, dep->name)) {
					add = 0;
					break;
				}
			}
			if (add) {
				*list = realloc(*list, sizeof(subdep) * (num + 1));
				if (*list == NULL) {
					fprintf(stderr, "No enough memory\n");
					exit(EXIT_FAILURE);
				}
				new_dep = &((*list)[num++]);
				new_dep->dep = dep;
				num = make_list_of_deps_for_dep1(dependency, dep, list, num);
			}
		}
	}
	return num;
}

// Free memory for all deps in Toplevel and Deps
static void del_all_from_list(void)
{
	int idx;
	dep *list = Deps, *next;

	while (list) {
		next = list->link;
		if (list->depnames) {
			for (idx = 0; idx < list->numdeps; idx++)
				if (list->depnames[idx])
					free(list->depnames[idx]);
			free(list->depnames);
		}

		if (list->name)
			free(list->name);
		if (list->deps)
			free(list->deps);
		free(list);
		list = next;
	}
}

// Print out ALL deps for firstdep (Cards, Deps)
void output_card_list(dep *firstdep, int space, int size)
{
	dep *temp_dep=firstdep;
	char *card_name;
	int tmp_size = 0, first = 1, idx;

	printf("  [");
	for (idx = 0; idx < space; idx++)
		printf(" ");
	while(temp_dep) {
		if (temp_dep->type == TYPE_TOPLEVEL) {
			card_name=get_card_name(temp_dep->name);
			if (!first) {
				printf(", ");
				tmp_size += 2;
			} else {
				first = 0;
			}
			if (tmp_size + strlen(card_name) + 2 > size) {
				printf("]\n  [");
				for (idx = 0; idx < space; idx++)
					printf(" ");
				tmp_size = 0;
			}
			printf(card_name);
			tmp_size += strlen(card_name);
			free(card_name);
		}
		temp_dep=temp_dep->link;
	}
}

// Output in acinlude.m4
static void output_acinclude(void)
{
	dep *tempdep;
	char *text;
	
	printf("dnl ALSA soundcard configuration\n");
	printf("dnl Find out which cards to compile driver for\n");
	printf("dnl Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no>,\n");
	printf("dnl                  Jaroslav Kysela <perex@suse.cz>\n\n");

	printf("AC_DEFUN(ALSA_TOPLEVEL_INIT, [\n");
	for (tempdep = Deps; tempdep; tempdep = tempdep->link) {
		if (tempdep->type != TYPE_TOPLEVEL)
			continue;
		text = convert_to_config_uppercase("CONFIG_", tempdep->name);
		printf("\t%s=\"\"\n", text);
		free(text);
	}
	printf("])\n\n");

	printf("AC_DEFUN(ALSA_TOPLEVEL_ALL, [\n");
	for (tempdep = Deps; tempdep; tempdep = tempdep->link) {
		int put_if = 1;
		if (tempdep->type != TYPE_TOPLEVEL)
			continue;
		if (tempdep->options & OPT_PNP_ONLY)
			printf("\tif test \"$CONFIG_ISAPNP\" = \"y\"; then\n");
		else if (strstr(tempdep->name, "pc98")) /* exception... */
			printf("\tif test \"$CONFIG_X86_PC9800\" = \"y\"; then\n");
		else if (strstr(tempdep->dir, "/isa"))
			printf("\tif test \"$CONFIG_ISA\" = \"y\"; then\n");
		else if (strstr(tempdep->dir, "/pci"))
			printf("\tif test \"$CONFIG_PCI\" = \"y\"; then\n");
		else if (strstr(tempdep->dir, "/usb"))
			printf("\tif test \"$CONFIG_USB\" = \"y\"; then\n");
		else if (strstr(tempdep->dir, "/ppc"))
			printf("\tif test \"$CONFIG_PPC\" = \"y\"; then\n");
		else if (strstr(tempdep->dir, "/arm"))
			printf("\tif test \"$CONFIG_ARM\" = \"y\"; then\n");
		else if (strstr(tempdep->dir, "/parisc"))
			printf("\tif test \"$CONFIG_PARISC\" = \"y\"; then\n");
		else if (strstr(tempdep->dir, "/pcmcia"))
			printf("\tif test \"$CONFIG_PCMCIA\" = \"y\"; then\n");
		else
			put_if = 0;
		text = convert_to_config_uppercase("CONFIG_", tempdep->name);
		printf("\t%s=\"m\"\n", text);
		printf("\tAC_DEFINE(%s_MODULE)\n", text);
		if (put_if)
			printf("\tfi\n");
		free(text);
	}
	printf("])\n\n");
	
	printf("AC_DEFUN(ALSA_TOPLEVEL_SELECT, [\n");
	printf("dnl Check for which cards to compile driver for...\n");
	printf("AC_MSG_CHECKING(for which soundcards to compile driver for)\n");
	printf("AC_ARG_WITH(cards,\n\
  [  --with-cards=<list>     compile driver for cards in <list>; ]\n\
  [                        cards may be separated with commas; ]\n\
  [                        'all' compiles all drivers; ]\n\
  [                        Possible cards are: ]\n");
	output_card_list(Deps, 24, 50);
	printf(" ],\n");
	printf("  cards=\"$withval\", cards=\"all\")\n");
	printf("if test \"$cards\" = \"all\"; then\n");
	printf("  ALSA_TOPLEVEL_ALL\n");
	printf("  AC_MSG_RESULT(all)\n");
	printf("else\n");
	printf("  cards=`echo $cards | sed 's/,/ /g'`\n");
	printf("  for card in $cards\n");
	printf("  do\n");
	printf("    case \"$card\" in\n");
	for (tempdep = Deps; tempdep; tempdep = tempdep->link) {
		if (tempdep->type != TYPE_TOPLEVEL)
			continue;
		text = get_card_name(tempdep->name);
		printf("\t%s)\n", text);
		free(text);
		text = convert_to_config_uppercase("CONFIG_", tempdep->name);
		printf("\t\t%s=\"m\"\n", text);
		printf("\t\tAC_DEFINE(%s_MODULE)\n", text);
		printf("\t\t;;\n");
		free(text);
	}
	printf("\t*)\n");
	printf("\t\techo \"Unknown soundcard $card, exiting!\"\n");
	printf("\t\texit 1\n");
	printf("\t\t;;\n");
	printf("    esac\n");
	printf("  done\n");
	printf("  AC_MSG_RESULT($cards)\n");
	printf("fi\n");
	for (tempdep = Deps; tempdep; tempdep = tempdep->link) {
		if (tempdep->type != TYPE_TOPLEVEL)
			continue;
		text = convert_to_config_uppercase("CONFIG_", tempdep->name);
		printf("AC_SUBST(%s)\n", text);
		free(text);
	}
	printf("])\n\n");
}

// Output in toplevel.conf
static void output_makeconf(void)
{
	dep *tempdep;
	char *text;
	
	printf("# Soundcard configuration for ALSA driver\n");
	printf("# Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no>,\n");
	printf("#                  Jaroslav Kysela <perex@suse.cz>\n\n");
	for (tempdep = Deps; tempdep; tempdep = tempdep->link) {
		if (tempdep->type != TYPE_TOPLEVEL)
			continue;
		text = convert_to_config_uppercase("CONFIG_", tempdep->name);
		printf("%s=@%s@\n", text, text);
		free(text);
	}
}

// Output in config.h
static void output_include(void)
{
	dep *tempdep;
	char *text;
	
	printf("/* Soundcard configuration for ALSA driver */\n");
	printf("/* Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no>, */\n");
	printf("/*                  Jaroslav Kysela <perex@suse.cz> */\n\n");
	for (tempdep = Deps; tempdep; tempdep = tempdep->link) {
		if (tempdep->type != TYPE_TOPLEVEL)
			continue;
		text = convert_to_config_uppercase("CONFIG_", tempdep->name);
		printf("#undef %s_MODULE\n", text);
		free(text);
	}
}

// example: snd-sb16 -> CONFIG_SND_SB16
static char *convert_to_config_uppercase(const char *pre, const char *line)
{
	char *holder, *p;
	int i;

	holder = malloc(strlen(line) * 2 + strlen(pre) + 1);
	if (holder == NULL) {
		fprintf(stderr, "No enough memory\n");
		exit(EXIT_FAILURE);
	}
	p = strcpy(holder, pre) + strlen(pre);
	for (i = 0; i < strlen(line); i++)
		switch (line[i]) {
		case '-':
			*p++ = '_';
			break;
		default:
			*p++ = toupper(line[i]);
			break;
		}

	*p++ = '\0';

	return holder;
}

#if 0
// example: a'b -> a\'b
static char *convert_to_escape(const char *line)
{
	char *holder, *p;
	int i;

	holder = malloc(strlen(line) + 1);
	if (holder == NULL) {
		fprintf(stderr, "No enough memory\n");
		exit(EXIT_FAILURE);
	}
	p = holder;
	for (i = 0; i < strlen(line); i++)
		switch (line[i]) {
		case '\'':
			*p++ = '`';
			break;
		default:
			*p++ = line[i];
			break;
		}

	*p++ = '\0';

	return holder;
}
#endif

// example: snd-sb16 -> sb16
static char *remove_word(const char *remove, const char *line)
{
	char *holder;
	int i;

	holder=malloc(strlen(line)-strlen(remove)+1);
	if(holder==NULL)
	{
		fprintf(stderr, "Not enough memory\n");
		exit(EXIT_FAILURE);
	}

	for(i=strlen(remove);i<strlen(line);i++)
		holder[i-strlen(remove)]=line[i];

	holder[i-strlen(remove)]='\0';

	return holder;
}

// example: snd-sb16 -> sb16
static char *get_card_name(const char *line)
{
	if (strncmp(line, "snd-", 4)) {
		fprintf(stderr, "Invalid card name '%s'\n", line);
		exit(EXIT_FAILURE);
	}
	return remove_word("snd-", line);
}

// Main function
int main(int argc, char *argv[])
{
	int method = METHOD_ACINCLUDE;
	int argidx = 1;
	char *filename;

	// Find out which method to use
	if (argc < 2)
		usage(argv[0]);
	if (strcmp(argv[argidx], "--acinclude") == 0)
		method = METHOD_ACINCLUDE;
	else if (strcmp(argv[argidx], "--makeconf") == 0)
		method = METHOD_MAKECONF;
	else if (strcmp(argv[argidx], "--include") == 0)
		method = METHOD_INCLUDE;
	else
		usage(argv[0]);
	argidx++;
	
	// Check the filename
	if (argc > argidx)
		filename = argv[argidx++];
	else
		filename = NULL;

	// Read the file into memory
	if (read_file(filename) < 0) {
		fprintf(stderr, "Error reading %s: %s",
			filename ? filename : "stdin", strerror(errno));
		exit(EXIT_FAILURE);
	}
	// Resolve dependencies
	resolve_dep(Deps);

	// Use method
	switch (method) {
	case METHOD_ACINCLUDE:
		output_acinclude();
		break;
	case METHOD_MAKECONF:
		output_makeconf();
		break;
	case METHOD_INCLUDE:
		output_include();
		break;
	default:
		fprintf(stderr, "This should not happen!\n");
		usage(argv[0]);
		break;
	}

	// Free some memory
	del_all_from_list();

	exit(EXIT_SUCCESS);
}

// Print out syntax
static void usage(char *programname)
{
	fprintf(stderr, "Usage: %s --makefile --dir directory <cfgfile>\n", programname);
	fprintf(stderr, "       %s --acinclude <cfgfile>\n", programname);
	fprintf(stderr, "       %s --makeconf <cfgfile>\n", programname);
	exit(EXIT_FAILURE);
}
