#!/bin/sh

# Put this script under src/prebuilt to run

TOP=`pwd`/../..

cd $TOP/project
mv acos acos_XXX
mkdir acos
cd acos
cp ../acos_XXX/config.in .
cp ../acos_XXX/config.mk .
cp ../acos_XXX/Makefile .
mkdir include
cd include
cp ../../acos_XXX/include/ambitCfg_NA.h .
cp ../../acos_XXX/include/ambitCfg_WW.h .
cp ../../acos_XXX/include/ambitCfg_GR.h .
cp ../../acos_XXX/include/ambitCfg_KO.h .


cd $TOP/ap
mv acos acos_XXX
mkdir acos
cd acos
ln -s ../../project/acos/config.in config.in
ln -s ../../project/acos/config.mk config.mk
ln -s ../../project/acos/Makefile Makefile

