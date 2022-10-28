#!/bin/bash
set -e

while getopts d:v: flag
do
	case $flag in
		d) date="$OPTARG";;
		v) ver="$OPTARG";;
	esac
done

if [ -z "$ver" ] ; then
	echo 'Version not specified' 1>&2
	exit 1
fi

if [ -n "$date" ] ; then
	input_date=( '-j' '-f' '%Y-%m-%d' "$date" )
fi
build_no=$(date "${input_date[@]}" '+%Y.%j')

echo "Version: $ver"
echo "Build: $build_no"
echo "$PWD"
