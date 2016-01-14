# table of mtypes


MT_ACQ2006=80
MT_ACQ1001=81
MT_ACQ2106=82

MT_ACQ420=1
MT_ACQ420F=A1
MT_ACQ435=2
MT_ACQ430=3
MT_ACQ424=4
MT_ACQ425=5
MT_ACQ425F=A5
MT_ACQ437=6
MT_ACQ480=8
MT_BOLO8=60
MT_DIO432=61
MT_DIO432P=62
MT_PMODAD1=63
MT_BOLO8B=64
MT_V2F=67
MT_AO420=40
MT_AO424=41


nameFromMT() {
	case $1 in
	$MT_ACQ2006)	echo "acq2006";;
	$MT_ACQ1001)	echo "acq1001";;
	$MT_ACQ2106)	echo "acq2106";;
	$MT_ACQ420)		echo "acq420";;
	$MT_ACQ420F)	echo "acq420";;
	$MT_ACQ435)		echo "acq435";;
	$MT_ACQ430)		echo "acq430";;
	$MT_ACQ424)		echo "acq424";;
	$MT_ACQ425)		echo "acq425";;
	$MT_ACQ425F)	echo "acq425";;
	$MT_ACQ437)		echo "acq437";;
	$MT_ACQ480)		echo "acq480";;
	$MT_BOLO8)		echo "bolo8";;
	$MT_DIO432)		echo "dio432";;
	$MT_DIO432P)	echo "dio432";;
	$MT_PMODAD1)	echo "pmodad1";;
	$MT_BOLO8B)		echo "bolo8B";;
	$MT_V2F)		echo "v2f";;
	$MT_AO420)		echo "ao420";;
	$MT_AO424)		echo "ao424";;
	# no default	
	esac					
}