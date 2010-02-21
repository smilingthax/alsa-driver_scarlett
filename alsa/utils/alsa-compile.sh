#!/bin/sh

protocol=
distrib=unknown
distribver="0.0"
tmpdir=$TMPDIR
if test -z "$tmpdir"; then
	tmpdir="/tmp"
fi
tmpdir=$tmpdir/alsa-compile-script
baseurl="http://www.alsa-project.org/snapshot/?package="
package="alsa-driver"
url=
gittree="git://git.alsa-project.org"
usegit=false
httpdownloader=
compile=
install=
quiet=
yes=
kernelmodules=

usage() {
	echo "Usage: $0 [OPTION]..."
	cat <<EOF

This is a script doing ALSA code compilation and installation.

Operation modes:
  -h, --help		print this help, then exit
  -q, --quiet		quiet mode
  -y, --yes		do not ask any questions - answer is always yes
  -c, --clean[=package]	do overall clean, do package clean
  --url=url		package url
  --git=giturl		work with git tree
  --compile		force compilation
  --install		install binaries and headers
  --tmpdir=dir		set temporary directory
  --kmodules[=mods]	reinstall kernel modules or install specified modules

Package selection:
  --driver		compile alsa-driver package (default)
  --lib			compile alsa-lib package
EOF
}

while :
do
	case "$1" in
	-h|--help)
		usage
		exit 0
		;;
	-q|--quiet)
		quiet=true ;;
	-y|--yes)
		yes=true ;;
	-c*|--clean*)
		rmpkg=
		case "$#,$1" in
		*,*=*)
			rmpkg=`expr "z$1" : 'z-[^=]*=\(.*\)'` ;;
		1,*)
			;;
		*)
			rmpkg="$2"
			shift ;;
		esac
		if test -z $rmpkg; then
			echo -n "Removing tree $tmpdir:"
			if test -d $tmpdir && rm -rf $tmpdir; then
				echo " failed"
				exit 1
			else
				echo " success"
			fi
		else
			echo -n "Removing package $package:"
			packagedir="$tmpdir/$package.dir"
			if test -r $packagedir; then
				tree=$(cat $packagedir)
				if test -d $tmpdir/$tree && rm -rf $tmpdir/$tree; then
					echo " failed"
					exit 1
				else
					rm -f $packagedir
					echo " success"
				fi
			fi
			echo " success"
		fi
		exit 0
		;;
	--url*)
		case "$#,$1" in
		*,*=*)
			url=`expr "z$1" : 'z-[^=]*=\(.*\)'` ;;
		1,*)
			usage ;;
		*)
			url="$2"
			shift ;;
		esac
		;;
	--git*)
		usegit="true"
		case "$#,$1" in
		*,*=*)
			gittree=`expr "z$1" : 'z-[^=]*=\(.*\)'` ;;
		1,*)
			;;
		*)
			gittree="$2"
			shift ;;
		esac
		;;
	--compile)
		compile="true"
		;;
	--install)
		install="true"
		;;
	--driver)
		package="alsa-driver"
		test -z "$url" || url="$basedir$package"
		;;
	--lib)
		package="alsa-lib"
		test -z "$url" || url="$basedir$package"
		;;
	--tmpdir*)
		case "$#,$1" in
		*,*=*)
			tmpdir=`expr "z$1" : 'z-[^=]*=\(.*\)'` ;;
		1,*)
			usage ;;
		*)
			tmpdir="$2"
			shift ;;
		esac
		tmpdir="$tmpdir/alsa-compile-script"
		;;
	--kmodules*)
		case "$#,$1" in
		*,*=*)
			kernelmodules=`expr "z$1" : 'z-[^=]*=\(.*\)'` ;;
		1,*)
			kernelmodules="reinstall" ;;
		*)
			kernelmodules="$2"
			shift ;;
		esac
		;;
	*)
		test -n "$1" && echo "Unknown parameter '$1'"
		break
		;;
	esac
	shift
