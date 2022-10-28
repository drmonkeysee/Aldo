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

echo "Setting version $ver ($build_no)..."

sed -i .bak "s/\(AldoVersion = \)\".*\"/\1\"$ver ($build_no)\"/" ${PWD}/src/version.c
echo 'Updated version.c'

# TODO: guard or select this explicitly for macOS
pushd mac
xcrun agvtool vers
xcrun agvtool mvers
popd
