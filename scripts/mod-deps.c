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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define WARNINGS 		1	// Output warnings to stderr

// Output methods
#define METHOD_MAKEFILE		1

// Dependency type
typedef enum {
	TYPE_TOPLEVEL = 1,
	TYPE_LIBRARY = 2
} Type;

typedef struct depStruct {
	Type type;
	char *dir;
	char *name;
	int numdeps;
	char **depnames;
	struct depStruct **deps;
	struct depStruct *link;
	int nummacros;
	char **macronames;
	int hitflag;
	int printflag;
} dep;

typedef struct makefileMacroStruct {
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
static void parse_makefile_outdesc(char *line);
static dep *alloc_mem_for_dep(Type type);
static char *get_word(char *line, char *word);
static dep *find_dep(char *parent, char *depname);
static int make_list_of_deps_for_dep(dep * dependency, dep **list[]);
static void del_all_from_list(void);

int main(int argc, char *argv[]);
static void usage(char *programname);
static void output_makefile(const char *dir, int all);
static char *convert_to_config_uppercase(const char *pre, const char *line);
// static char *convert_to_escape(const char *line);

// Globals
static dep *Deps = NULL;			// All other modules 
static makefileMacro *makefileMacros = NULL;	// All makefile macros

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
			if (!strncmp(buffer, "%makefile ", 10))
				parse_makefile_outdesc(buffer + 10);
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
		if (word[0] == '@') {	/* macro */
			new_dep->macronames = realloc(new_dep->macronames, sizeof(char *) * (new_dep->nummacros + 1));
			if (new_dep->macronames == NULL) {
				fprintf(stderr, "No enough memory\n");
				exit(EXIT_FAILURE);
			}
			new_dep->macronames[new_dep->nummacros] = strdup(word);
			if (new_dep->macronames[new_dep->nummacros] == NULL) {
				fprintf(stderr, "No enough memory\n");
				exit(EXIT_FAILURE);
			}
			new_dep->nummacros++;
			continue;
		}
		new_dep->depnames = realloc(new_dep->depnames, sizeof(char *) * (numdeps + 1));
		new_dep->deps = realloc(new_dep->deps, sizeof(new_dep) * (numdeps + 1));
		if (new_dep->depnames == NULL || new_dep->deps == NULL) {
			fprintf(stderr, "No enough memory\n");
			exit(EXIT_FAILURE);
		}
		new_dep->depnames[numdeps] = strdup(word);
		if (new_dep->depnames[numdeps] == NULL) {
			fprintf(stderr, "No enough memory\n");
			exit(EXIT_FAILURE);
		}
		new_dep->deps[numdeps] = NULL;
		numdeps++;
	}
	new_dep->numdeps = numdeps;
	free(word);
	return;
}

static void add_makefile_text(char **dst, const char *src)
{
	int len = *dst ? strlen(*dst) : 0;
	char *tmp;
	
	tmp = malloc(len + strlen(src) + 2);
	if (tmp == NULL) {
		fprintf(stderr, "No enough memory\n");
		exit(EXIT_FAILURE);
	}
	if (*dst) {
		strcpy(tmp, *dst);
		free(*dst);
	} else {
		tmp[0] = 0;
	}
	strcat(tmp, src);
	strcat(tmp, "\n");
	*dst = tmp;
}

