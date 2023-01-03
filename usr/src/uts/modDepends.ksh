#!/bin/ksh
found=0
while getopts N:d:r name
do
	case $name in
	N)      if [ found -eq 1 ]
		then
			depend="$depend $OPTARG"
		else
			depend="$OPTARG"
			found=1
		fi
		;;
	*)	true
		;;
	esac
done
shift $(($OPTIND-1))
if [ $# -eq 1 ]
then
	echo "\t.align 4" >$1
	echo "\t.global _depends_on" >>$1
	echo "_depends_on:" >>$1
	echo "\t.asciz \"$depend\"" >>$1
	echo "\t.size _depends_on, .-_depends_on" >>$1
	exit 0
else
	exit 1
fi
