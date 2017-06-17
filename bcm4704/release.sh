#!/bin/bash
#
# Copyright 2007, Broadcom Corporation
# All Rights Reserved.                
#                                     
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;   
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior      
# written permission of Broadcom Corporation.                            
#  
# Creates an open license tarball
#
# $Id: release.sh,v 1.1.1.1 2010/03/05 07:31:02 reynolds Exp $
#

FILELIST="gpl-filelist.txt"
FILELISTTEMP="gpl-filelist-temp.txt"

RELEASE=yes


usage ()
{
    echo "usage: $0 [open release]"
    exit 1
}

[ "${#*}" = "0" ] && usage

echo "cat ${FILELIST} | sed -e 's/[[:space:]]*$//g' > ${FILELISTTEMP}"
cat ${FILELIST} | sed -e 's/[[:space:]]*$//g' > ${FILELISTTEMP}
if ! diff ${FILELIST} ${FILELISTTEMP} > /dev/null 2>&1; then
   echo "WARNING: "
   echo "WARNING: Fix trailing whitespace in following entries"
   echo "WARNING: in ${FILELIST} file (ignored)"
   echo "WARNING: "
   diff ${FILELIST} ${FILELISTTEMP} | grep "^>"
fi

# create open license tarball
echo -e "\ntar czf \"$1\" -T ${FILELISTTEMP} --ignore-failed-read\n"
tar czf "$1" -T ${FILELISTTEMP} --ignore-failed-read
rm -f ${FILELISTTEMP}


