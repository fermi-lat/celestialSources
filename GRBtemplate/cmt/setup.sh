# echo "Setting GRBtemplate v0r1 in /data0/glast/ScienceTools_v5/celestialSources"

if test "${CMTROOT}" = ""; then
  CMTROOT=/data0/glast/extlib/rh9_gcc32/CMT/v1r16p20040701; export CMTROOT
fi
. ${CMTROOT}/mgr/setup.sh

tempfile=`${CMTROOT}/mgr/cmt build temporary_name -quiet`
if test ! $? = 0 ; then tempfile=/tmp/cmt.$$; fi
${CMTROOT}/mgr/cmt -quiet setup -sh -pack=GRBtemplate -version=v0r1 -path=/data0/glast/ScienceTools_v5/celestialSources  $* >${tempfile}; . ${tempfile}
/bin/rm -f ${tempfile}

