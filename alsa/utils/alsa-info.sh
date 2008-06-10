#!/bin/bash

SCRIPT_VERSION=0.4.47
CHANGELOG="http://www.alsa-project.org/alsa-info.sh"

#################################################################################
#Copyright (C) 2007 Free Software Foundation.

#This program is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#(at your option) any later version.

#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with this program; if not, write to the Free Software
#Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

##################################################################################

#The script was written for 2 main reasons:
# 1. Remove the need for the devs/helpers to ask several questions before we can easily help the user.
# 2. Allow newer/inexperienced ALSA users to give us all the info we need to help them.

#Change the PATH variable, so we can run lspci (needed for some distros)
PATH=$PATH:/bin:/sbin:/usr/bin:/usr/sbin
BGTITLE="ALSA-Info v $SCRIPT_VERSION"
PASTEBINKEY="C9cRIO8m/9y8Cs0nVs0FraRx7U0pHsuc"
#Define some simple functions

pbcheck(){
	[[ $(ping -c1 www.pastebin.ca) ]] || KEEP_FILES="yes" NOUPLOAD="yes" PBERROR="yes"
}

update() {
	wget -O /tmp/alsa-info.sh "http://git.alsa-project.org/?p=alsa-driver.git;a=blob_plain;f=utils/alsa-info.sh" >/dev/null 2>&1
	REMOTE_VERSION=`grep SCRIPT_VERSION /tmp/alsa-info.sh |head -n1 |sed 's/.*=//'`
	if [ "$REMOTE_VERSION" != "$SCRIPT_VERSION" ]; then
		if [[ -n $DIALOG ]]
		then
			dialog --yesno "Newer version of ALSA-Info has been found\n\nDo you wish to install it?" 0 0
			DIALOG_EXIT_CODE=$?
			if [[ $DIALOG_EXIT_CODE = 0 ]]
			then
				cp /tmp/alsa-info.sh $0
				echo "ALSA-Info script has been updated to v $REMOTE_VERSION"
				echo "To view the ChangeLog, please visit $CHANGELOG"
				echo "Please re-run the script"
				exit
			fi
		else
			cp /tmp/alsa-info.sh $0
			echo "Newer version detected: $REMOTE_VERSION"
			echo "To view the ChangeLog, please visit $CHANGELOG"
			echo "ALSA-Info script has been updated. Please re-run it."

			exit
		fi
	fi
	rm /tmp/alsa-info.sh 2>/dev/null
}

cleanup() {
	rm -r $TEMPDIR 2>/dev/null
}


#### FIX ME
withsecure() {
	POST_URL="http://alsa-info.pastebin.ca"
}
###########

withaplay() {
        echo "!!Aplay/Arecord output" >> $FILE
        echo "!!------------" >> $FILE
        echo "" >> $FILE
       	echo "APLAY" >> $FILE
	echo "" >> $FILE 
	aplay -l >> $FILE 2>&1
        echo "" >> $FILE
       	echo "ARECORD" >> $FILE
	echo "" >> $FILE
	arecord -l >> $FILE 2>&1
	echo "" >> $FILE
}

withlsmod() {
	echo "!!All Loaded Modules" >> $FILE
	echo "!!------------------" >> $FILE
	echo "" >> $FILE
	lsmod |awk {'print $1'} >> $FILE
	echo "" >> $FILE
	echo "" >> $FILE
}

withamixer() {
        echo "!!Amixer output" >> $FILE
        echo "!!-------------" >> $FILE
        echo "" >> $FILE
	for i in `grep "]: " /proc/asound/cards | awk -F ' ' '{ print $1} '` ; do
	CARD_NAME=`grep "^ *$i " /tmp/alsainfo/alsacards.tmp|awk {'print $2'}`
	echo "!!-------Mixer controls for card $i $CARD_NAME]" >> $FILE
	echo "" >>$FILE
	amixer -c$i>> $FILE 2>&1
        echo "" >> $FILE
	done
	echo "" >> $FILE
}

withalsactl() {
	echo "!!Alsactl output" >> $FILE
        echo "!!-------------" >> $FILE
        echo "" >> $FILE
        exe=""
        if [ -x /usr/sbin/alsactl ]; then
        	exe="/usr/sbin/alsactl"
        fi
        if [ -x /usr/local/sbin/alsactl ]; then
        	exe="/usr/local/sbin/alsactl"
        fi
        if [ -z "$exe" ]; then
        	exe=`whereis alsactl | cut -d ' ' -f 2`
        fi
	$exe -f /tmp/alsainfo/alsactl.tmp store
	echo "--startcollapse--" >> $FILE
	cat /tmp/alsainfo/alsactl.tmp >> $FILE
	echo "--endcollapse--" >> $FILE
	echo "" >> $FILE
	echo "" >> $FILE
}

