#!/bin/sh
#
# Build the photo product automatically.
#
# Get setup command

#
# Get the fermi UNIX environment
#
PATH=/usr/local/bin:$PATH
export PATH

OS_FLAVOR=`/usr/local/bin/funame -s`

echo "OS_FLAVOR is $OS_FLAVOR"

. /usr/local/etc/setups.sh

# setup the required products. There should only be two setup commands; 
# Other setups should be in the UPS dependencies for these omnibus products.
# If something is not set up correctly, then the dependencies are wrong.

setup photo devel
setup sdssrcvs

echo "======================================================"
echo "DERVISH_USER is defined to : $DERVISH_USER"
echo "======================================================"
echo

PHOTODATA_DIR="/usrdevel/s1/heidi/$OS_FLAVOR/photodata"
export PHOTODATA_DIR

# This is commented out until ups is fixed.  The line above replaces it.
# setup -r /usrdevel/s1/heidi/$OS_FLAVOR/photodata photodata

CRONDATA_DIR=$PHOTODATA_DIR/frames_1
export CRONDATA_DIR

#
# Build according to standards.
#

# 
# Set the path to acc on SunOS
#
if [ "$OS_FLAVOR" = "SunOS" ]
then
   PATH=$PATH:/usr/lang
   export PATH
fi

# Check out photodata
#cd /usrdevel/s1/heidi/$OS_FLAVOR
#rm -rf photodata
#cvs co photodata
cd $PHOTODATA_DIR
cvs update -d

#
# Check out photo
cd /usrdevel/s1/heidi/$OS_FLAVOR
rm -rf photo
cvs co photo

#
# Compile photo
cd photo
if [ "$OS_FLAVOR" = "SunOS" ]
then
   setup gcc
   sdssmake -cc gcc all PHOTO_DIR=/usrdevel/s1/heidi/$OS_FLAVOR/photo
else
   sdssmake all -debug PHOTO_DIR=/usrdevel/s1/heidi/$OS_FLAVOR/photo
fi

#
# Install photo
echo $PHOTO_DIR
sdssmake install
#
# Run the pipeline
if [ "$OS_FLAVOR" = "SunOS" ]
then
   cd etc
   echo "================================================================"
   echo "DERVISH_USER = $DERVISH_USER"
   echo "PHOTO_DIR  = $PHOTO_DIR"
   echo "PHOTODATA_DIR = $PHOTODATA_DIR"
   echo "================================================================"
   exec 1>$PHOTODATA_DIR/frames_1/output/log 2>&1
   ../bin/photo -file frames_cron.tcl
   exec 1>`tty` 2>&1
else
   cd bin
   echo "================================================================"
   echo "DERVISH_USER = $DERVISH_USER"
   echo "PHOTO_DIR  = $PHOTO_DIR"
   echo "PHOTODATA_DIR = $PHOTODATA_DIR"
   echo "================================================================"
   pixie -o photo.pixie photo
   exec 1>$PHOTODATA_DIR/frames_1/output/log 2>&1
   ./photo.pixie -file ../etc/frames_cron.tcl
   exec 1>`tty` 2>&1
   prof -pixie photo photo.Addrs photo.Counts > photo.Prof
   CUR_WD=`pwd`
fi
#
# Install photodata
cd $PHOTODATA_DIR
# When UPS is fixed, this should be replaced by a setup command
PHOTODATA_DIR="/p/GENERIC_UNIX/photodata/devel"; export PHOTODATA_DIR
echo $PHOTODATA_DIR
if [ "$OS_FLAVOR" = "IRIX" ]
then
   sdssmake install
   cp $CUR_WD/photo.Prof $PHOTODATA_DIR/frames_1/output/photo.Prof
fi
