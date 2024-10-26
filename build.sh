#!/bin/sh

set -e

export BUILD_CONFIG=$1

if [ -n "${BUILD_CONFIG+set}" ]
	then
		true
	else
		export BUILD_CONFIG=Release
fi


# Build LVR
	premake5 gmake2 --cc=gcc --verbose
	make config=$(echo "$BUILD_CONFIG" | tr '[:upper:]' '[:lower:]') "$2"
