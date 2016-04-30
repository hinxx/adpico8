< envPaths

errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/pico8DemoApp.dbd")
pico8DemoApp_registerRecordDeviceDriver(pdbbase) 

# Prefix for all records
epicsEnvSet("PREFIX", "PICO8:")
# The port name for the detector
epicsEnvSet("PORT",   "PICO8")
# The queue size for all plugins
epicsEnvSet("QSIZE",  "20")
# The maximim image width; used for row profiles in the NDPluginStats plugin
#epicsEnvSet("XSIZE",  "2000")
#epicsEnvSet("XSIZE",  "1000")
#epicsEnvSet("XSIZE",  "200")
epicsEnvSet("XSIZE",  "1048576")
# The maximim image height; used for column profiles in the NDPluginStats plugin
#epicsEnvSet("YSIZE",  "1500")
#epicsEnvSet("YSIZE",  "1000")
#epicsEnvSet("YSIZE",  "200")
epicsEnvSet("YSIZE",  "1")
# The maximum number of time series points in the NDPluginStats plugin
epicsEnvSet("NCHANS", "2048")
# The maximum number of frames buffered in the NDPluginCircularBuff plugin
epicsEnvSet("CBUFFS", "500")
# The search path for database files
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")

asynSetMinTimerPeriod(0.001)

# The EPICS environment variable EPICS_CA_MAX_ARRAY_BYTES needs to be set to a value at least as large
# as the largest image that the standard arrays plugin will send.
# That vlaue is $(XSIZE) * $(YSIZE) * sizeof(FTVL data type) for the FTVL used when loading the NDStdArrays.template file.
# The variable can be set in the environment before running the IOC or it can be set here.
# It is often convenient to set it in the environment outside the IOC to the largest array any client 
# or server will need.  For example 10000000 (ten million) bytes may be enough.
# If it is set here then remember to also set it outside the IOC for any CA clients that need to access the waveform record.  
# Do not set EPICS_CA_MAX_ARRAY_BYTES to a value much larger than that required, because EPICS Channel Access actually
# allocates arrays of this size every time it needs a buffer larger than 16K.
# Uncomment the following line to set it in the IOC.
epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", "10000000")