withdevices() {
        echo "!!ALSA Device nodes" >> $FILE
        echo "!!-----------------" >> $FILE
        echo "" >> $FILE
        ls -la /dev/snd/* >> $FILE
        echo "" >> $FILE
        echo "" >> $FILE
}

withconfigs() {
if [[ -e $HOME/.asoundrc ]] || [[ -e /etc/asound.conf ]] || [[ -e $HOME/.asoundrc.asoundconf ]]
then
        echo "!!ALSA configuration files" >> $FILE
        echo "!!------------------------" >> $FILE
        echo "" >> $FILE

        #Check for ~/.asoundrc
        if [[ -e $HOME/.asoundrc ]]
        then
                echo "!!User specific config file (~/.asoundrc)" >> $FILE
                echo "" >> $FILE
                cat $HOME/.asoundrc >> $FILE
                echo "" >> $FILE
                echo "" >> $FILE
        fi
	#Check for .asoundrc.asoundconf (seems to be Ubuntu specific)
	if [[ -e $HOME/.asoundrc.asoundconf ]]
	then
		echo "!!asoundconf-generated config file" >> $FILE
		echo "" >> $FILE
		cat $HOME/.asoundrc.asoundconf >> $FILE
		echo "" >> $FILE
		echo "" >> $FILE
	fi
        #Check for /etc/asound.conf
        if [[ -e /etc/asound.conf ]]
        then
                echo "!!System wide config file (/etc/asound.conf)" >> $FILE
                echo "" >> $FILE
                cat /etc/asound.conf >> $FILE
                echo "" >> $FILE
                echo "" >> $FILE
        fi
fi
}


#Run checks to make sure the programs we need are installed.
LSPCI=$(which lspci 2>/dev/null| sed 's|^[^/]*||' 2>/dev/null);
TPUT=$(which tput 2>/dev/null| sed 's|^[^/]*||' 2>/dev/null);
DIALOG=$(which dialog 2>/dev/null | sed 's|^[^/]*||' 2>/dev/null);

#Check to see if sysfs is enabled in the kernel. We'll need this later on
SYSFS=$(mount |grep sysfs|awk {'print $3'});

#Check modprobe config files for sound related options
SNDOPTIONS=$(modprobe -c|sed -n 's/^options \(snd[-_][^ ]*\)/\1:/p')

QUESTION="yes"
PROCEED="yes"
REPEAT=""
while [ -z "$REPEAT" ]; do
REPEAT="no"
case "$1" in
	--update|--help|--about)
		QUESTION="no"
		PROCEED="no"
		;;
	--no-upload)
		NOUPLOAD="yes"
		;;
	--no-dialog)
		DIALOG=""
		REPEAT=""
		shift
		;;
esac
done
		

#Script header output.
if [ "$QUESTION" = "yes" ]; then
if [[ -n "$DIALOG" ]]
then
if [ -z "$NOUPLOAD" ]; then
	dialog --backtitle "$BGTITLE" --title "ALSA-Info script v $SCRIPT_VERSION" --yesno "\nThis script will collect information about your ALSA installation and sound related hardware, to help diagnose your problem\n\nBy default, this script will AUTOMATICALLY UPLOAD your information to a pastebin site.\n\nSee $0 --help for options\n\nDo you want to run this script?" 0 0
else
	dialog --backtitle "$BGTITLE" --title "ALSA-Info script v $SCRIPT_VERSION" --yesno "\nThis script will collect information about your ALSA installation and sound related hardware, to help diagnose your problem\n\nSee $0 --help for options\n\nDo you want to run this script?" 0 0
fi
DIALOG_EXIT_CODE=$?
if [ $DIALOG_EXIT_CODE != 0 ]; then
echo "Thank you for using the ALSA-Info Script"
exit 0;
fi
else

echo "ALSA Information Script v $SCRIPT_VERSION"
echo "--------------------------------"
echo ""
echo "This script will collect information about your ALSA installation and sound related hardware, to help diagnose your problem."
echo ""
if [ -z "$NOUPLOAD" ]; then
if [[ -n "$TPUT" ]]; then
tput bold
echo "By default, the collected information will be AUTOMATICALLY uploaded to a pastebin site."
echo "If you do not wish for this to occur, run the script with the --no-upload argument"
tput sgr0
else
echo "By default, the collected information will be AUTOMATICALLY uploaded to a pastebin site."
echo "If you do not wish for this to occur, run the script with the --no-upload argument"
fi
echo ""
fi # NOUPLOAD
echo -n "Do you want to run this script? [y/n] : "
read -e CONFIRM
if [ "$CONFIRM" != "y" ]; then
echo ""
echo "Thank you for using the ALSA-Info Script"
exit 0;
fi
fi
fi # question

#Set the output file
TEMPDIR="/tmp/alsainfo/"
FILE="/tmp/alsa-info.txt"

if [ "$PROCEED" = "yes" ]; then

if [[ -z "$LSPCI" ]] 
	then
	echo "This script requires lspci. Please install it, and re-run this script."
exit 0
fi

#Create the temporary work dir.
mkdir $TEMPDIR 2>/dev/null

#Fetch the info and store in temp files/variables
DISTRO=`grep -ihs "buntu\|SUSE\|Fedora\|PCLinuxOS\|MEPIS\|Mandriva\|Debian\|Damn\|Sabayon\|Slackware\|KNOPPIX\|Gentoo\|Zenwalk\|Mint\|Kubuntu\|FreeBSD\|Puppy\|Freespire\|Vector\|Dreamlinux\|CentOS\|Arch\|Xandros\|Elive\|SLAX\|Red\|BSD\|KANOTIX\|Nexenta\|Foresight\|GeeXboX\|Frugalware\|64\|SystemRescue\|Novell\|Solaris\|BackTrack\|KateOS\|Pardus" /etc/{issue,*release,*version}`
KERNEL_VERSION=`uname -r`
KERNEL_PROCESSOR=`uname -p`
KERNEL_MACHINE=`uname -m`
KERNEL_OS=`uname -o`
[[ `uname -v |grep SMP`  ]] && KERNEL_SMP="Yes" || KERNEL_SMP="No" 
ALSA_DRIVER_VERSION=`cat /proc/asound/version |head -n1|awk {'print $7'} |sed 's/\.$//'`
ALSA_LIB_VERSION=`grep VERSION_STR /usr/include/alsa/version.h 2>/dev/null|awk {'print $3'}|sed 's/"//g'`
ALSA_UTILS_VERSION=`amixer -v |awk {'print $3'}`
VENDOR_ID=`lspci -vn |grep 040[1-3] | awk -F':' '{print $3}'|awk {'print substr($0, 2);}' >/tmp/alsainfo/vendor_id.tmp`
DEVICE_ID=`lspci -vn |grep 040[1-3] | awk -F':' '{print $4}'|awk {'print $1'} >/tmp/alsainfo/device_id.tmp`
LAST_CARD=$((`grep "]: " /proc/asound/cards | wc -l` - 1 ))
cat /proc/asound/modules 2>/dev/null|awk {'print $2'}>/tmp/alsainfo/alsamodules.tmp
cat /proc/asound/cards >/tmp/alsainfo/alsacards.tmp
lspci |grep -i "multi\|audio">/tmp/alsainfo/lspci.tmp

#Check for HDA-Intel cards codec#*
cat /proc/asound/card*/codec\#* > /tmp/alsainfo/alsa-hda-intel.tmp 2> /dev/null

#Check for AC97 cards codec
cat /proc/asound/card*/codec97\#0/ac97\#0-0 > /tmp/alsainfo/alsa-ac97.tmp 2> /dev/null
cat /proc/asound/card*/codec97\#0/ac97\#0-0+regs > /tmp/alsainfo/alsa-ac97-regs.tmp 2> /dev/null

#Fetch the info, and put it in $FILE in a nice readable format.
echo "name=$USER&type=33&description=/tmp/alsa-info.txt&expiry=&s=Submit+Post&content=" > $FILE
echo "!!################################" >> $FILE
echo "!!ALSA Information Script v $SCRIPT_VERSION" >> $FILE
echo "!!################################" >> $FILE
echo "" >> $FILE
echo "!!Script ran on: `LANG=C date`" >> $FILE
echo "" >> $FILE
echo "" >> $FILE
echo "!!Linux Distribution" >> $FILE
echo "!!------------------" >> $FILE
echo "" >> $FILE
echo $DISTRO >> $FILE
echo "" >> $FILE
echo "" >> $FILE
echo "!!Kernel Information" >> $FILE
echo "!!------------------" >> $FILE
echo "" >> $FILE
echo "Kernel release:    $KERNEL_VERSION" >> $FILE
echo "Operating System:  $KERNEL_OS" >> $FILE
echo "Architecture:      $KERNEL_MACHINE" >> $FILE
echo "Processor:         $KERNEL_PROCESSOR" >> $FILE
echo "SMP Enabled:       $KERNEL_SMP" >> $FILE
echo "" >> $FILE
echo "" >> $FILE
echo "!!ALSA Version" >> $FILE
echo "!!------------" >> $FILE
echo "" >> $FILE
echo "Driver version:     $ALSA_DRIVER_VERSION" >> $FILE
echo "Library version:    $ALSA_LIB_VERSION" >> $FILE
echo "Utilities version:  $ALSA_UTILS_VERSION" >> $FILE
echo "" >> $FILE
echo "" >> $FILE
echo "!!Loaded ALSA modules" >> $FILE
echo "!!-------------------" >> $FILE
echo "" >> $FILE
cat /tmp/alsainfo/alsamodules.tmp >> $FILE
echo "" >> $FILE
echo "" >> $FILE
echo "!!Soundcards recognised by ALSA" >> $FILE
echo "!!-----------------------------" >> $FILE
echo "" >> $FILE
cat /tmp/alsainfo/alsacards.tmp >> $FILE
echo "" >> $FILE
echo "" >> $FILE
echo "!!PCI Soundcards installed in the system" >> $FILE
echo "!!--------------------------------------" >> $FILE
echo "" >> $FILE
cat /tmp/alsainfo/lspci.tmp >> $FILE
echo "" >> $FILE
echo "" >> $FILE
echo "!!Advanced information - PCI Vendor/Device/Susbsystem ID's" >> $FILE
echo "!!--------------------------------------------------------" >> $FILE
echo "" >> $FILE
lspci -vvn |grep -A1 040[1-3] >> $FILE
echo "" >> $FILE
echo "" >> $FILE

if [ "$SNDOPTIONS" ]
then
echo "!!Modprobe options (Sound related)" >> $FILE
echo "!!--------------------------------" >> $FILE
echo "" >> $FILE
modprobe -c|sed -n 's/^options \(snd[-_][^ ]*\)/\1:/p' >> $FILE
echo "" >> $FILE
echo "" >> $FILE
fi

if [ -d $SYSFS ]
then
echo "!!Loaded sound module options" >> $FILE
echo "!!--------------------------" >> $FILE
echo "" >> $FILE
for mod in `cat /proc/asound/modules|awk {'print $2'}`;do
echo "!!Module: $mod" >> $FILE
for params in `ls $SYSFS/module/$mod/parameters/*`; do /bin/echo -ne "\t";/bin/echo "$params : `cat $params`"|sed 's:.*/::' >> $FILE;done
echo "" >> $FILE
done
echo "" >> $FILE
fi

if [ -s "/tmp/alsainfo/alsa-hda-intel.tmp" ] 
then
	echo "!!HDA-Intel Codec information" >> $FILE
	echo "!!---------------------------" >> $FILE
	echo "--startcollapse--" >> $FILE
	echo "" >> $FILE
	cat /tmp/alsainfo/alsa-hda-intel.tmp >> $FILE
	echo "--endcollapse--" >> $FILE
	echo "" >> $FILE
	echo "" >> $FILE
fi

if [ -s "/tmp/alsainfo/alsa-ac97.tmp" ]
then
        echo "!!AC97 Codec information" >> $FILE
        echo "!!---------------------------" >> $FILE
        echo "--startcollapse--" >> $FILE
        echo "" >> $FILE
        cat /tmp/alsainfo/alsa-ac97.tmp >> $FILE
        echo "" >> $FILE
        cat /tmp/alsainfo/alsa-ac97-regs.tmp >> $FILE
        echo "--endcollapse--" >> $FILE
	echo "" >> $FILE
	echo "" >> $FILE
fi


#If no command line options are specified, then run as though --with-all was specified
if [[ -z "$1" ]]
then
	update
	withdevices
	withconfigs
	withaplay
	withamixer
	withalsactl
	withlsmod
	pbcheck	
fi

fi # proceed

#loop through command line arguments, until none are left.
if [[ -n "$1" ]]
then
	until [ -z "$1" ]
	do
	case "$1" in
		--update)
			update
			exit
			;;
		--no-upload)
			NOUPLOAD="yes"
			KEEP_FILES="yes"
			withdevices
			withconfigs
			withaplay
			withamixer
			withalsactl
			withlsmod
			;;
		--debug)
			echo "Debugging enabled. $FILE and $TEMPDIR will not be deleted"
			KEEP_FILES="yes"
			echo ""
			withdevices
			withconfigs
			withaplay
			withamixer
			withalsactl
			withlsmod
			;;
		--with-all)
			withdevices
			withconfigs
			withaplay
			withamixer
			withalsactl
			withlsmod
			;;
		--with-aplay)
			withaplay
			;;
		--with-amixer)
			withamixer
			;;
		--with-alsactl)
			withalsactl
			;;
		--with-devices)
			withdevices
			;;
		--with-configs)
			if [[ -e $HOME/.asoundrc ]] || [[ -e /etc/asound.conf ]]
			then
				echo "!!ALSA configuration files" >> $FILE
				echo "!!------------------------" >> $FILE
				echo "" >> $FILE

				#Check for ~/.asoundrc
				if [[ -e $HOME/.asoundrc ]]
				then
					echo "!!User specific config file ($HOME/.asoundrc)" >> $FILE
					echo "" >> $FILE
					cat $HOME/.asoundrc >> $FILE
					echo "" >> $FILE
					echo "" >> $FILE
				fi

				#Check for /etc/asound.conf
				if [[ -e /etc/asound.conf ]]
				then
					echo "!!System wide config file (/etc/asound.conf)" >> $FILE
					echo "" >> $FILE
					cat /etc/asound.conf >> $FILE
					echo "" >> $FILE
					echo "" >> $FILE
				fi
			fi
			;;
		--about)
			echo "Written/Tested by the following users of #alsa on irc.freenode.net:"
			echo ""
			echo "	wishie - Script author and developer / Testing"
			echo "	crimsun - Various script ideas / Testing"
			echo "	gnubien - Various script ideas / Testing"
			echo "	GrueMaster - HDA Intel specific items / Testing"
			echo "	olegfink - Script update function"
			cleanup
			exit 0
			;;
		*)
			echo ""
			echo "Available options:"
			echo "	--with-aplay (includes the output of aplay -l)"
			echo "	--with-amixer (includes the output of amixer)"
			echo "	--with-alsactl (includes the output of alsactl)"
			echo "	--with-configs (includes the output of ~/.asoundrc and /etc/asound.conf if they exist)" 
			echo "	--with-devices (shows the device nodes in /dev/snd/)"
			echo ""
			echo "	--update (check server for script updates)"
			echo "	--no-upload (do not upload contents to remote server)"
			echo "	--about (show some information about the script)"
			echo "	--debug (will run the script as normal, but will not delete $FILE)"
			cleanup
			exit 0
			;;
	esac
	shift 1
	done
