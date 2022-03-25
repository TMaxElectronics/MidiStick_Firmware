#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-Debug.mk)" "nbproject/Makefile-local-Debug.mk"
include nbproject/Makefile-local-Debug.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=Debug
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/Synth.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/Synth.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

ifdef SUB_IMAGE_ADDRESS

else
SUB_IMAGE_ADDRESS_COMMAND=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=usb_descriptors.c usb_device.c usb_events.c usb_device_hid.c usb_device_audio.c main.c MidiController.c ConfigManager.c UART32.c ADSREngine.c SignalGenerator.c VMSRoutines.c mapper.c HIDController.c DLL.c DutyCompressor.c app_device_audio_microphone.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/usb_descriptors.o ${OBJECTDIR}/usb_device.o ${OBJECTDIR}/usb_events.o ${OBJECTDIR}/usb_device_hid.o ${OBJECTDIR}/usb_device_audio.o ${OBJECTDIR}/main.o ${OBJECTDIR}/MidiController.o ${OBJECTDIR}/ConfigManager.o ${OBJECTDIR}/UART32.o ${OBJECTDIR}/ADSREngine.o ${OBJECTDIR}/SignalGenerator.o ${OBJECTDIR}/VMSRoutines.o ${OBJECTDIR}/mapper.o ${OBJECTDIR}/HIDController.o ${OBJECTDIR}/DLL.o ${OBJECTDIR}/DutyCompressor.o ${OBJECTDIR}/app_device_audio_microphone.o
POSSIBLE_DEPFILES=${OBJECTDIR}/usb_descriptors.o.d ${OBJECTDIR}/usb_device.o.d ${OBJECTDIR}/usb_events.o.d ${OBJECTDIR}/usb_device_hid.o.d ${OBJECTDIR}/usb_device_audio.o.d ${OBJECTDIR}/main.o.d ${OBJECTDIR}/MidiController.o.d ${OBJECTDIR}/ConfigManager.o.d ${OBJECTDIR}/UART32.o.d ${OBJECTDIR}/ADSREngine.o.d ${OBJECTDIR}/SignalGenerator.o.d ${OBJECTDIR}/VMSRoutines.o.d ${OBJECTDIR}/mapper.o.d ${OBJECTDIR}/HIDController.o.d ${OBJECTDIR}/DLL.o.d ${OBJECTDIR}/DutyCompressor.o.d ${OBJECTDIR}/app_device_audio_microphone.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/usb_descriptors.o ${OBJECTDIR}/usb_device.o ${OBJECTDIR}/usb_events.o ${OBJECTDIR}/usb_device_hid.o ${OBJECTDIR}/usb_device_audio.o ${OBJECTDIR}/main.o ${OBJECTDIR}/MidiController.o ${OBJECTDIR}/ConfigManager.o ${OBJECTDIR}/UART32.o ${OBJECTDIR}/ADSREngine.o ${OBJECTDIR}/SignalGenerator.o ${OBJECTDIR}/VMSRoutines.o ${OBJECTDIR}/mapper.o ${OBJECTDIR}/HIDController.o ${OBJECTDIR}/DLL.o ${OBJECTDIR}/DutyCompressor.o ${OBJECTDIR}/app_device_audio_microphone.o

