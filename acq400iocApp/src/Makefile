TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
# Huge kludge by pgm reaches the parts ..
USR_LDFLAGS += -L $(TOP)/lib/$(T_A)
#=============================
# Build the IOC application

PROD_IOC = acq400ioc
# acq400ioc.dbd will be created and installed
DBD += acq400ioc.dbd

# acq400ioc.dbd will be made up from these files:
acq400ioc_DBD += base.dbd

# Include dbd files from all support applications:
#acq400ioc_DBD += xxx.dbd
acq400ioc_DBD += asyn.dbd
acq400ioc_DBD += drvAsynIPPort.dbd
acq400ioc_DBD += stream.dbd

# Add all the support libraries needed by this IOC
acq400ioc_LIBS += 

#PROD_LIBS += stream
#PROD_LIBS += asyn
# acq400ioc_registerRecordDeviceDriver.cpp derives from acq400ioc.dbd
acq400ioc_SRCS += acq400ioc_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
acq400ioc_SRCS_DEFAULT += acq400iocMain.cpp
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