// Parse makefile output description
static void parse_makefile_outdesc(char *line)
{
	static enum {
		NONE = 0,
		HEADER = 1,
		FOOTER = 2,
	} command = NONE;
	static makefileMacro *macro = NULL;
	char *word;
	
	word = malloc(strlen(line) + 1);
	if (word == NULL) {
		fprintf(stderr, "No enough memory\n");
		exit(EXIT_FAILURE);
	}
	get_word(line, word);
	if (macro == NULL) {
		if (!strcmp(word, "group")) {
			get_word(line, word);
			if (word[0] == '\0') {
				fprintf(stderr, "macro group has null name\n");
				exit(EXIT_FAILURE);
			}
			macro = (makefileMacro *) calloc(1, sizeof(makefileMacro));
			if (macro == NULL) {
				fprintf(stderr, "No enough memory\n");
				exit(EXIT_FAILURE);
			}
			macro->name = strdup(word);
			if (macro->name == NULL) {
				fprintf(stderr, "No enough memory\n");
				exit(EXIT_FAILURE);
			}
			return;
		} else {
			fprintf(stderr, "Unknown command '%s' for makefile macro section\n", word);
			exit(EXIT_FAILURE);
		}
	}
	if (command == NONE) {
		if (!strcmp(word, "endgroup")) {
			get_word(line, word);
			if (word[0] == '\0') {
				fprintf(stderr, "macro endgroup has null name\n");
				exit(EXIT_FAILURE);
			}
			if (strcmp(macro->name, word)) {
				fprintf(stderr, "endgroup name does not match group name\n");
				exit(EXIT_FAILURE);
			}
			macro->link = makefileMacros;
			makefileMacros = macro;
			macro = NULL;
			return;
		} else if (!strcmp(word, "header")) {
			command = HEADER;
		} else if (!strcmp(word, "footer")) {
			command = FOOTER;
		} else if (!strcmp(word, "indent")) {
			get_word(line, word);
			macro->indent = atoi(word);
		} else {
			fprintf(stderr, "unknown command %s (none scope)\n", word);
			exit(EXIT_FAILURE);
		}
	} else if (!strcmp(word, "line")) {
		if (command == HEADER)
			add_makefile_text(&macro->header, line);
		else if (command == FOOTER)
			add_makefile_text(&macro->footer, line);
		else {
			fprintf(stderr, "wrong line command usage\n");
			exit(EXIT_FAILURE);
		}
	} else if (!strcmp(word, "endheader")) {
		if (command != HEADER) {
			fprintf(stderr, "wrong endheader command usage\n");
			exit(EXIT_FAILURE);
		}
		command = NONE;
	} else if (!strcmp(word, "endfooter")) {
		if (command != FOOTER) {
			fprintf(stderr, "wrong endfooter command usage\n");
			exit(EXIT_FAILURE);
		}
		command = NONE;
	} else {
		fprintf(stderr, "wrong %s command (%i scope)\n", word, command);
		exit(EXIT_FAILURE);
	}
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

// Find the macro named "depname"
static makefileMacro *find_makefileMacro(char *macroname)
{
	makefileMacro *macro = makefileMacros;

	while (macro) {
		// fprintf(stderr, "macroname = '%s', name = '%s'\n", macroname, macro->name);
		if (!strcmp(macroname, macro->name))
			return macro;
		macro = macro->link;
	}
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
static int make_list_of_deps_for_dep1(dep * parent, dep * dependency, dep **list[], int num)
{
	int i, j;
	int add;

	if (dependency->hitflag) {
		fprintf(stderr, "endless dependency for %s parent %s\n", dependency->name, parent ? parent->name : "(none)");
		exit(EXIT_FAILURE);
	}
	dependency->hitflag = 1;
	for (i = 0; i < dependency->numdeps; i++)
		if (dependency->deps[i]) {
			add = 1;
			for (j = 0; j < num; j++)
				if (!strcmp((*list)[j]->name, dependency->deps[i]->name)) {
					add = 0;
					break;
				}
			if (add) {
				*list = realloc(*list, sizeof(dep *) * (num + 1));
				if (*list == NULL) {
					fprintf(stderr, "No enough memory\n");
					exit(EXIT_FAILURE);
				}
				(*list)[num++] = dependency->deps[i];
				num = make_list_of_deps_for_dep1(dependency, dependency->deps[i], list, num);
			}
		}
	return num;
}

// Clear all print flags
static void clear_printflags(void)
{
	dep *temp_dep = Deps;

	while (temp_dep) {
		temp_dep->printflag = 0;
		temp_dep = temp_dep->link;
	}
}

// Fill list[] with all deps for dependency
static int make_list_of_deps_for_dep(dep * dependency, dep **list[])
{
	dep * temp_dep = Deps;

	while (temp_dep) {
		temp_dep->hitflag = 0;
		temp_dep = temp_dep->link;
	}
	*list = NULL;
	return make_list_of_deps_for_dep1(NULL, dependency, list, 0);
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
		if (list->macronames) {
			for (idx = 0; idx < list->nummacros; idx++)
				if (list->macronames[idx])
					free(list->macronames[idx]);
			free(list->macronames);
		}
		if (list->name)
			free(list->name);
		if (list->deps)
			free(list->deps);
		free(list);
		list = next;
	}
}

// Print spaces
static void print_indent(int indent)
{
	while (indent >= 8) {
		printf("\t");
		indent -= 8;
	}
	while (indent-- > 0)
		printf(" ");
}

// Output in Makefile.in format
static void output_makefile1(const char *dir, int all)
{
	dep *tempdep, **list;
	char *text;
	int num, idx, midx, vidx, lidx, first, mfirst, macroloop, indent;
	int nummacros = 0, indir = 0;
	makefileMacro **macros = NULL;

	for (tempdep = Deps; tempdep; tempdep = tempdep->link) {
		if (!all && tempdep->type != TYPE_TOPLEVEL)
			continue;
		if (tempdep->printflag)
			continue;
		indir = !dir || !strcmp(dir, tempdep->dir);
		for (midx = 0; midx < tempdep->nummacros; midx++) {
			makefileMacro *macro = find_makefileMacro(tempdep->macronames[midx++]);
			for (vidx = 0; vidx < nummacros; vidx++) {
				if (macros[vidx] == macro)
					break;
			}
			if (vidx >= nummacros) {
				macros = (makefileMacro **)realloc(macros, sizeof(makefileMacro *) * (nummacros + 1));
				if (macros == NULL) {
					fprintf(stderr, "No enough memory\n");
					exit(EXIT_FAILURE);
				}
				macros[nummacros++] = macro;
			}
			if (macro != NULL)
				goto __out1_1;
		}
		first = 1;
		num = make_list_of_deps_for_dep(tempdep, &list);
		for (idx = 0; idx < num; idx++) {
			dep *ldep = list[idx];
			if (dir && strcmp(dir, ldep->dir))
				continue;
			for (midx = 0; midx < ldep->nummacros; midx++) {
				makefileMacro *macro = find_makefileMacro(ldep->macronames[midx++]);
				for (vidx = 0; vidx < nummacros; vidx++) {
					if (macros[vidx] == macro)
						break;
				}
				if (vidx >= nummacros) {
					macros = (makefileMacro **)realloc(macros, sizeof(makefileMacro *) * (nummacros + 1));
					if (macros == NULL) {
						fprintf(stderr, "No enough memory\n");
						exit(EXIT_FAILURE);
					}
					macros[nummacros++] = macro;
				}
				if (macro != NULL)
					goto __out1;
			}
			if (first) {
				text = convert_to_config_uppercase("CONFIG_", tempdep->name);
				printf("obj-$(%s) +=", text);
				free(text);
				tempdep->printflag = 1;
				first = 0;
				if (indir)
					printf(" %s.o", tempdep->name);
			}
			printf(" %s.o", ldep->name);
		      __out1:
		}
		free(list);
		if (!first)
			printf("\n");
		if (first && indir) {
			text = convert_to_config_uppercase("CONFIG_", tempdep->name);
			printf("obj-$(%s) += %s.o\n", text, tempdep->name);
			free(text);
		}
	      __out1_1:
	}
	if (nummacros == 0)
		macroloop = 0;
	else {
		macroloop = 1;
		for (idx = 1; idx < nummacros; idx++)
			macroloop <<= 1;
	}
	for (lidx = 1; lidx <= macroloop; lidx++) {
		indent = 0;
		mfirst = 1;
		for (tempdep = Deps; tempdep; tempdep = tempdep->link) {
			if (!all && tempdep->type != TYPE_TOPLEVEL)
				continue;
			indir = !dir || !strcmp(dir, tempdep->dir);
			first = 1;
			for (midx = 0; midx < tempdep->nummacros; midx++) {
				makefileMacro *macro = find_makefileMacro(tempdep->macronames[midx]);
				if (macro == NULL)
					goto __ok2_2;
				for (vidx = 0; vidx < nummacros; vidx++) {
					if (!(lidx & (1 << vidx)))
						continue;
					if (macro == macros[vidx])
						goto __ok2_2;
				}
				goto __out2_2;
			}
		      __ok2_2:
			num = make_list_of_deps_for_dep(tempdep, &list);
			for (idx = 0; idx < num; idx++) {
				dep *ldep = list[idx];
				if (!ldep->nummacros)
					continue;
				if (dir && strcmp(dir, ldep->dir))
					continue;
				for (midx = 0; midx < ldep->nummacros; midx++) {
					makefileMacro *macro = find_makefileMacro(ldep->macronames[midx]);
					if (macro == NULL)
						goto __ok2;
					for (vidx = 0; vidx < nummacros; vidx++) {
						if (!(lidx & (1 << vidx)))
							continue;
						if (macro == macros[vidx])
							goto __ok2;
					}
					goto __out2;
				}
			      __ok2:
				if (first) {
					if (mfirst) {
						for (vidx = 0; vidx < nummacros; vidx++) {
							makefileMacro *macro;
							if (!(lidx & (1 << vidx)))
								continue;
							macro = macros[vidx];
							if (macro->header) {
								print_indent(indent);
								printf(macro->header);
							}
							indent += macro->indent;
						}
						mfirst = 0;
					}
					text = convert_to_config_uppercase("CONFIG_", tempdep->name);
					print_indent(indent);
					printf("obj-$(%s) +=", text);
					free(text);
					first = 0;
					if (!tempdep->printflag && indir)
						printf(" %s.o", tempdep->name);
				}
				printf(" %s.o", ldep->name);
			      __out2:
			}
			free(list);
			if (!first)
				printf("\n");
		      __out2_2:
		}
		if (!mfirst) {
			for (vidx = 0; vidx < nummacros; vidx++) {
				makefileMacro *macro;
				if (!(lidx & (1 << vidx)))
					continue;
				macro = macros[vidx];
				indent -= macro->indent;
				if (macro->footer) {	
					print_indent(indent);
					printf(macro->footer);
				}
			}
		}
	}
}

// Output in Makefile.in format
static void output_makefile(const char *dir, int all)
{
	printf("# Module Dependency\n");
	clear_printflags();
	output_makefile1(dir, all);
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

// Main function
int main(int argc, char *argv[])
{
	int method = METHOD_MAKEFILE;
	int argidx = 1, all = 0;
	char *filename, *dir;

	// Find out which method to use
	if (argc < 2)
		usage(argv[0]);
	if (strcmp(argv[argidx], "--makefile") == 0)
		method = METHOD_MAKEFILE;
	else
		usage(argv[0]);
	argidx++;
	
	// Select directory
	if (argc > argidx && strcmp(argv[argidx], "--dir") == 0) {
		if (argc > ++argidx)
			dir = argv[argidx++];
		else
			dir = NULL;
	} else
		dir = NULL;
	
	// Select all dependencies
	if (argc > argidx && strcmp(argv[argidx], "--all") == 0) {
		argidx++;
		all = 1;
	} else
		all = 0;

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
	case METHOD_MAKEFILE:
		output_makefile(dir, all);
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
	exit(EXIT_FAILURE);
}
