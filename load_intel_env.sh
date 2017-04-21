#!/bin/bash
# source this script to setup compiler ENV

export BUILD=intel
export COMPILER="intel"

echo "Using Intel icc, icpc, and ifort compilers for all binary builds"

if [ ! -z ${1} ]; then NODETYPE="${1^^}"; fi

if [ -z ${NODETYPE} ]
then
    echo "WARNING - NODETYPE is not set, defaulting to WS, workstation"
    export NODETYPE="WS"
fi

echo "INFO - Our node type is ${NODETYPE}"

if [ "${NODETYPE}" == "COMPUTE" ] || [ "${NODETYPE}" == "LOGIN" ]
then  
    if [ -z ${USE_IOBUF} ]
    then
	echo "INFO - USE_IOBUF is not set, defaulting to NO"
	export USE_IOBUF="NO"
    fi
    # NOTE: Do not invoke the Intel compilers directly through the 
    # NOTE: icc, icpc, or ifort commands. The resulting executable 
    # NOTE: will not run on the Cray XT series system.
    # NOTE: Do not invoke the Intel compilers directly through the 
    # NOTE: icc, icpc, or ifort commands. The resulting executable 
    # NOTE: will not run on the Cray XT series system.
    # Set make vars for all builds
    # https://www.gnu.org/software/make/manual/html_node/Implicit-Variables.html
    export CC="cc"
    export CXX="CC"
    export CPP="${CC} -E"
    export F90="ftn"
    export F77="ftn"
    export FC="ftn"
    export CFLAGS="-v -Wall -O2 -axCore-AVX2"
    export CXXFLAGS="-v -Wall -O2 -axCore-AVX2"
    export FFLAGS="-v -warn all -O2 -mp1 -assume buffered_io -axCore-AVX2"
    export FCFLAGS="-v -warn all -O2 -mp1 -assume buffered_io -axCore-AVX2"
    export FFLAGS90="-v -warn all -O2 -mp1 -assume buffered_io -axCore-AVX2"
    
    # Set addtional variables for builds
    export NUMCPUS=$(cat /proc/cpuinfo | grep processor | wc -l | tr -d " ")
    export OMP_NUM_THREADS=${NUMCPUS}
    export KMP_AFFINITY=disabled

    module list |& grep -- PrgEnv-gnu &>/dev/null
    if [ $? -eq 0 ]; then module swap PrgEnv-gnu PrgEnv-intel; fi
    module list |& grep -- PrgEnv-cray &>/dev/null
    if [ $? -eq 0 ]; then module swap PrgEnv-cray PrgEnv-intel; fi
    module list |& grep -- PrgEnv-intel &>/dev/null
    if [ $? -ne 0 ]; then module load PrgEnv-intel; fi
    module list |& grep -- craype-haswell &>/dev/null
    if [ $? -eq 0 ]; then module swap craype-haswell craype-sandybridge; fi
    module list |& grep -- craype-sandybridge &>/dev/null
    if [ $? -ne 0 ]; then module load craype-sandybridge; fi

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
    echo "Using Cray wrappers for icc, icpc, and ifort"
    echo "Intel build supports both sandybridge and haswell, with haswell optimizations"
fi

if [ "${NODETYPE}" == "WS" ]
then
    source /opt/intel/bin/iccvars.sh -arch intel64 -platform linux
    unset IOBUF_PARAMS
    export CC="icc"
    export CXX="icpc"
    export F90="ifort"
    export F77="ifort"
    export FC="ifort"
    export CFLAGS="-v -Wall -O2"
    export CXXFLAGS="-v -Wall -O2"
    export FFLAGS="-v -warn all -O2 -mp1 -assume buffered_io"
    export FCFLAGS="-v -warn all -O2 -mp1 -assume buffered_io"
    export FFLAGS90="-v -warn all -O2 -mp1 -assume buffered_io"
    export NUMCPUS=$(cat /proc/cpuinfo | grep processor | wc -l | tr -d " ")
    export OMP_NUM_THREADS=${NUMCPUS}
    export OMP_PROC_BIND="true"
    export FORT_BUFFERED="true"
    export KMP_AFFINITY="granularity=fine,compact,1,0,verbose"
fi

echo "INFO - Setup env for INTEL build on ${NODETYPE} node" 