done
if test -z "$url"; then
	package="alsa-driver"
	url="$baseurl$package"
fi

question_bool() {
	if test "$yes" = "yes"; then
		echo "true"
	else
		echo >&2 -n "$1 (Y/ ) "
		read i
		i=${i:0:1}
		if test "$i" = "Y" -o "$i" = "y"; then
			echo "true"
		else
			echo "false"
		fi
	fi
}	

check_distribution() {
	distrib=$(lsb_release -ds 2> /dev/null | cut -d ' ' -f 1)
	distrib=${distrib:1}
	distribver=$(lsb_release -rs 2> /dev/null)
	if test -z "$distrib"; then
		echo >&2 "Unable to determine Linux distribution!"
		exit 1
	fi
	if test -z "$distribver"; then
		echo >&2 "Unable to determine Linux distribution version!"
		exit 1
	fi
	echo "Detected Linux distribution '$distrib $distribver'."
}

is_rpm_installed() {
	a=$(rpm -qi $1 | head -1 | cut -d ' ' -f 1)
	if test "$a" = "Name"; then
		echo "frue"
	else
		echo "false"
	fi
}

install_package() {
	echo "Trying to install package '$1':"
	case "$distrib" in
	openSUSE)
		if zypper install $1; then
			echo "Cannot install package '$1'."
			exit 1
		fi
		;;
	Fedora)
		if yum install $1; then
			echo "Cannot install package '$1'."
			exit 1
		fi
		;;
	*)
		echo >&2 "install_package: Unsupported distribution $distrib"
		exit 1
	esac
	echo "  installed"
}

check_kernel_source() {
	echo "Checking kernel build environment:"
	case "$distrib" in
	openSUSE)
		if test $(is_rpm_installed kernel-source) = "false" ; then
			install_package kernel-source
		fi
		;;
	Fedora)
		if test $(is_rpm_installed kernel-devel) = "false" ; then
			install_package kernel-devel
		fi
		;;
	*)
		echo >&2 "check_kernel_source: Unsupported distribution $distrib"
		exit 1
		;;
	esac
	echo "  passed"
}

check_environment() {
	if ! test -d $tmpdir ; then
		mkdir -p $tmpdir
		if ! test -d $tmpdir; then
			echo >&2 "Unable to create directory $tmpdir"
			exit 1
		fi
	fi
	echo "Using temporary tree $tmpdir"
	check_distribution
}

check_compilation_environment() {
	a=$(wget --version | head -1 | cut -d ' ' -f 2)
	if test "$a" = "Wget"; then
		httpdownloader="wget"
	else
		a=$(curl --version | head -1 | cut -d ' ' -f 1 )
		if test "$a" = "curl"; then
			httpdownloader="curl"
		fi
	fi
	if test -z "$httpdownloader"; then
		echo >&2 "Unable to determine HTTP downloader."
		exit 1
	fi
	a=$(git --version | head -1 | cut -d ' ' -f 1)
	if test "$a" != "git"; then
		install_package git
	else
		echo "Program git found."
	fi
	a=$(autoconf --version | head -1 | cut -d ' ' -f 1)
	if test "$a" != "autoconf"; then
		install_package autoconf
	else
		echo "Program autoconf found."
	fi
	a=$(gcc --version | head -1 | cut -d ' ' -f 1)
	if test "$a" != "gcc" ; then
		install_package gcc
	else
		echo "Program gcc found."
	fi
	check_kernel_source
}

download_http_file() {
	echo "Downloading '$1':"
	case "$httpdownloader" in
	wget)
		wget -q -O $2 $1
		;;
	curl)
		curl -s -o $2 $1
		;;
	*)
		echo >&2 "Unknown httpdownloader '$httpdownloader'."
		exit 1
		;;
	esac
	size=$(stat -c%s $2)
	echo "  success ($size bytes)"
}

