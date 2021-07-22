#!/bin/sh

projectPath=`dirname "$0"`

clang-format -i "$projectPath/../VimToDatasmith"/*.*
clang-format -i "$projectPath/../UnrealEngine"/*.*
