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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MODFILE "../modules.config"

typedef struct depStruct
{
	char name[30];
	struct depStruct *uplink;
	struct depStruct *downlink;
} dep;

typedef struct cardStruct
{
	char name[30];
	char define[30];
	dep *dependencies;
	dep *dependenciesLast;
	struct cardStruct *uplink;
	struct cardStruct *downlink;
} card;

dep *PublicDeps;
dep *PublicDepsLast;

card *Cards;
card *CardsLast;

extern card *add_card(void);
extern dep *add_dep(card *tempcard);
extern dep *add_publicdep(void);
extern card *get_card(char *name);
extern dep *get_dep(card *tempcard, char *name);
extern void del_cards(void);
extern void del_card(card *tempcard);
extern void del_dep(card *tempcard, dep *tempdep);
extern char *get_one_word(char *line, char *buf);
extern void make_acinclude(void);
extern void make_cards_conf_in(void);
extern void add_cards(void);
extern char *convert_config(char *config);
extern int main(int argc, char *argv[]);

card *add_card(void)
{
	if(!Cards)
	{
		Cards=(card *)malloc(sizeof(card));
		Cards->uplink=NULL;
		Cards->downlink=NULL;
		CardsLast=Cards;
		return Cards;
	}
	else
	{
		CardsLast->downlink=(card *)malloc(sizeof(card));
		CardsLast->downlink->uplink=CardsLast;
		CardsLast=CardsLast->downlink;
		CardsLast->downlink=NULL;
		return CardsLast;
	}
}

dep *add_dep(card *tempcard)
{
	if(!tempcard->dependencies)
	{
		tempcard->dependencies=(dep *)malloc(sizeof(dep));
		tempcard->dependencies->uplink=NULL;
		tempcard->dependencies->downlink=NULL;
		tempcard->dependenciesLast=tempcard->dependencies;
		return tempcard->dependencies;
	}
	else
	{
		tempcard->dependenciesLast->downlink=(dep *)malloc(sizeof(dep));
		tempcard->dependenciesLast->downlink->uplink=tempcard->dependenciesLast;
		tempcard->dependenciesLast=tempcard->dependenciesLast->downlink;
		tempcard->dependenciesLast->downlink=NULL;
		return tempcard->dependenciesLast;
	}
}

dep *add_publicdep(void)
{
	if(!PublicDeps)
	{
		PublicDeps=(dep *)malloc(sizeof(dep));
		PublicDeps->uplink=NULL;
		PublicDeps->downlink=NULL;
		PublicDepsLast=PublicDeps;
		return PublicDeps;
	}
	else
	{
		PublicDepsLast->downlink=(dep *)malloc(sizeof(dep));
		PublicDepsLast->downlink->uplink=PublicDepsLast;
		PublicDepsLast=PublicDepsLast->downlink;
		PublicDepsLast->downlink=NULL;
		return PublicDepsLast;
	}
}

card *get_card(char *name)
{
	card *tempcard;
	tempcard=Cards;
	while(tempcard)
	{
		if(!strcmp(tempcard->name,name))
			return tempcard;
		tempcard=tempcard->downlink;
	}
	return NULL;
}

dep *get_dep(card *tempcard, char *name)
{
	dep *tempdep;
	tempdep=tempcard->dependencies;
	while(tempdep)
	{
		if(!strcmp(tempdep->name,name))
			return tempdep;
		tempdep=tempdep->downlink;
	}
	return NULL;
}

void del_cards(void)
{
	card *tempcard;
	dep *tempdep;
	tempcard=Cards;
	while(tempcard)
	{
		tempdep=tempcard->dependencies;
		while(tempdep)
		{
			del_dep(tempcard,tempdep);
			tempdep=tempdep->downlink;
		}
		del_card(tempcard);
		tempcard=tempcard->downlink;
	}
	return;
}

void del_card(card *tempcard)
{
	if(!tempcard->uplink)
		Cards=Cards->downlink;
	else
		tempcard->uplink->downlink=tempcard->downlink;
	if(!tempcard->downlink)
		CardsLast=CardsLast->uplink;
	else
		tempcard->downlink->uplink=tempcard->uplink;
	free((void *)tempcard);
	return;
}

void del_dep(card *tempcard, dep *tempdep)
{
	if(!tempdep->uplink)
		tempcard->dependencies=tempcard->dependencies->downlink;
	else
		tempdep->uplink->downlink=tempdep->downlink;
	if(!tempdep->downlink)
		tempcard->dependenciesLast=tempcard->dependenciesLast->uplink;
	else
		tempdep->downlink->uplink=tempdep->uplink;
	free((void *)tempdep);
	return;
}

void add_to_publicdep(char *currentdep)
{
	dep *pubdep;
	int i;

	pubdep=PublicDeps;
	
	while(pubdep)
	{
		i=strcmp(currentdep, pubdep->name);
		if(i==0)
			return;
		pubdep=pubdep->downlink;
	}

	pubdep=NULL;
	pubdep=add_publicdep();

	strcpy(pubdep->name, currentdep);
	
	return;
}

char *get_one_word(char *line, char *buf)
{
	int flag=0,i;
	buf[0]=0x0;
	for(i=0;;i++)
	{
		if(!line[i])
			return NULL;
		if(line[i]!=0x20)
		{
			if(flag)
				return (char *)&line[i];
			strncat(buf,(char *)&line[i],1);
			continue;
		}
		else
			flag=1;
	}
}

