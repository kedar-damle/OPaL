printf "build/marc --invalid_flag --output=test.tmp\n"

export LD_LIBRARY_PATH=build/
build/marc --invalid_flag --output=test.tmp
if [[ $? -ne 64 ]] ; then
  exit 1
fi
exit 0