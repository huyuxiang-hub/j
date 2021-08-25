#!/bin/bash -l 

pkg=$(basename $(pwd))
from=P:/tmp/$pkg/
to=/tmp/$pkg/

echo pkg $pkg from $from to $to

if [ "$1" != "ls" ]; then
rsync -zarv --progress --include="*/" --include="*.npy" --include="*.jpg" --include="*.mp4" --include "*.json" --exclude="*" "$from" "$to"
fi 

ls -1rt `find ${to%/} -name '*.json' `
ls -1rt `find ${to%/} -name '*.jpg' -o -name '*.mp4' -o -name '*.npy'  `