fi

if [ "$PROCEED" = "yes" ]; then

#Test that wget is installed, and supports --post-file. Upload $FILE if it does, and prompt user to upload file if it doesnt. 
if
WGET=$(which wget 2>/dev/null| sed 's|^[^/]*||' 2>/dev/null); [[ -n "${WGET}" ]] && [[ -x "${WGET}" ]] && [[ `wget --help |grep post-file` ]]
then
if [[ -n "$DIALOG" ]]
then
	if [[ -z $NOUPLOAD ]]; then
	wget -O - --tries=5 --timeout=60 --post-file=/tmp/alsa-info.txt "http://pastebin.ca/quiet-paste.php?api=$PASTEBINKEY&encrypt=t&encryptpw=blahblah" &>/tmp/alsainfo/wget.tmp || echo "Upload failed; exit"
	{ for i in 10 20 30 40 50 60 70 80 90; do
		echo $i
		sleep 0.2
	done
	echo; } |dialog --backtitle "$BGTITLE" --guage "Uploading information to www.pastebin.ca ..." 6 70 0
	fi
else

	if [[ -z $NOUPLOAD ]]; then
	echo -n "Uploading information to www.pastebin.ca ... " 
	wget -O - --tries=5 --timeout=60 --post-file=/tmp/alsa-info.txt http://pastebin.ca/quiet-paste.php?api=$PASTEBINKEY &>/tmp/alsainfo/wget.tmp &
	fi
