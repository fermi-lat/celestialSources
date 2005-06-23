if ( $?CMTROOT == 0 ) then
  setenv CMTROOT /data1/glast/extlib/rh9_gcc32/CMT/v1r16p20040701
endif
source ${CMTROOT}/mgr/setup.csh
set tempfile=`${CMTROOT}/mgr/cmt build temporary_name -quiet`
if $status != 0 then
  set tempfile=/tmp/cmt.$$
endif
${CMTROOT}/mgr/cmt -quiet cleanup -csh -pack=GRBtemplate -version= -path=/data1/glast/GLASTRELEASE_HEAD/celestialSources $* >${tempfile}; source ${tempfile}
/bin/rm -f ${tempfile}

