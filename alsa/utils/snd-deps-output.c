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
	else if(strcmp(argv[1], "--configin")==0)
		method=METHOD_CONFIGIN;
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
		case METHOD_CONFIGIN:
			output_configin();
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
	fprintf(stderr, "       %s --configin\n", programname);
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
  [  --with-cards=<list>     compile driver for cards in <list>; ]\n\
  [                        cards may be separated with commas; ]\n\
  [                        'all' compiles all drivers; ]\n\
  [                        Possible cards are: ]\n");
	output_card_list(Cards, 24, 50);
	printf(" ],\n\
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

// Output in Config.in format
void output_configin(void)
{
	printf("\
# ALSA soundcard-configuration\n\
\n\
if [ \"x$CONFIG_SND_SEQUENCER\" == \"x\" ]; then\n\
  CONFIG_SND_SEQUENCER=\"n\"\n\
fi\n\
if [ \"x$CONFIG_SND_OSSEMUL\" == \"x\" ]; then\n\
  CONFIG_SND_OSSEMUL=\"n\"\n\
fi\n\
if [ \"x$CONFIG_SND_MIXER_OSS\" == \"x\" ]; then\n\
  CONFIG_SND_MIXER_OSS=\"n\"\n\
fi\n\
if [ \"x$CONFIG_SND_PCM_OSS\" == \"x\" ]; then\n\
  CONFIG_SND_PCM_OSS=\"n\"\n\
fi\n\
if [ \"x$CONFIG_SND_SEQUENCER_OSS\" == \"x\" ]; then\n\
  CONFIG_SND_SEQUENCER_OSS=\"n\"\n\
fi\n\
if [ \"$CONFIG_SND\" != \"n\" ]; then\n\
  dep_tristate 'Sequencer support' CONFIG_SND_SEQUENCER $CONFIG_SND\n\
  bool 'OSS API emulation' CONFIG_SND_OSSEMUL\n\
  if [ \"$CONFIG_SND_OSSEMUL\" = \"y\" ]; then\n\
    dep_tristate 'OSS Mixer API' CONFIG_SND_MIXER_OSS $CONFIG_SND\n\
    dep_tristate 'OSS PCM API' CONFIG_SND_PCM_OSS $CONFIG_SND\n\
    if [ \"$CONFIG_SND_SEQUENCER\" != \"n\" ]; then\n\
      dep_tristate 'OSS Sequencer API' CONFIG_SND_SEQUENCER_OSS $CONFIG_SND_SEQUENCER\n\
    fi\n\
  fi\n\
  bool 'Debug' CONFIG_SND_DEBUG\n\
  if [ \"$CONFIG_SND_DEBUG\" = \"y\" ]; then\n\
    bool 'Debug memory' CONFIG_SND_DEBUG_MEMORY\n\
    bool 'Debug full' CONFIG_SND_DEBUG_FULL\n\
    bool 'Debug detection' CONFIG_SND_DEBUG_DETECT\n\
  fi\n\
fi\n\n");
	output1_dep(Deps);
printf("\
XX_CONFIG_SND_SEQ_DEVICE=$CONFIG_SND_SEQUENCER\n\
XX_CONFIG_SND_SEQ=$CONFIG_SND_SEQUENCER\n\
XX_CONFIG_SND_TIMER=$CONFIG_SND_SEQUENCER\n\
XX_CONFIG_SND_PCM=$CONFIG_SND_PCM_OSS\n\
XX_CONFIG_SND_SEQ_MIDI_EVENT=$CONFIG_SND_SEQUENCER_OSS\n\
");
	output1_card(Cards);
	output2_dep(Deps);
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

// Print out all deps for firstdep (Cards, Deps) according to format
void output1_dep(dep *firstdep)
{
	dep *tempdep=firstdep;
	char *text;

	while(tempdep)
	{
		text=convert_to_config_uppercase(tempdep->name);
		if (strcmp(text, "CONFIG_SND"))
			printf("XX_%s=\"n\"\n", text);
		free(text);
		tempdep=tempdep->link;
	}
	return;
}

// Print out all deps for firstdep (Cards, Deps) according to format
void output2_dep(dep *firstdep)
{
	dep *tempdep=firstdep;
	char *text;

	while(tempdep)
	{
		text=convert_to_config_uppercase(tempdep->name);
		if (strcmp(text, "CONFIG_SND"))
			printf("define_bool %s $XX_%s\n", text, text);
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
		card_name=get_card_name(temp_dep->name);
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

// Print out ALL deps for firstdep (Cards, Deps)
void output_card_list(dep *firstdep, int space, int size)
{
	dep *temp_dep=firstdep;
	char *card_name;
	int tmp_size = 0, idx;

	printf("  [");
	for (idx = 0; idx < space; idx++)
		printf(" ");
	while(temp_dep)
	{
		card_name=get_card_name(temp_dep->name);
		if (temp_dep != firstdep) {
			printf(", ");
			tmp_size += 2;
		}
		if (tmp_size + strlen(card_name) + 2 > size) {
			printf("]\n  [");
			for (idx = 0; idx < space; idx++)
				printf(" ");
			tmp_size = 0;
		}
		printf(card_name);
		tmp_size += strlen(card_name);
		temp_dep=temp_dep->link;
	}
	return;
}

// Print out ALL deps for firstdep (Cards, Deps)
void output1_card(dep *firstdep)
{
	depname list[200];
	dep *temp_dep=firstdep;
	int num,i;
	char *card_name;
	char *card_config;
	char *card_comment;
	char *dep_config;
	
	while(temp_dep)
	{
		card_name=get_card_name(temp_dep->name);
		card_config=convert_to_config_uppercase(temp_dep->name);
		card_comment=convert_to_escape(temp_dep->comment);
		printf("\
dep_tristate '%s' %s $CONFIG_SND\n\
if [ \"$%s\" != \"n\" ]; then\n\
", card_comment, card_config, card_config);
		num=make_list_of_deps_for_dep(temp_dep, list, 0);
		for(i=0;i<num;i++)
		{
			dep_config=convert_to_config_uppercase(list[i]);
			if (strcmp(dep_config, "CONFIG_SND"))
				printf("\
  if [ \"$XX_%s\" != \"n\" ]; then\n\
    if [ \"$XX_%s\" = \"m\" -a \"$%s\" = \"y\" ]; then\n\
      XX_%s=\"y\"\n\
    fi\n\
  else\n\
    XX_%s=$%s\n\
  fi\n\
", dep_config, dep_config, card_config, dep_config, dep_config, card_config);
			free(dep_config);
		}
		printf("fi\n");
		free(card_comment);
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
	char *holder, *p;
	int i;

	holder=malloc(strlen(line)*2+strlen(pre)+1);
	if(holder==NULL)
	{
		fprintf(stderr, "Not enough memory\n");
		exit(EXIT_FAILURE);
	}
	p = strcpy(holder, pre) + strlen(pre);
	for(i=0;i<strlen(line);i++)
		switch(line[i])
		{
			case '-':
				*p++='_';
				break;
			default:
				*p++=toupper(line[i]);
				break;
		}

	*p++='\0';
	
	return holder;
}

// example: a'b -> a\'b
char *convert_to_escape(const char *line)
{
	char *holder, *p;
	int i;

	holder=malloc(strlen(line)+1);
	if(holder==NULL)
	{
		fprintf(stderr, "Not enough memory\n");
		exit(EXIT_FAILURE);
	}
	p = holder;
	for(i=0;i<strlen(line);i++)
		switch(line[i])
		{
			case '\'':
				*p++='`';
				break;
			default:
				*p++=line[i];
				break;
		}

	*p++='\0';
	
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

// example: card-sb16 -> sb16
char *get_card_name(const char *line)
{
	if (strncmp(line, "snd-card-", 9)) {
		fprintf(stderr, "Invalid card name '%s'\n", line);
		exit(EXIT_FAILURE);
	}
	return remove_word("snd-card-", line);
}