fi
#Progess spinner for wget transfer.
if [[ -z "$DIALOG" ]]	
then
	i=1
	sp="/-\|"
	echo -n ' '
	while pgrep wget &>/dev/null
	do
	echo -en "\b${sp:i++%${#sp}:1}"
	done
fi

#See if tput is available, and use it if it is.	
if [[ -z $NOUPLOAD ]]; then
	if [[ -n "$TPUT" ]]
	then
		FINAL_URL=`tput setaf 1; grep "SUCCESS:" /tmp/alsainfo/wget.tmp |sed -n 's/.*\:\([0-9]\+\).*/http:\/\/pastebin.ca\/\1/p';tput sgr0`
	else
		FINAL_URL=`grep "SUCCESS:" /tmp/alsainfo/wget.tmp |sed -n 's/.*\:\([0-9]\+\).*/http:\/\/pastebin.ca\/\1/p'`
	fi
fi
#Output the URL of the uploaded file.	
if [[ -z $DIALOG ]]
then
	echo -e "\b Done!"
	echo ""
	if [[ -z $NOUPLOAD ]]; then
		echo "Your ALSA information is located at $FINAL_URL"
		echo "Please inform the person helping you."
		echo ""
	fi
fi
if [[ -n $DIALOG ]]
then
	if [[ -n $NOUPLOAD ]]; then
		if [[ -n $PBERROR ]]; then
			dialog --backtitle "$BGTITLE" --title "Information collected" --msgbox "An error occured while contacting the pastebin. Your information was NOT automatically uploaded.\n\nYour ALSA information can be seen by looking in $FILE" 10 100
		else
			dialog --backtitle "$BGTITLE" --title "Information collected" --msgbox "You requested that your information was NOT automatically uploaded to the pastebin\n\nYour ALSA information can be seen by looking in $FILE" 10 100
		fi
	else
		dialog --backtitle "$BGTITLE" --title "Information uploaded" --yesno "Would you like to see the uploaded information?" 5 100 
		DIALOG_EXIT_CODE=$?
	if [ $DIALOG_EXIT_CODE = 0 ]; then
		grep -v "alsa-info.txt" /tmp/alsa-info.txt >/tmp/alsainfo/uploaded.txt
		dialog --backtitle "$BGTITLE" --textbox /tmp/alsainfo/uploaded.txt 0 0
	fi
	fi
