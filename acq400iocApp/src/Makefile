TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
# Huge kludge by pgm reaches the parts ..
USR_LDFLAGS += -L $(TOP)/lib/$(T_A)

CXXFLAGS+=-fno-operator-names
#=============================
# Build the IOC application

PROD_IOC = acq400ioc
# acq400ioc.dbd will be created and installed
DBD += acq400ioc.dbd

# acq400ioc.dbd will be made up from these files:
acq400ioc_DBD += base.dbd
acq400ioc_DBD += system.dbd
acq400ioc_DBD += xxAcqHost.dbd

acq400ioc_DBD += sysfs.dbd
acq400ioc_DBD += lxb.dbd
acq400ioc_DBD += acqAi.dbd

# Include dbd files from all support applications:
#acq400ioc_DBD += xxx.dbd
acq400ioc_DBD += asyn.dbd
acq400ioc_DBD += drvAsynIPPort.dbd
acq400ioc_DBD += stream.dbd
#acq400ioc_DBD += seqSoftIoc.dbd
acq400ioc_DBD += sncExample.dbd
acq400ioc_DBD += autoRepeat.dbd
acq400ioc_DBD += aoRangeSet.dbd
acq400ioc_DBD += acq43x_setSampleRate.dbd
acq400ioc_DBD += onSetTransient.dbd
acq400ioc_DBD += mgtStopOnLinkDown.dbd
acq400ioc_DBD += calmon.dbd
acq400ioc_DBD += setMB_ClockMonitor.dbd
acq400ioc_DBD += pvmirror.dbd
acq400ioc_DBD += on_delay.dbd
acq400ioc_DBD += cpsc2_sfp_laneup_monitor.dbd

# Add all the support libraries needed by this IOC
acq400ioc_LIBS += acq2ioc
acq400ioc_LIBS += seq pv
#acq400ioc_LIBS +=fftw3

#PROD_LIBS += stream
#PROD_LIBS += asyn
# acq400ioc_registerRecordDeviceDriver.cpp derives from acq400ioc.dbd
acq400ioc_SRCS += acq400ioc_registerRecordDeviceDriver.cpp

acq400ioc_SRCS += sncExample.st
acq400ioc_SRCS += autoRepeat.st
acq400ioc_SRCS += aoRangeSet.st
acq400ioc_SRCS += aoRangeSet424.st
acq400ioc_SRCS += acq43x_setSampleRate.st
acq400ioc_SRCS += acq43x_setSampleRateIntClk.st
acq400ioc_SRCS += onSetTransient.st
acq400ioc_SRCS += mgtStopOnLinkDown.st
acq400ioc_SRCS += calmon.st
acq400ioc_SRCS += calmon42x.st
acq400ioc_SRCS += calmon480.st
acq400ioc_SRCS += setMB_ClockMonitor.st
acq400ioc_SRCS += v2f_cal.st
acq400ioc_SRCS += pvmirror.st
acq400ioc_SRCS += on_delay.st
acq400ioc_SRCS += cpsc2_sfp_laneup_monitor.st

# Build the main IOC entry point on workstation OSs.
acq400ioc_SRCS_DEFAULT += acq400iocMain.cpp \
    AcqHostDescr.cpp AcqHostDevice.cpp \
    devAiAcqHost.cpp AcqAiHostDescr.cpp \
    devWfAcqHost.cpp AcqWfHostDescr.cpp AcqWfCalc.cpp \
    devBiAcqHost.cpp AcqBiHostDescr.cpp \
    devBoAcqHost.cpp AcqBoHostDescr.cpp \
    devLiAcqHost.cpp AcqLiHostDescr.cpp \
    devLoAcqHost.cpp AcqLoHostDescr.cpp \
    devMbbiAcqHost.cpp AcqMbbiHostDescr.cpp \
    devMbboAcqHost.cpp AcqMbboHostDescr.cpp \
    devWfAWGHost.cpp AcqWfAWGHostDescr.cpp \
    devBoAcqCommand.cpp AcqBoCommandDescr.cpp \
    acq400_caldata.cpp \
    tinyxml2.cpp \
    FileMonitor.cpp VFile.cpp 

acq400ioc_SRCS_linux-arm += bollocks.cpp

acq400ioc_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#acq400ioc_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
acq400ioc_LIBS += $(EPICS_BASE_IOC_LIBS)

PROD_SYS_LIBS_DEFAULT += stream asyn pcre
#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

