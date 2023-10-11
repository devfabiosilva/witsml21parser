
search_dir=$(pwd)/../build/Release

if [ -d $search_dir ]; then
 echo $(readlink -f $search_dir)
 export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(readlink -f $search_dir)
 echo "Node path set to $LD_LIBRARY_PATH"
else
 echo "ERROR: JSWITSML 2.1 to BSON parser not found. Compile addon with 'make nodejs' and run this environment again"
 return 1
fi

