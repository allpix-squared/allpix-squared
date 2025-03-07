#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

echo -e "\nPreparing code basis for a new module:\n"

# Ask for module name:
read -p "Name of the module? " MODNAME

# Ask for module description:
read -p "Short description of the module? " MODDESC

# Ask for module type:
echo -e "Type of the module?\n"
type=0
select yn in "unique" "detector"; do
    case $yn in
        unique ) type=1; break;;
        detector ) type=2; break;;
    esac
done

# Ask for module type:
echo -e "Event order requirements?\n"
sequence=0
select yn in "Module" "SequentialModule"; do
    case $yn in
        Module ) sequence=0; break;;
        SequentialModule ) sequence=1; break;;
    esac
done

# Ask for the message type to run over
echo ""
read -p "Input message type? " MESSAGETYPE

# Check that message type exists
OBJDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P )"
OBJDIR=$OBJDIR/../../src/objects
if [ ! -z "${MESSAGETYPE}" ] && [ ! -e ${OBJDIR}/${MESSAGETYPE}.hpp ]
then
    echo -e "\nMessage type ${MESSAGETYPE} does not exist. \nPlease see the message types in ${OBJDIR}\n"
    exit 1
fi

echo "Creating directory and files..."

echo
# Try to find the modules directory:
BASEDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P )"
DIRECTORIES[0]="${BASEDIR}/../../src/modules"
DIRECTORIES[1]="../src/modules"
DIRECTORIES[2]="src/modules"

MODDIR=""
for DIR in "${DIRECTORIES[@]}"; do
    if [ -d "$DIR" ]; then
        MODDIR="$DIR"
        break
    fi
done

# Create directory
mkdir "$MODDIR/$MODNAME"
if [ $? != 0 ]; then
    exit 1
fi

# Copy over CMake file and sources from Dummy:
sed -e "s/Dummy/$MODNAME/g" $MODDIR/Dummy/CMakeLists.txt > $MODDIR/$MODNAME/CMakeLists.txt

# Copy over the README, setting current git username/email as author
# If this fails, use system username and hostname
MYNAME=$(git config user.name)
MYMAIL=$(git config user.email)
if [ -z "$MYNAME" ]; then
    MYNAME=$(whoami)
fi
if [ -z "$MYMAIL" ]; then
    MYMAIL=$(hostname)
fi
sed -e "s/Dummy/$MODNAME/g" \
    -e "s/A dummy module/$MODDESC/g" \
    -e "s/John Doe/$MYNAME/g" \
    -e "s/john\.doe@example\.com/$MYMAIL/g" \
    -e "s/Functional/Immature/g" \
    "$MODDIR/Dummy/README.md" > "$MODDIR/$MODNAME/README.md"

# Copy over source code skeleton:
sed -e "s/Dummy/$MODNAME/g" "$MODDIR/Dummy/DummyModule.hpp" > "$MODDIR/$MODNAME/${MODNAME}Module.hpp"
sed -e "s/Dummy/$MODNAME/g" "$MODDIR/Dummy/DummyModule.cpp" > "$MODDIR/$MODNAME/${MODNAME}Module.cpp"

# Options for sed vary slightly between mac and linux
opt=-i
platform=`uname`
if [ "$platform" = "Darwin" ]; then opt="-i \"\""; fi

# Change to detector module type if necessary:
if [ "$type" = 2 ]; then

    # Prepare sed commands to change to per detector module
    # Change module type in CMakeLists
    command="sed $opt 's/_UNIQUE_/_DETECTOR_/g' $MODDIR/$MODNAME/CMakeLists.txt"
    eval $command
    # Change header file
    command="sed ${opt} \
        -e 's/ unique / detector-specific /g' \
        -e 's/param geo_manager.*/param detector Pointer to the detector for this module instance/g' \
        -e 's/GeometryManager\* geo\_manager/std::shared\_ptr\<Detector\> detector/g' \
        -e 's/GeometryManager/DetectorModel/g' \
        $MODDIR/$MODNAME/${MODNAME}Module.hpp"
    eval $command
    # Change implementation file
    command="sed ${opt} \
        -e 's/GeometryManager\* geo_manager/std::shared\_ptr\<Detector\> detector/g' \
        -e 's/Module(config)/Module\(config\, detector\)/g' \
        -e 's/geo_manager_(geo_manager)/detector_(std::move(detector))/g' \
        -e 's/bindMulti/bindSingle/g' \
        -e '/for(auto/d' \
        -e 's/\s\{8\}/\ \ \ \ /' \
        -e '/geo_manager_/d' \
        -e 's/detector->get/detector_->get/g' \
        -e '/    }/d' \
        -e '/Loop/d' \
        -e 's/auto messages/auto message/g' \
        -e 's/fetchMultiMessage/fetchMessage/g' \
        $MODDIR/$MODNAME/${MODNAME}Module.cpp"
    eval $command
fi

# Change to SequentialModule if required
if [ "$sequence" = 1 ]; then
    # Change header file
    command="sed ${opt} \
        -e 's/public Module/public SequentialModule/g' \
        $MODDIR/$MODNAME/${MODNAME}Module.hpp"
    eval $command
    # Change implementation file
    command="sed ${opt} \
        -e 's/ Module/ SequentialModule/g' \
        $MODDIR/$MODNAME/${MODNAME}Module.cpp"
    eval $command
fi

if [ ! -z "${MESSAGETYPE}" ]; then
    # Replace the corresponding message type in the header and source file
    command="sed ${opt} \
        -e 's/PixelHit/${MESSAGETYPE}/g' \
        $MODDIR/$MODNAME/${MODNAME}Module.*pp"
else
    command="sed ${opt} \
        -e '/PixelHit/d' \
        -e '/\/\/ Messages:/d' \
        -e '/for(auto\& message/,+4d' \
        -e '/particular message/,+2d' \
        $MODDIR/$MODNAME/${MODNAME}Module.*pp"
fi
eval $command

# Print a summary of the module created:
FINALPATH=`realpath $MODDIR/$MODNAME`
echo "Name:        $MODNAME"
echo "Description: $MODDESC"
echo "Author:      $MYNAME ($MYMAIL)"
echo "Path:        $FINALPATH"
if [ ! -z "${MESSAGETYPE}" ]; then
    echo "This module listens to \"$MESSAGETYPE\" messages from" $([ "$type" = 2 ] && echo "one detector" || echo "all detectors")
fi
if [ "$sequence" = 1 ]; then
    echo "This module requires sequential processing of events in multithreaded environments."
fi
echo
echo "Re-run CMake in order to build your new module."