do_compile() {
	cmd="./gitcompile"
	case "$package" in
	alsa-driver)
		if test -r acore/hwdep.o; then
			cmd="make"
		fi
		;;
	esac
	echo "Running $cmd:"
	if ! $cmd; then
		a=$(pwd)
		echo >&2 "Working directory $a"
		echo >&2 "Compilation of $package tree failed."
		echo >&2 "Report this problem to the <alsa-devel@alsa-project.org> mailing list."
		exit 1
	fi
}

do_install() {
	if ! test "$UID" -eq 0 ; then
		echo >&2 "Root priviledges required for installation."
		echo >&2 "Login as root to continue."
		exit 1
	fi
	cmd="make install"
	case "$package" in
	alsa-driver)
		cat <<EOF
WARNING! You chose to install new ALSA kernel modules. The current ALSA
kernel modules will be removed from your system pernamently. In case of
problems, reinstall your kernel package.
EOF
		if test $(question_bool "Really continue (Ctrl-C to abort)?") = "true"; then
			cmd="make install-modules"
		fi
		;;
	esac
	$cmd || exit 1
}

do_git() {
	echo "do git"
}

do_cmd() {
	echo "> $@"
	$@ || exit 1
}

kill_audio_apps() {
	pids0=$(fuser /dev/snd/* 2> /dev/null)
	pids1=$(fuser /dev/mixer* 2> /dev/null)
	pids2=$(fuser /dev/sequencer* 2> /dev/null)
	pids=
	for pid in $pids0 $pids1 $pids2; do
		pids="$pids $pid"
	done
	if ! test -z "$pids"; then
		echo
		echo "WARNING! An audio application uses ALSA driver:"
		echo
		for pid in $pids; do
			ps --no-headers -p $pids || exit 1
		done
		echo
		if test $(question_bool "Would you like to kill these apps?") = "true"; then
			for pid in $pids; do
				do_cmd kill $pid
			done
			sleep 2
			killed=
			for pid in $pids; do
				a=$(ps --no-headers -p $pids)
				if test -n "$a"; then
					do_cmd kill -9 $pid
					killed="true"
				fi
			done
			if test "$killed" = "true"; then
				sleep 2
				for pid in $pids; do
					a=$(ps --no-headers -p $pids)
					if test -n "$a"; then
						echo >&2 "Unable to kill application:"
						echo >&2 "  $a"
						exit 1
					fi
				done
			fi
		else
			echo >&2 "Terminated by user."
			exit 1
		fi
	fi
}

parse_modules() {	
	if ! test -s ../modules.dep; then
		rel=$(uname -r)
		dst="xxxx/lib/modules/$rel"
		mkdir -p $dst/modules || exit 1
		for i in modules/*.*o; do
			ln -sf ../../../../../$i $dst/$i || exit 1
		done
		p=$(pwd)
		if ! depmod -b $p/xxxx ; then
			echo >&2 "depmod failed."
			exit 1
		fi
		
		if ! cp $dst/modules.dep ../modules.dep ; then
			echo >&2 "cp problem."
			exit 1
		fi
		rm -rf xxxx || exit 1
		echo "File modules.dep has been created."
	else
		echo "File modules.dep is cached."
	fi

	if ! test -s ../modules.top ; then
		for i in modules/*.*o; do
			a=$(modinfo $i | grep "parm:" | grep "enable:")
			if ! test -z "$a"; then
				a=$(basename $i | cut -d . -f 1)
				echo $a >> ../modules.top
			fi
		done
		echo "File modules.top has been created."
	else
		echo "File modules.top is cached."
	fi
}

current_modules() {
	lsmod | cut -d ' ' -f 1 | grep -E "^(snd[_-]|snd$)"
}

my_rmmod() {
	while test -n "$1"; do
		if ! rmmod $1 2> /dev/null > /dev/null; then
			phase2="$phase2 $1"
		else
			echo "> rmmod $1"
		fi
		shift
	done
	for mod in $phase2; do
		echo "> rmmod $mod"
		if ! rmmod $mod ; then
			echo >&2 "Unable to remove kernel module $mod"
			exit 1
		fi
	done
}

my_insmod() {
	while test -n "$1"; do
		if test -r modules/$1.ko; then
			mod=modules/$1.ko
			echo "> insmod $1.ko"
			if ! insmod modules/$1.ko ; then
				echo >&2 "Unable to insert kernel module $1.ko"
				exit 1
			fi
		else
			if test -r modules/$1.o; then
				mod=modules/$1.o
				echo "> insmod $1.o"
				if ! insmod modules/$1.o ; then
					echo >&2 "Unable to insert kernel module $1.o"
					exit 1
				fi
			else
				echo >&2 "Unable to find kernel module $1"
				exit 1
			fi
		fi
		shift
	done
}

do_kernel_modules() {
	usermods="$@"
	curmods=$(current_modules)
	if test -z "$curmods" -a -z "$usermods"; then
		echo >&2 "Unable to determine current ALSA kernel modules."
		exit 1
	fi
	if test -n "$usermods"; then
		loadmods="$usermods"
		dst="../modules.user"
		rm -f $dst || exit 1
	else
		loadmods="$curmods"
		dst="../modules.insmod"
	fi
	parse_modules
	if ! test -s $dst; then
		topmods=$(cat ../modules.top)
		cat > run.awk <<EOF
function basename(name) {
	split(name, y, "[/.]")
	return y[length(y)-1]
}
function skipempty(dst, src, delim,   x) {
	split(src, y, delim)
	for (x in y) {
		if (length(y[x]) > 0)
			dst[x] = y[x]
	}
}
function skipemptyuser(dst, src, delim,   x, m) {
	split(src, y, delim)
	for (x in y) {
		if (length(y[x]) > 0) {
			m = y[x]
			if (substr(m, 0, 4) != "snd-")
				m = "snd-" m
			dst[x] = m
		}
	}
}
function moduleindex(mod,   x) {
	for (x in modules) {
		if (modules[x] == mod)
			return x
	}
	return 0;
}
function addmodule(mod,    v, j, k) {
	if (moduleindex(mod) > 0)
		return
	v = split(deps[mod], j, " ")
	for (k = 1; k <= v; k++)
		addmodule(j[k])
	modules[length(modules)+1] = mod
}
function addslavemodules(mod, exclude,   m, v, j, k)
{
	for (m in deps) {
		v = split(deps[m], j, " ")
		for (k = 1; k <= v; k++) {
			if (j[k] == mod && m != exclude) {
				addmodule(m)
			}
		}
	}
}
function addtopmodule(mod,   v, j, k, z)
{
	v = split(deps[mod], j, " ")
	for (k = 1; k <= v; k++) {
		for (z = 1; z <= length(alldeps); z++) {
			if (j[k] == alldeps[z])
				addslavemodules(j[k], mod)
		}
	}
	addmodule(mod)
	addslavemodules(mod)
}
BEGIN   {
		skipempty(akmods, kmods, ",")
		if (length(usermods) > 0)
			skipemptyuser(acurmods, usermods, " ")
		else
			skipempty(acurmods, curmods, " ")
		skipempty(atopmods, topmods, " ")
		for (b in atopmods)
			alldeps[b] = atopmods[b]
		# hack to load HDA codec modules
		alldeps[length(alldeps)+1] = "snd-hda-codec"
	}
	{
		split(\$0, a, "[: ]")
		first = 1
		for (b = 1; b <= length(a); b++) {
			if (length(a[b]) <= 0)
				continue
			base = basename(a[b])
			if (first) {
				mod = base
				deps[mod] = ""
				first = 0
			} else {
				if (length(deps[mod]) > 0)
					deps[mod] = deps[mod] " " base
				else
					deps[mod] = base
			}
		}
		delete a

	}
END     {
		addmodule("snd-page-alloc")

		# all direct toplevel modules dependencies
		for (mod in deps) {
			split(deps[mod], mods, " ")
			for (d in atopmods) {
				if (mod != atopmods[d])
					continue
				for (b in acurmods) {
					if (mod != acurmods[b])
						continue
					addtopmodule(mod)
					break
				}
			}
		}

		addmodule("snd-seq")
		addmodule("snd-mixer-oss")
		for (b in modules) {
			if ("snd-pcm" == modules[b])
				addmodule("snd-pcm-oss")
		}

		for (b = 1; b <= length(modules); b++) {
			mod = modules[b]
			if (length(mod) > 0)
				print mod
		}
	}
EOF
		awk -f run.awk -v kmods="$kernelmodules" \
		       -v curmods="$curmods" \
		       -v usermods="$usermods" \
		       -v topmods="$topmods" \
				../modules.dep > $dst || exit 1
		rm run.awk || exit 1
		if ! test -s $dst; then
			if test -n "$usermods"; then
				echo >&2 "Unable to determine specified ALSA kernel modules."
			else
				echo >&2 "Unable to determine current ALSA kernel modules."
			fi
			exit 1
		fi
		if test -z "$usermods"; then
			echo "File modules.insmod has been created."
		fi
	else
		if test -z "$usermods"; then
			echo "File modules.insmod is cached."
		fi
	fi
	kill_audio_apps
	my_rmmod $curmods
	my_insmod $(cat $dst)
	echo "Kernel modules ready:"
	cat /proc/asound/cards
	if ! alsactl init 2> /dev/null > /dev/null ; then
		echo "Mixer initialized using 'alsactl init' command."
	else
		echo "Use a mixer application (like alsamixer) to set reasonable volume levels."
	fi
}

kernel_modules() {
	if test "$package" != "alsa-driver"; then
		echo >&2 "--kernel-modules works only for alsa-driver package."
		exit 1
	fi
	if test "$kernelmodules" = "reinstall"; then
		do_kernel_modules
	else
		a=$(echo $kernelmodules | sed -e 's/,/ /g')
		do_kernel_modules $a
	fi
}

export LANG=C
protocol=$(echo $url | cut -d ':' -f 1)
check_environment $protocol
do_cmd cd $tmpdir
if test -n "$kernelmodules" -a -z "$compile"; then
	packagedir="$package.dir"
	if test -r $packagedir; then
		tree=$(cat $package.dir)
	fi
	do_cmd cd $tree
	kernel_modules
	exit 0
fi
case "$protocol" in
http|https)
	packagedir="$package.dir"
	if test -r $packagedir; then
		tree=$(cat $packagedir)
		echo "$package tree $tree is present."
		echo "Reusing it."
		echo "Use '$0 --clean=$package' command to refetch and rebuild."
	else
		check_compilation_environment $protocol
		snapshot="snapshot.tar.bz2"
		download_http_file $url $snapshot
		do_cmd tar xjf $snapshot
		rm $snapshot
		tree=$(ls | grep $package)
	fi
	if ! test -x $tree/gitcompile ; then
		echo >&2 "Fatal: $package tree not found"
		exit 1
	fi
	echo "Sources unpacked to $tree"
	echo $tree > $packagedir
	do_cmd cd $tree
	do_compile
	if test "$install" = "true"; then
		do_install
	else
		echo "ALSA package $package was successfully built."
		echo "Binaries are in $tmpdir/$tree directory."
	fi
	do_cmd cd $tmpdir
	;;
git)
	do_git $url
	;;
*)
	echo >&2 "Unknown protocol '$protocol'."
	exit 1
esac

if test -n "$kernelmodules"; then
	packagedir="$package.dir"
	if test -r $packagedir; then
		tree=$(cat $package.dir)
	fi
	do_cmd cd $tree
	kernel_modules
	exit 0
fi

exit 0
