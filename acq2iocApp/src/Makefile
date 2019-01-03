TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library

#LIBRARY_IOC += xxxSupport
LIBRARY_IOC += acq2ioc


#=============================
# build an ioc application

#PROD_IOC = acq2ioc
# acq2ioc.dbd will be created and installed
DBD += acq2ioc.dbd

# acq2ioc.dbd will be made up from these files:
acq2ioc_DBD += base.dbd
#include definitions for any other support applications needed
acq2ioc_DBD += sysfs.dbd
acq2ioc_DBD += lxb.dbd
acq2ioc_DBD += acqAi.dbd
#acq2ioc_DBD += ao32_cal.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
acq2ioc_SRCS += acq2ioc_registerRecordDeviceDriver.cpp
#acq2ioc_SRCS_DEFAULT += acq2iocMain.cpp 
acq2ioc_SRCS_vxWorks += -nil-

# Add locally compiled object code

acq2ioc_SRCS += sysfs_ds.cpp 
acq2ioc_SRCS += Sysfs.cpp
acq2ioc_SRCS += devLxb.c
acq2ioc_SRCS += drvLxb.c
acq2ioc_SRCS += devAcqAi.c
acq2ioc_SRCS += drvAcqAi.c
#acq2ioc_SRCS += devAO32_cal.cpp

# The following adds support from base/src/vxWorks
acq2ioc_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

#add a definition for each support application used by this application
#acq2ioc_LIBS += asyn
#acq2ioc_LIBS += ao32
#CROSS_LDFLAGS += -L$(TOP)/lib -lao32
CROSS_LDFLAGS += -L$(TOP)/lib 

# NOTE: To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file

acq2ioc_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

