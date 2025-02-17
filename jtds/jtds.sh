#!/bin/bash -l 
usage(){ cat << EOU
~/j/jtds/jtds.sh
==================

::

   ~/j/jtds/jtds.sh info

   REMOTE=P ~/j/jtds/jtds.sh grab
   REMOTE=L ~/j/jtds/jtds.sh grab
   ~/j/jtds/jtds.sh grab

   ~/j/jtds/jtds.sh ana
   EVT=2 ~/j/jtds/jtds.sh ana
   EVT=9 ~/j/jtds/jtds.sh ana

EOU
}

defarg="info_ana"
arg=${1:-$defarg}

name=jtds
SDIR=$(cd $(dirname $BASH_SOURCE) && pwd)
source $HOME/.opticks/GEOM/GEOM.sh 

export REMOTE=${REMOTE:-P}
export L_TMP=/hpcfs/juno/junogpu/blyth/tmp
export P_TMP=/data/blyth/opticks    # HOME/tmp on P is symbolic link to /data/blyth/opticks 
# using local laptop directories that duplicate the remote ones

TMPVAR=${REMOTE}_TMP
export TMP=${!TMPVAR}

export BASE=${TMP:-/tmp/$USER/opticks}/GEOM/$GEOM/jok-tds/ALL0

EVT=${EVT:-000}
export AFOLD=$BASE/A$EVT
export BFOLD=$BASE/B$EVT
script=$SDIR/$name.py 

vars="BASH_SOURCE REMOTE L_TMP P_TMP TMPVAR TMP arg name SDIR GEOM BASE EVT evt AFOLD BFOLD script"

if [ "${arg/info}" != "$arg" ]; then
   for var in $vars ; do printf "%20s : %s \n" "$var" "${!var}" ; done 
fi 

if [ "${arg/grab}" != "$arg" ]; then 
    source $OPTICKS_HOME/bin/rsync.sh $BASE
fi 

if [ "${arg/ana}" != "$arg" ]; then
   ${IPYTHON:-ipython} --pdb -i $script 
   [ $? -ne 0 ] && echo $BASH_SOURCE ana error && exit 1 
fi


