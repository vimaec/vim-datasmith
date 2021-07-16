#!/bin/sh

projectPath=`dirname "$0"`

clang-format -i "$projectPath/../VimToDatasmith"/*.*
