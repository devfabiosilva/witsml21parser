
search_dir=$(pwd)/../build/lib*

if [ -d $search_dir ]; then
 echo $(readlink -f $search_dir)
 export PYTHONPATH=$PYTHONPATH:$(readlink -f $search_dir)
 echo "Python path set to $PYTHONPATH"
else
 echo "ERROR: Py WITSML 2.1 to BSON parser not found. Compile it witm 'make py' and run this environment again"
 return 1
fi