# Source Files
SOURCEFILES=usb_descriptors.c usb_device.c usb_events.c usb_device_hid.c usb_device_audio.c main.c MidiController.c ConfigManager.c UART32.c ADSREngine.c SignalGenerator.c VMSRoutines.c mapper.c HIDController.c DLL.c DutyCompressor.c app_device_audio_microphone.c



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-Debug.mk dist/${CND_CONF}/${IMAGE_TYPE}/Synth.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=32MX270F256B
MP_LINKER_FILE_OPTION=
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/usb_descriptors.o: usb_descriptors.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/usb_descriptors.o.d 
	@${RM} ${OBJECTDIR}/usb_descriptors.o 
	@${FIXDEPS} "${OBJECTDIR}/usb_descriptors.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/usb_descriptors.o.d" -o ${OBJECTDIR}/usb_descriptors.o usb_descriptors.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/usb_device.o: usb_device.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/usb_device.o.d 
	@${RM} ${OBJECTDIR}/usb_device.o 
	@${FIXDEPS} "${OBJECTDIR}/usb_device.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/usb_device.o.d" -o ${OBJECTDIR}/usb_device.o usb_device.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/usb_events.o: usb_events.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/usb_events.o.d 
	@${RM} ${OBJECTDIR}/usb_events.o 
	@${FIXDEPS} "${OBJECTDIR}/usb_events.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/usb_events.o.d" -o ${OBJECTDIR}/usb_events.o usb_events.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/usb_device_hid.o: usb_device_hid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/usb_device_hid.o.d 
	@${RM} ${OBJECTDIR}/usb_device_hid.o 
	@${FIXDEPS} "${OBJECTDIR}/usb_device_hid.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/usb_device_hid.o.d" -o ${OBJECTDIR}/usb_device_hid.o usb_device_hid.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/usb_device_audio.o: usb_device_audio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/usb_device_audio.o.d 
	@${RM} ${OBJECTDIR}/usb_device_audio.o 
	@${FIXDEPS} "${OBJECTDIR}/usb_device_audio.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/usb_device_audio.o.d" -o ${OBJECTDIR}/usb_device_audio.o usb_device_audio.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/main.o: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	@${FIXDEPS} "${OBJECTDIR}/main.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/main.o.d" -o ${OBJECTDIR}/main.o main.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/MidiController.o: MidiController.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/MidiController.o.d 
	@${RM} ${OBJECTDIR}/MidiController.o 
	@${FIXDEPS} "${OBJECTDIR}/MidiController.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/MidiController.o.d" -o ${OBJECTDIR}/MidiController.o MidiController.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/ConfigManager.o: ConfigManager.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/ConfigManager.o.d 
	@${RM} ${OBJECTDIR}/ConfigManager.o 
	@${FIXDEPS} "${OBJECTDIR}/ConfigManager.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/ConfigManager.o.d" -o ${OBJECTDIR}/ConfigManager.o ConfigManager.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/UART32.o: UART32.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/UART32.o.d 
	@${RM} ${OBJECTDIR}/UART32.o 
	@${FIXDEPS} "${OBJECTDIR}/UART32.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/UART32.o.d" -o ${OBJECTDIR}/UART32.o UART32.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/ADSREngine.o: ADSREngine.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/ADSREngine.o.d 
	@${RM} ${OBJECTDIR}/ADSREngine.o 
	@${FIXDEPS} "${OBJECTDIR}/ADSREngine.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/ADSREngine.o.d" -o ${OBJECTDIR}/ADSREngine.o ADSREngine.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/SignalGenerator.o: SignalGenerator.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/SignalGenerator.o.d 
	@${RM} ${OBJECTDIR}/SignalGenerator.o 
	@${FIXDEPS} "${OBJECTDIR}/SignalGenerator.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/SignalGenerator.o.d" -o ${OBJECTDIR}/SignalGenerator.o SignalGenerator.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/VMSRoutines.o: VMSRoutines.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/VMSRoutines.o.d 
	@${RM} ${OBJECTDIR}/VMSRoutines.o 
	@${FIXDEPS} "${OBJECTDIR}/VMSRoutines.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/VMSRoutines.o.d" -o ${OBJECTDIR}/VMSRoutines.o VMSRoutines.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/mapper.o: mapper.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/mapper.o.d 
	@${RM} ${OBJECTDIR}/mapper.o 
	@${FIXDEPS} "${OBJECTDIR}/mapper.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/mapper.o.d" -o ${OBJECTDIR}/mapper.o mapper.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/HIDController.o: HIDController.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/HIDController.o.d 
	@${RM} ${OBJECTDIR}/HIDController.o 
	@${FIXDEPS} "${OBJECTDIR}/HIDController.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/HIDController.o.d" -o ${OBJECTDIR}/HIDController.o HIDController.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/DLL.o: DLL.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/DLL.o.d 
	@${RM} ${OBJECTDIR}/DLL.o 
	@${FIXDEPS} "${OBJECTDIR}/DLL.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/DLL.o.d" -o ${OBJECTDIR}/DLL.o DLL.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/DutyCompressor.o: DutyCompressor.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/DutyCompressor.o.d 
	@${RM} ${OBJECTDIR}/DutyCompressor.o 
	@${FIXDEPS} "${OBJECTDIR}/DutyCompressor.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/DutyCompressor.o.d" -o ${OBJECTDIR}/DutyCompressor.o DutyCompressor.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/app_device_audio_microphone.o: app_device_audio_microphone.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/app_device_audio_microphone.o.d 
	@${RM} ${OBJECTDIR}/app_device_audio_microphone.o 
	@${FIXDEPS} "${OBJECTDIR}/app_device_audio_microphone.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/app_device_audio_microphone.o.d" -o ${OBJECTDIR}/app_device_audio_microphone.o app_device_audio_microphone.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
