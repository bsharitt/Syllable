#!/bin/bash

VERSION=0.6.2

BUILD_DIR=$HOME/Build
INSTALLER_DIR=$BUILD_DIR/Installer

# If you want to automatically upload the build to
# an FTP server, set the following environment variables
# in your .profile:
#
# FTP_SERVER
# FTP_USER
# FTP_PASSWD

# Clean up from any previous build
if [ -e $INSTALLER_DIR/base-syllable.zip ]
then
  rm $INSTALLER_DIR/base-syllable.zip
fi
if [ -e $INSTALLER_DIR/files ]
then
  rm -rf $INSTALLER_DIR/files
fi

# XXXKV: Need to manually copy the GRUB files
cd $BUILD_DIR/system/stage/image
cp -a atheos/usr/grub/lib/grub/i386-pc/* boot/grub/

# Finish the build and package it
cd $BUILD_DIR/system
image finish

cd $BUILD_DIR/system/stage/image
zip -yr9 $INSTALLER_DIR/base-syllable.zip *
sync

# XXXKV: Ensure we have the latest installer scripts
cd $INSTALLER_DIR/installer
cvs update -dP
sync

# Build the CD
cd $INSTALLER_DIR
./build-cd.sh "base-syllable.zip" "$BUILD_DIR/Net-Binaries" "$VERSION"
if [ -e syllable.iso.bz2 ]
then
  rm syllable.iso.bz2
fi
bzip2 -k9 syllable.iso

# Transfer the files
FILES=`printf "base-syllable.zip syllable.iso.bz2\n"`
if [ -n "$FTP_USER" ]
then
  ftp -n $FTP_SERVER << END
quote user $FTP_USER
quote pass $FTP_PASSWD
prompt
mput $FILES
quit
END
fi
