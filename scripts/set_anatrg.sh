# source me

HN=$(hostname)
set_anatrg() {
	ch=$1; level1=$2; level2=${3:-0}; mode=${4:-1}; grp=${5:-1}; gain=${6}
	site=${SITE:-1}

	if [ -z $level1 ]; then
		echo 'USAGE set_anatrg CH LEVEL1 [LEVEL2 [MODE [GAIN]]]'
	       exit 1	
	fi
	[ -z $gain ] ||	caput $(hostname):${site}:GAIN:${ch} ${gain}
	caput ${HN}:${site}:ANATRG:${ch}:M ${mode}
	caput ${HN}:${site}:ANATRG:${ch}:L1 ${level1}
	caput ${HN}:${site}:ANATRG:${ch}:L2 ${level2}
	chn=${ch##0}
	if [ $chn -gt 16 ]; then
		chn=$(($chn-16))
		gm=GROUP_MASK2
	else
		gm=GROUP_MASK
	fi
	chx=$(printf %x $((chn-1)))
	caput ${HN}:${site}:ANATRG:${gm}.B${chx} $grp
}