else
${OBJECTDIR}/usb_descriptors.o: usb_descriptors.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/usb_descriptors.o.d 
	@${RM} ${OBJECTDIR}/usb_descriptors.o 
	@${FIXDEPS} "${OBJECTDIR}/usb_descriptors.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/usb_descriptors.o.d" -o ${OBJECTDIR}/usb_descriptors.o usb_descriptors.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/usb_device.o: usb_device.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/usb_device.o.d 
	@${RM} ${OBJECTDIR}/usb_device.o 
	@${FIXDEPS} "${OBJECTDIR}/usb_device.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/usb_device.o.d" -o ${OBJECTDIR}/usb_device.o usb_device.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/usb_events.o: usb_events.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/usb_events.o.d 
	@${RM} ${OBJECTDIR}/usb_events.o 
	@${FIXDEPS} "${OBJECTDIR}/usb_events.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/usb_events.o.d" -o ${OBJECTDIR}/usb_events.o usb_events.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/usb_device_hid.o: usb_device_hid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/usb_device_hid.o.d 
	@${RM} ${OBJECTDIR}/usb_device_hid.o 
	@${FIXDEPS} "${OBJECTDIR}/usb_device_hid.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/usb_device_hid.o.d" -o ${OBJECTDIR}/usb_device_hid.o usb_device_hid.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/usb_device_audio.o: usb_device_audio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/usb_device_audio.o.d 
	@${RM} ${OBJECTDIR}/usb_device_audio.o 
	@${FIXDEPS} "${OBJECTDIR}/usb_device_audio.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/usb_device_audio.o.d" -o ${OBJECTDIR}/usb_device_audio.o usb_device_audio.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/main.o: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	@${FIXDEPS} "${OBJECTDIR}/main.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/main.o.d" -o ${OBJECTDIR}/main.o main.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/MidiController.o: MidiController.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/MidiController.o.d 
	@${RM} ${OBJECTDIR}/MidiController.o 
	@${FIXDEPS} "${OBJECTDIR}/MidiController.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/MidiController.o.d" -o ${OBJECTDIR}/MidiController.o MidiController.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/ConfigManager.o: ConfigManager.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/ConfigManager.o.d 
	@${RM} ${OBJECTDIR}/ConfigManager.o 
	@${FIXDEPS} "${OBJECTDIR}/ConfigManager.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/ConfigManager.o.d" -o ${OBJECTDIR}/ConfigManager.o ConfigManager.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/UART32.o: UART32.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/UART32.o.d 
	@${RM} ${OBJECTDIR}/UART32.o 
	@${FIXDEPS} "${OBJECTDIR}/UART32.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/UART32.o.d" -o ${OBJECTDIR}/UART32.o UART32.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/ADSREngine.o: ADSREngine.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/ADSREngine.o.d 
	@${RM} ${OBJECTDIR}/ADSREngine.o 
	@${FIXDEPS} "${OBJECTDIR}/ADSREngine.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/ADSREngine.o.d" -o ${OBJECTDIR}/ADSREngine.o ADSREngine.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/SignalGenerator.o: SignalGenerator.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/SignalGenerator.o.d 
	@${RM} ${OBJECTDIR}/SignalGenerator.o 
	@${FIXDEPS} "${OBJECTDIR}/SignalGenerator.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/SignalGenerator.o.d" -o ${OBJECTDIR}/SignalGenerator.o SignalGenerator.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/VMSRoutines.o: VMSRoutines.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/VMSRoutines.o.d 
	@${RM} ${OBJECTDIR}/VMSRoutines.o 
	@${FIXDEPS} "${OBJECTDIR}/VMSRoutines.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/VMSRoutines.o.d" -o ${OBJECTDIR}/VMSRoutines.o VMSRoutines.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/mapper.o: mapper.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/mapper.o.d 
	@${RM} ${OBJECTDIR}/mapper.o 
	@${FIXDEPS} "${OBJECTDIR}/mapper.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/mapper.o.d" -o ${OBJECTDIR}/mapper.o mapper.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/HIDController.o: HIDController.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/HIDController.o.d 
	@${RM} ${OBJECTDIR}/HIDController.o 
	@${FIXDEPS} "${OBJECTDIR}/HIDController.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/HIDController.o.d" -o ${OBJECTDIR}/HIDController.o HIDController.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/DLL.o: DLL.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/DLL.o.d 
	@${RM} ${OBJECTDIR}/DLL.o 
	@${FIXDEPS} "${OBJECTDIR}/DLL.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/DLL.o.d" -o ${OBJECTDIR}/DLL.o DLL.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/DutyCompressor.o: DutyCompressor.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/DutyCompressor.o.d 
	@${RM} ${OBJECTDIR}/DutyCompressor.o 
	@${FIXDEPS} "${OBJECTDIR}/DutyCompressor.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/DutyCompressor.o.d" -o ${OBJECTDIR}/DutyCompressor.o DutyCompressor.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
${OBJECTDIR}/app_device_audio_microphone.o: app_device_audio_microphone.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/app_device_audio_microphone.o.d 
	@${RM} ${OBJECTDIR}/app_device_audio_microphone.o 
	@${FIXDEPS} "${OBJECTDIR}/app_device_audio_microphone.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"usb lib" -MMD -MF "${OBJECTDIR}/app_device_audio_microphone.o.d" -o ${OBJECTDIR}/app_device_audio_microphone.o app_device_audio_microphone.c    -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mdfp=${DFP_DIR}
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/Synth.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -g   -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/Synth.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)   -mreserve=data@0x0:0x1FC -mreserve=boot@0x1FC00490:0x1FC00BEF  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D=__DEBUG_D,--defsym=_min_heap_size=50000,--defsym=_min_stack_size=10000,--no-code-in-dinit,--no-dinit-in-serial-mem,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--cref,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml -mdfp=${DFP_DIR}
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/Synth.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/Synth.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_Debug=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=_min_heap_size=50000,--defsym=_min_stack_size=10000,--no-code-in-dinit,--no-dinit-in-serial-mem,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--cref,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml -mdfp=${DFP_DIR}
	${MP_CC_DIR}\\xc32-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/Synth.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/Debug
	${RM} -r dist/Debug

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
