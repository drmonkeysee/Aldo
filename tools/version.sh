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
read -d '.' major_ver <<< "$ver"

if [ -n "$date" ] ; then
	input_date=( '-j' '-f' '%Y-%m-%d' "$date" )
fi
build_no=$(date "${input_date[@]}" '+%Y.%j')

echo "Setting version $ver ($build_no) (lib-compat: $major_ver)..."

sed -i .bak "s/\(AldoVersion = \)\".*\"/\1\"$ver ($build_no)\"/" ${PWD}/src/version.c
echo 'Updated c-src version'

sed -i .bak -E \
	-e "s/(MARKETING|DYLIB_CURRENT)(_VERSION = ).*;/\1\2$ver;/" \
	-e "s/(CURRENT_PROJECT_VERSION = ).*;/\1$build_no;/" \
	-e "s/(DYLIB_COMPATIBILITY_VERSION = ).*;/\1$major_ver;/" \
	${PWD}/mac/Aldo.xcodeproj/project.pbxproj
echo 'Updated mac project versions'