# Create a pico8 driver
# Pico8Configure(const char *portName, int maxBuffers, int maxMemory,
#             const char *devicePath, int priority, int stackSize)
Pico8Configure("$(PORT)", 0, 0, "/dev/amc_pico", 0, 0)
dbLoadRecords("$(PICO8)/db/pico8.db",       "P=$(PREFIX),R=acq:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
#dbLoadRecords("$(PICO8)/db/pico8_ch.db",    "P=$(PREFIX),R=acq:,PORT=$(PORT),ADDR=0,TIMEOUT=1,NELEMENTS=$(XSIZE)")
dbLoadRecords("$(PICO8)/db/pico8_ch.db",    "P=$(PREFIX),R=ch1:,PORT=$(PORT),ADDR=1,TIMEOUT=1,NELEMENTS=$(XSIZE)")
dbLoadRecords("$(PICO8)/db/pico8_ch.db",    "P=$(PREFIX),R=ch2:,PORT=$(PORT),ADDR=2,TIMEOUT=1,NELEMENTS=$(XSIZE)")
dbLoadRecords("$(PICO8)/db/pico8_ch.db",    "P=$(PREFIX),R=ch3:,PORT=$(PORT),ADDR=3,TIMEOUT=1,NELEMENTS=$(XSIZE)")
dbLoadRecords("$(PICO8)/db/pico8_ch.db",    "P=$(PREFIX),R=ch4:,PORT=$(PORT),ADDR=4,TIMEOUT=1,NELEMENTS=$(XSIZE)")
dbLoadRecords("$(PICO8)/db/pico8_ch.db",    "P=$(PREFIX),R=ch5:,PORT=$(PORT),ADDR=5,TIMEOUT=1,NELEMENTS=$(XSIZE)")
dbLoadRecords("$(PICO8)/db/pico8_ch.db",    "P=$(PREFIX),R=ch6:,PORT=$(PORT),ADDR=6,TIMEOUT=1,NELEMENTS=$(XSIZE)")
dbLoadRecords("$(PICO8)/db/pico8_ch.db",    "P=$(PREFIX),R=ch7:,PORT=$(PORT),ADDR=7,TIMEOUT=1,NELEMENTS=$(XSIZE)")
dbLoadRecords("$(PICO8)/db/pico8_ch.db",    "P=$(PREFIX),R=ch8:,PORT=$(PORT),ADDR=8,TIMEOUT=1,NELEMENTS=$(XSIZE)")

# Load an NDFile database.  This is not supported for the simDetector which does not write files.
#dbLoadRecords("NDFile.template","P=$(PREFIX),R=acq:,PORT=PICO8,ADDR=0,TIMEOUT=1")

NDStdArraysConfigure("Trace1", 3, 0, "$(PORT)", 0)
dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=trace1:,PORT=Trace1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Float32,FTVL=FLOAT,NELEMENTS=$(XSIZE)")

# Create a standard arrays plugin, set it to get data from first simDetector driver.
#NDStdArraysConfigure("Image1", 3, 0, "$(PORT)", 0)

# This creates a waveform large enough for 2000x2000x3 (e.g. RGB color) arrays.
# This waveform only allows transporting 8-bit images
#dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int8,FTVL=UCHAR,NELEMENTS=12000000")
#dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int8,FTVL=UCHAR,NELEMENTS=3000000")
# This waveform only allows transporting 16-bit images
#dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int16,FTVL=USHORT,NELEMENTS=12000000")
# This waveform allows transporting 32-bit images
#dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int32,FTVL=LONG,NELEMENTS=12000000")

# Create a standard arrays plugin, set it to get data from second simDetector driver.
#NDStdArraysConfigure("Image2", 1, 0, "SIM2", 0)

# This creates a waveform large enough for 640x480x3 (e.g. RGB color) arrays.
# This waveform allows transporting 64-bit images, so it can handle any detector data type at the expense of more memory and bandwidth
#dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=image2:,PORT=Image2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM2,TYPE=Float64,FTVL=DOUBLE,NELEMENTS=921600")


#NDFitsConfigure("FITS1", $(QSIZE), 0, "$(PORT)", 0, 3, 0, 0, 0)
#dbLoadRecords("$(ADPLUGINFITS)/db/NDFits.template",     "P=$(PREFIX),R=Fits1:,   PORT=FITS1,ADDR=0,TIMEOUT=1,XSIZE=$(XSIZE),YSIZE=$(YSIZE),NCHANS=$(NCHANS),NDARRAY_PORT=$(PORT)")
#dbLoadRecords("$(ADPLUGINFITS)/db/NDFitsN.template",    "P=$(PREFIX),R=Fits1:1:, PORT=FITS1,ADDR=0,TIMEOUT=1,NCHANS=$(NCHANS)")
#dbLoadRecords("$(ADPLUGINFITS)/db/NDFitsN.template",    "P=$(PREFIX),R=Fits1:2:, PORT=FITS1,ADDR=1,TIMEOUT=1,NCHANS=$(NCHANS)")
#dbLoadRecords("$(ADPLUGINFITS)/db/NDFitsN.template",    "P=$(PREFIX),R=Fits1:3:, PORT=FITS1,ADDR=2,TIMEOUT=1,NCHANS=$(NCHANS)")


# Load all other plugins using commonPlugins.cmd
#< $(ADCORE)/iocBoot/commonPlugins.cmd
#set_requestfile_path("$(ADEXAMPLE)/exampleApp/Db")
#set_requestfile_path("$(ADPLUGINFITS)/adpluginfitsApp/Db")

asynSetTraceIOMask("$(PORT)",0,255)
asynSetTraceMask("$(PORT)",0,255)
#asynSetTraceIOMask("FileNetCDF",0,2)
#asynSetTraceMask("FileNetCDF",0,255)
#asynSetTraceMask("FileNexus",0,255)
#asynSetTraceMask("SIM2",0,255)

iocInit()

# save things every thirty seconds
#create_monitor_set("auto_settings.req", 30, "P=$(PREFIX)")

