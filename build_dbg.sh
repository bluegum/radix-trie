#!/bin/sh

c_dir=`pwd`
echo ${c_dir}
dbg_dir=${c_dir}/dbg/share
mkdir -p ${dbg_dir}
config_site=${dbg_dir}/config.site
echo 'CFLAGS="-g -O0 -DDEBUG"' > ${config_site}

./configure --prefix=${c_dir}/dbg

make
