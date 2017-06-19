# Run the second argument in the directory created from the first argument
rm -rf $1
mkdir $1
cd $1
exec $2
