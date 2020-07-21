#/bin/bash
set -e

PROJECTNAME=`cat buildenv/projname`
ESPCONTAINERVERSION=`cat buildenv/espcontainerver`

docker run --name $PROJECTNAME --rm -it --privileged -e USER_ID=$UID -e GROUP_ID=`id -g` -v /dev/bus/usb:/dev/bus/usb -v $PWD:/esp/project $PROJECTNAME:$ESPCONTAINERVERSION 
