#!/bin/sh

#CmdPath="/Users/richardyoung/Library/Developer/Xcode/DerivedData/VimToDatasmith-egojbinbcntqulbcqycnyhrmfoce/Build/Products/Release/VimToDatasmith -NoHierarchicalInstance"
CmdPath="/Users/richardyoung/Library/Developer/Xcode/DerivedData/VimToDatasmith-egojbinbcntqulbcqycnyhrmfoce/Build/Products/Release/VimToDatasmith"
VIMTestFolder="/Volumes/RY Données/Downloads/VIM"

${CmdPath} "${VIMTestFolder}/Substation.v1.vim"
${CmdPath} "${VIMTestFolder}/Project Soane.v1.vim"
${CmdPath} "${VIMTestFolder}/Dwelling.v1.vim"
${CmdPath} "${VIMTestFolder}/Autodesk Hospital.v1.vim"
${CmdPath} "${VIMTestFolder}/Skanska_next.vim"
${CmdPath} "${VIMTestFolder}/_MAIN-STADIUM-R19.vim"
${CmdPath} "${VIMTestFolder}/rac_basic_sample_project.vim"

open "${VIMTestFolder}"
