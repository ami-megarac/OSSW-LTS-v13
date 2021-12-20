:
archive=/tmp/sd_regression.tgz

if [ ! -f ./AppArmor.rtf ]
then
	echo "Error: pwd is not valid" >&2
	exit 1
fi

make clean

tar -czf $archive --exclude=TODO --exclude .svn --exclude scripts . && \

echo 
echo "output is $archive"
