#!/bin/bash

echo -e "\nPreparing code basis for a new module:\n"

# Ask for module name:
read -p "Name of the module? " MODNAME

# Ask for module type:
echo "Type of the module?"
type=0
select yn in "unique" "detector"; do
    case $yn in
        unique ) type=1; break;;
        detector ) type=2; break;;
    esac
done

echo "Creating directory and files..."

echo 
# Try to find the modules directory:
DIRECTORIES[0]="../src/modules"
DIRECTORIES[1]="src/modules"

MODDIR=""
for DIR in "${DIRECTORIES[@]}"; do
    if [ -d "$DIR" ]; then
	MODDIR="$DIR"
	break
    fi
done

# Create directory
mkdir "$MODDIR/$MODNAME"

# Copy over CMake file and sources from Dummy:
sed -e "s/Dummy/$MODNAME/g" "$MODDIR/Dummy/CMakeLists.txt" > "$MODDIR/$MODNAME/CMakeLists.txt" 

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
    -e "s/\*NAME\*/$MYNAME/g" \
    -e "s/\*EMAIL\*/$MYMAIL/g" \
    -e "s/Functional/Immature/g" \
    "$MODDIR/Dummy/README.md" > "$MODDIR/$MODNAME/README.md"

# Copy over source code skeleton:
sed -e "s/Dummy/$MODNAME/g" "$MODDIR/Dummy/DummyModule.hpp" > "$MODDIR/$MODNAME/${MODNAME}Module.hpp" 
sed -e "s/Dummy/$MODNAME/g" "$MODDIR/Dummy/DummyModule.cpp" > "$MODDIR/$MODNAME/${MODNAME}Module.cpp" 

# Change to detetcor module type if necessary:
if [ "$type" == 2 ]; then
    sed -i -e "s/_UNIQUE_/_DETECTOR_/g" "$MODDIR/$MODNAME/CMakeLists.txt"
    sed -i -e "s/ unique / detector-specific /g" \
	-e "/param geo/c\         \* \@param detector Pointer to the detector for this module instance" \
	-e "s/GeometryManager\* geo\_manager/std::shared\_ptr\<Detector\> detector/g" \
	-e "s/GeometryManager/DetectorModel/g" \
	"$MODDIR/$MODNAME/${MODNAME}Module.hpp"
    sed -i -e "s/GeometryManager\*/std::shared\_ptr\<Detector\> detector/g" \
	-e "s/Module(config)/Module\(config\, detector\)/g" \
	"$MODDIR/$MODNAME/${MODNAME}Module.cpp"
fi

echo "Name:   $MODNAME"
echo "Author: $MYNAME ($MYMAIL)"
echo "Path:   $MODDIR/$MODNAME"
echo
echo "Re-run CMake in order to build your new module."
