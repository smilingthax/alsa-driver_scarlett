/*
 *  Utility to output soundcard dependencies in different formats
 *  Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no>,
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

#include "snd-deps.h"

// Main function
int main(int argc, char *argv[])
{
	int method = METHOD_ACINCLUDE;

	// Find out which method to use
	if(argc<2)
		usage(argv[0]);
	if(strcmp(argv[1], "--acinclude")==0)
		method=METHOD_ACINCLUDE;
	else if(strcmp(argv[1], "--makefile")==0)
		method=METHOD_MAKEFILE;
	else if(strcmp(argv[1], "--cinclude")==0)
		method=METHOD_CINCLUDE;
	else
		usage(argv[0]);

	// Read the file into memory
	if(read_file(MODULEDEPFILE)<0)
	{
		perror("Error reading modules.config file");
		exit(EXIT_FAILURE);
	}

	// Use method
	switch(method)
	{
		case METHOD_ACINCLUDE:
			output_acinclude();
			break;
		case METHOD_MAKEFILE:
			output_makefile();
			break;
		case METHOD_CINCLUDE:
			output_cinclude();
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
void usage(char *programname)
{
	fprintf(stderr, "Usage: %s --acinclude\n", programname);
	fprintf(stderr, "       %s --makefile\n", programname);
	fprintf(stderr, "       %s --cinclude\n", programname);
	exit(EXIT_FAILURE);
}

// Output in format used by acinclude.m4
void output_acinclude(void)
{
	printf("\
dnl ALSA soundcard-configuration\n\
dnl Find out which cards to compile driver for\n\
dnl Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no>\n\
\n\
AC_DEFUN(ALSA_CARDS_INIT, [\n\
");
	output_dep(Deps, "\t%s=\"0\"\n", 1);
	output_dep(Cards, "\t%s=\"0\"\n", 1);
	printf("\
])\n\
\n\
AC_DEFUN(ALSA_CARDS_ALL, [\n\
");
	output_dep(Deps, "\t%s=\"1\"\n\tAC_DEFINE(%s)\n", 2);
	output_dep(Cards, "\t%s=\"1\"\n\tAC_DEFINE(%s)\n", 2);
	printf("\
])\n\
\n\
AC_DEFUN(ALSA_CARDS_SELECT, [\n\
dnl Check for which cards to compile driver for...\n\
AC_MSG_CHECKING(for which soundcards to compile driver for)\n\
AC_ARG_WITH(cards,\n\
  [  --with-cards=<list>     compile driver for cards in <list>. ]\n\
  [                        cards may be separated with commas. ]\n\
  [                        \"all\" compiles all drivers ],\n\
  cards=\"$withval\", cards=\"all\")\n\
if test \"$cards\" = \"all\"; then\n\
  ALSA_CARDS_ALL\n\
  AC_MSG_RESULT(all)\n\
else\n\
  cards=`echo $cards | sed 's/,/ /g'`\n\
  for card in $cards\n\
  do\n\
    case \"$card\" in\n\
");
	output_card(Cards, "\t%s)\n\t\t%s=\"1\"\n\t\tAC_DEFINE(%s)\n", "\t\t%s=\"1\"\n\t\tAC_DEFINE(%s)\n");
	printf("\
\t*)\n\
\t\techo \"Unknown soundcard $card, exiting!\"\n\
\t\texit 1\n\
\t\t;;\n\
");
        printf("\
    esac\n\
  done\n\
  AC_MSG_RESULT($cards)\n\
fi\n\
");
	output_dep(Deps, "AC_SUBST(%s)\n", 1);
	output_dep(Cards, "AC_SUBST(%s)\n", 1);
	printf("\
])\n\
");
	return;
}

// Output in Makefile.in format
void output_makefile(void)
{
	printf("\
\n\
# Soundcard-Configuration for ALSA driver\n\
# Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no>\n\
\n\
");
	output_dep(Deps, "%s=@%s@\n", 2);
	output_dep(Cards, "%s=@%s@\n", 2);
	return;
}

// Output in c format
void output_cinclude(void)
{
	printf("\
\n\
/* Soundcard-Configuration for ALSA driver */\n\
/* Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no> */\n\
\n\
");
	output_dep(Deps, "#undef %s\n", 1);
	output_dep(Cards, "#undef %s\n", 1);
	return;
}

// Print out all deps for firstdep (Cards, Deps) according to format
void output_dep(dep *firstdep, char *format, int num)
{
	dep *tempdep=firstdep;
	char *text;

	while(tempdep)
	{
		text=convert_to_config_uppercase(tempdep->name);
		if(num==1)
			printf(format, text);
		else if(num==2)
			printf(format, text, text);
		free(text);
		tempdep=tempdep->link;
	}
	return;
}

// Print out ALL deps for firstdep (Cards, Deps)
void output_card(dep *firstdep, char *card_format, char *dep_format)
{
	depname list[200];
	dep *temp_dep=firstdep;
	int num,i;
	char *card_name;
	char *card_config;
	char *dep_config;
	
	while(temp_dep)
	{
		card_name=remove_word("snd_", temp_dep->name);
		card_config=convert_to_config_uppercase(temp_dep->name);
		printf(card_format, card_name, card_config, card_config);
		num=make_list_of_deps_for_dep(temp_dep, list, 0);
		for(i=0;i<num;i++)
		{
			dep_config=convert_to_config_uppercase(list[i]);
			printf(dep_format, dep_config, dep_config);
			free(dep_config);
		}
		printf("\t\t;;\n");
		free(card_name);
		free(card_config);
		temp_dep=temp_dep->link;
	}
	return;
}

// example: snd-sb16 -> CONFIG_SND_SB16
char *convert_to_config_uppercase(const char *line)
{
	char pre[]="CONFIG_";
	char *holder;
	int i;

	holder=malloc(strlen(line)+strlen(pre)+1);
	if(holder==NULL)
	{
		fprintf(stderr, "Not enough memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(holder, pre);
	for(i=0;i<strlen(line);i++)
		switch(line[i])
		{
			case '-':
				holder[i+strlen(pre)]='_';
				break;
			default:
				holder[i+strlen(pre)]=toupper(line[i]);
				break;
		}

	holder[i+strlen(pre)]='\0';
	
	return holder;
}

// example: snd-sb16 -> sb16
char *remove_word(const char *remove, const char *line)
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