fi 
clear
if [[ -n $NOUPLOAD ]]; then
	if [[ -n $PBERROR ]]; then
		echo "An error occured while contacting the pastebin. Your information was NOT automatically uploaded."
		echo ""
		echo "Your ALSA information can be seen by looking in $FILE"
		echo ""
	else
		echo "You requested that your information was NOT automatically uploaded to the pastebin"
		echo ""
		echo "Your ALSA information can be seen by looking in $FILE"
		echo ""
	fi
fi
if [[ -z $NOUPLOAD ]]; then
echo "Your ALSA information is located at $FINAL_URL"
echo "Please inform the person helping you."
echo ""
fi
	#We posted the file to pastebin.ca , so we dont need it anymore. delete it.
	if [ -z $KEEP_FILES ]
	then
		rm $FILE 
	fi

#We couldnt find a suitable wget, so tell the user to upload manually.
else
	if [[ -z $DIALOG ]]
	then
		echo ""
		echo "Could not automatically upload output to http://www.pastebin.ca"
		echo "Possible reasons are:"
		echo "    1. Couldnt find 'wget' in your PATH"
		echo "    2. Your version of wget is less than 1.8.2"
		echo ""
		echo "Please manually upload $FILE to http://www.pastebin.ca/upload.php and submit your post."
		echo ""
	fi
	if [[ -n $DIALOG ]]
	then
		dialog --backtitle "$BGTITLE" --msgbox "Could not automatically upload output to http://www.pastebin.ca.\nPossible reasons are:\n\n    1. Couldn't find 'wget' in your PATH\n    2. Your version of wget is less than 1.8.2\n\nPlease manually upload $FILE to http://www.pastebin.ca/upload.php and submit your post." 25 100
	fi
fi
#Clean up the temp files
if [ -z $KEEP_FILES ]
then
	cleanup
fi

fi # proceed