void make_acinclude(void)
{
	card *tempcard;
	dep *tempdep;
	
	tempdep=PublicDeps;
	tempcard=Cards;

	printf("dnl ALSA soundcard-configuration\n");
	printf("dnl Find out which cards to compile driver for\n");
	printf("dnl Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no>\n\n");

	printf("AC_DEFUN(ALSA_CARDS_INIT, [\n");
	while(tempcard)
	{
		printf("\t%s=\"0\"\n", tempcard->define);
		tempcard=tempcard->downlink;
	}
	tempcard=Cards;
	while(tempdep)
	{
		printf("\t%s=\"0\"\n", tempdep->name);
		tempdep=tempdep->downlink;
	}
	tempdep=PublicDeps;
	printf("])\n\n");

	printf("AC_DEFUN(ALSA_CARDS_ALL, [\n");
	while(tempcard)
	{
		printf("\t%s=\"1\"\n", tempcard->define);
		tempcard=tempcard->downlink;
	}
	tempcard=Cards;
	while(tempdep)
	{
		printf("\t%s=\"1\"\n", tempdep->name);
		tempdep=tempdep->downlink;
	}
	tempdep=PublicDeps;
	printf("])\n\n");

	printf("AC_DEFUN(ALSA_CARDS_SELECT, [\n");
	printf("dnl Check for which cards to compile driver for...\n");
	printf("AC_MSG_CHECKING(for which soundcards to compile driver for)\n");
	printf("AC_ARG_WITH(cards,\n");
	printf("  [  --with-cards=<list>     compile driver for cards in <list>. ]\n");
	printf("  [                        cards may be separated with commas. ]\n");
	printf("  [                        \"all\" compiles all drivers ],\n");
	printf("  cards=\"$withval\", cards=\"all\")\n");
	printf("if test \"$cards\" = \"all\"; then\n");
	printf("  ALSA_CARDS_ALL\n");
	printf("  AC_MSG_RESULT(all)\n");
	printf("else\n");
	printf("  cards=`echo $cards | sed 's/,/ /g'`\n");
	printf("  for card in $cards\n  do\n    case \"$card\" in\n");
	
	while(tempcard)
	{
		printf("\t%s)\n\t\t%s=\"1\"\n",tempcard->name, tempcard->define);
		
		tempdep=tempcard->dependencies;
		while(tempdep)
		{
			printf("\t\t%s=\"1\"\n",tempdep->name);
			tempdep=tempdep->downlink;
		}
		tempcard=tempcard->downlink;
		printf("\t\t;;\n");
	}

	printf("\t*)\n\t\techo \"Unknown soundcard $card, exiting!\"\n\t\texit 1\n\t\t;;\n");

	printf("    esac\n  done\n  AC_MSG_RESULT($cards)\nfi\n");

	tempcard=Cards;
	while(tempcard)
	{
		printf("AC_SUBST(%s)\n", tempcard->define);
		tempcard=tempcard->downlink;
	}

	tempdep=PublicDeps;
	while(tempdep)
	{
		printf("AC_SUBST(%s)\n", tempdep->name);
		tempdep=tempdep->downlink;
	}

	printf("])\n");

	return;
}

void make_cards_conf_in(void)
{
	dep *tempdep;
	card *tempcard;

	tempdep=PublicDeps;
	tempcard=Cards;

	printf("#\n# Soundcard-Configuration for ALSA driver\n");
	printf("# Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no>\n#\n\n");
	
	while(tempcard)
	{
		printf("%s=@%s@\n", tempcard->define, tempcard->define);
		tempcard=tempcard->downlink;
	}

	while(tempdep)
	{
		printf("%s=@%s@\n", tempdep->name, tempdep->name);
		tempdep=tempdep->downlink;
	}
}

void add_cards(void)
{
	char buffer[255],*line,arg[255];
	FILE *modfile;
	card *tempcard;
	dep *tempdep;
	int i;

	if((modfile=fopen(MODFILE,"r"))==NULL)
	{
		printf("Could not open %s for read\n",MODFILE);
		exit(-1);
	}
	while(!feof(modfile))
	{
		bzero(buffer,255);
		fscanf(modfile,"%[^\n]\n",buffer);
		if(buffer[0]=='|')
		{
			line=&buffer[1];
			line=get_one_word(line,(char *)arg);
			tempcard=add_card();
			strcpy(tempcard->name,(char *)arg);
			for(i=0;i<strlen(tempcard->name)-3;i++)
				tempcard->name[i]=tempcard->name[i+4];
			strcpy(tempcard->define, convert_config((char *)arg));

			for(;;)
			{
				line=get_one_word(line,(char *)arg);
				if(!line)
				{
					tempdep=add_dep(tempcard);
					strcpy(tempdep->name,(char *)arg);
					convert_config(tempdep->name);
					add_to_publicdep(tempdep->name);
					break;
				}
				tempdep=add_dep(tempcard);
				strcpy(tempdep->name,(char *)arg);
				convert_config(tempdep->name);
				add_to_publicdep(tempdep->name);
			}
		}
	}
	fclose(modfile);
	return;
}

char *convert_config(char *config)
{
	char retvalue[30];
	int i;

	for(i=0;i<strlen(config)-3;i++)
		config[i]=config[i+4];
	for(i=0;i<strlen(config);i++)
		if(config[i]=='-')
			config[i]='_';
		else
			config[i]=toupper(config[i]);

	strcpy(retvalue, config);
	strcpy(config, "CONFIG_SND_");
	strcat(config, retvalue);

	return config;
}

void usage(char *programname)
{
	fprintf(stderr, "Usage: %s --acinclude\n       %s --cards-conf\n", programname, programname);
	exit(1);
}

int main(int argc, char *argv[])
{
	add_cards();
	if(argc<2)
		usage(argv[0]);
	if(strcmp(argv[1], "--acinclude")==0)
		make_acinclude();
	else if(strcmp(argv[1], "--cards-conf")==0)
		make_cards_conf_in();
	else
		usage(argv[0]);
	del_cards();
	return 0;
}
