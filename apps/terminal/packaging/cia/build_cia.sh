#!/bin/zsh
set -euo pipefail

script_dir=${0:A:h}
app_dir=${script_dir:h:h}
tool_dir=${INFINIT3_CIA_TOOLS:-/private/tmp/infinit3-cia-tools}
makerom=${tool_dir}/makerom-x86_64/makerom
bannertool=${tool_dir}/bannertool/mac-x86_64/bannertool
banner_png=${script_dir}/banner.png
banner_bnr=${script_dir}/banner.bnr
banner_aiff=${script_dir}/banner_silence.aiff
banner_wav=${script_dir}/banner_silence.wav
output=${app_dir}/INFINIT3_TERMINAL.cia

if [[ ! -x ${makerom} || ! -x ${bannertool} ]]; then
  print -u2 "CIA tools unavailable. Set INFINIT3_CIA_TOOLS to their directory."
  exit 1
fi

if [[ ! -f ${banner_png} ]]; then
  print -u2 "Static banner image is missing: ${banner_png}"
  exit 1
fi

say -o ${banner_aiff} ''
afconvert -f WAVE -d LEI16 ${banner_aiff} ${banner_wav}
${bannertool} makebanner -i ${banner_png} -a ${banner_wav} -o ${banner_bnr}
${makerom} -f cia -o ${output} -rsf ${script_dir}/INFINIT3_TERMINAL.rsf -target t -exefslogo -elf ${app_dir}/INFINIT3_TERMINAL.elf -icon ${app_dir}/INFINIT3_TERMINAL.smdh -banner ${banner_bnr}
shasum -a 256 ${output}
