#!/bin/bash
# source this script to setup compiler ENV

export BUILD=cray
export COMPILER=cray

echo "Using Cray cc, CC, and ftn compilers for all binary builds"

if [ ! -z ${1} ]; then NODETYPE="${1^^}"; fi

if [ -z ${NODETYPE} ]
then
    echo "WARNING - NODETYPE is not set, defaulting to LOGIN, interactive node"
    export NODETYPE="LOGIN"
fi

echo "INFO - Our node type is ${NODETYPE}"

# Set make vars for all builds
# https://www.gnu.org/software/make/manual/html_node/Implicit-Variables.html
export CC="cc"
export CXX="CC"
export CPP="${CC} -E"
export F90="ftn"
export F77="ftn"
export FC="ftn"
#
# Optimize with default level, no floating point opt and vectorization
#export CFLAGS="-O2 -craype-verbose -h fp0,vector0 -DCRAYCE"
#
# Default optimization
#export CFLAGS="-O2 -craype-verbose -DCRAYCE"
#
# Aggresive optimization
#export CFLAGS="-O3 -craype-verbose -DCRAYCE"
#
export CFLAGS="-O2 -craype-verbose -DCRAYCE"
export C_MESSAGES="-h msgs,negmsgs,display_opt"
export CXXFLAGS="${CFLAGS}"
export FFLAGS="-O2"
export FCFLAGS="-O2"
export FFLAGS90="-O2"

# Set addtional variables for builds
export NUMCPUS=$(cat /proc/cpuinfo | grep processor | wc -l | tr -d " ")
export OMP_NUM_THREADS=${NUMCPUS}
export KMP_AFFINITY=disabled

# Unset ENV vars that do not work with Cray
unset CPATH

module list |& grep -- PrgEnv-intel &>/dev/null
if [ $? -eq 0 ]; then module swap PrgEnv-intel PrgEnv-cray; fi
module list |& grep -- PrgEnv-gnu &>/dev/null
if [ $? -eq 0 ]; then module swap PrgEnv-gnu PrgEnv-cray; fi
module list |& grep -- PrgEnv-cray &>/dev/null
if [ $? -ne 0 ]; then module load PrgEnv-cray; fi

if [ "${NODETYPE}" == "LOGIN" ]
then 
    module list |& grep -- craype-haswell &>/dev/null
    if [ $? -eq 0 ]; then module swap craype-haswell craype-sandybridge; fi
    module list |& grep -- craype-sandybridge &>/dev/null
    if [ $? -ne 0 ]; then module load craype-sandybridge; fi
fi

if [ "${NODETYPE}" == "COMPUTE" ] 
then 
    module list |& grep -- craype-sandybridge &>/dev/null
    if [ $? -eq 0 ]; then module swap craype-sandybridge craype-haswell; fi
    module list |& grep -- craype-haswell &>/dev/null
    if [ $? -ne 0 ]; then module load craype-haswell; fi
fi

if [ "${USE_IOBUF^^}" != "YES" ]; then
    module list |& grep -- iobuf &>/dev/null
    if [ $? -eq 0 ]; then module unload ifobuf; fi
    unset IOBUF_PARAMS
    echo "IOBUF is disabled for this build"
else
    module list |& grep -- iobuf &>/dev/null
    if [ $? -ne 0 ]; then module load iobuf; fi
    export IOBUF_PARAMS="*:verbose:size=32M:count=4"
    echo "IOBUF is enabled for this build"
fi

cc -V; CC -V; ftn -V

echo "INFO - Setup env for CRAY build on ${NODETYPE} node" 

