/*
 *  Utility to find soundcard dependencies from modules.config
 *  Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no>,
 *		     Martin Dahl <dahlm@vf.telia.no>
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

#ifndef __SND_DEPS_H__
#define __SND_DEPS_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// Defines
#define MODULEDEPFILE "../modules.config" // modules.config filename
#define MODULENAME_MAXLENGTH 30 // Maximum length of module name
#define WARNINGS // Output warnings to stderr

#define METHOD_ACINCLUDE 1 // Output method
#define METHOD_MAKEFILE 2
#define METHOD_CINCLUDE 3
#define METHOD_CONFIGIN 4

#define TYPE_CARDS 1
#define TYPE_DEPS 2

// Typedefs
typedef char depname[MODULENAME_MAXLENGTH];

typedef struct depStruct
{
	depname name;
	char *comment;
	int numdeps;
	struct depStruct **deps;
	struct depStruct *link;
} dep;

// Globals
extern dep *Cards; // All cards
extern dep *Deps; // All other modules

// Prototypes

// snd-deps-output.c
int main(int argc, char *argv[]);
void usage(char *programname);
void output_acinclude(void);
void output_makefile(void);
void output_cinclude(void);
void output_configin(void);
void output_dep(dep *firstdep, char *format, int num);
void output1_dep(dep *firstdep);
void output2_dep(dep *firstdep);
void output_card(dep *firstdep, char *card_format, char *dep_format);
void output1_card(dep *firstdep);
char *convert_to_config_uppercase(const char *line);
char *remove_word(const char *remove, const char *line);

// snd-deps-find.c
int read_file(char *filename);
void add_dep(char *line, dep *firstdep, short type);
dep *alloc_mem_for_dep(dep *firstdep, short type);
char *get_word(char *line, char *word);
dep *find_dep(char *parent, char *depname);
int make_list_of_deps_for_dep(dep *dependency, depname list[], int num);
void del_all_from_list(void);

#endif // __SND_DEPS_H__
