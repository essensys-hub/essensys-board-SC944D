################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.local

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS_QUOTED += \
"../Ethernet/Cryptage.c" \
"../Ethernet/Cryptagecencode.c" \
"../Ethernet/Cryptagemd5.c" \
"../Ethernet/Cryptagerijndael_mode.c" \
"../Ethernet/Download.c" \
"../Ethernet/GestionSocket.c" \
"../Ethernet/Json.c" \
"../Ethernet/www.c" \

C_SRCS += \
../Ethernet/Cryptage.c \
../Ethernet/Cryptagecencode.c \
../Ethernet/Cryptagemd5.c \
../Ethernet/Cryptagerijndael_mode.c \
../Ethernet/Download.c \
../Ethernet/GestionSocket.c \
../Ethernet/Json.c \
../Ethernet/www.c \

OBJS += \
./Ethernet/Cryptage_c.obj \
./Ethernet/Cryptagecencode_c.obj \
./Ethernet/Cryptagemd5_c.obj \
./Ethernet/Cryptagerijndael_mode_c.obj \
./Ethernet/Download_c.obj \
./Ethernet/GestionSocket_c.obj \
./Ethernet/Json_c.obj \
./Ethernet/www_c.obj \

OBJS_QUOTED += \
"./Ethernet/Cryptage_c.obj" \
"./Ethernet/Cryptagecencode_c.obj" \
"./Ethernet/Cryptagemd5_c.obj" \
"./Ethernet/Cryptagerijndael_mode_c.obj" \
"./Ethernet/Download_c.obj" \
"./Ethernet/GestionSocket_c.obj" \
"./Ethernet/Json_c.obj" \
"./Ethernet/www_c.obj" \

C_DEPS += \
./Ethernet/Cryptage_c.d \
./Ethernet/Cryptagecencode_c.d \
./Ethernet/Cryptagemd5_c.d \
./Ethernet/Cryptagerijndael_mode_c.d \
./Ethernet/Download_c.d \
./Ethernet/GestionSocket_c.d \
./Ethernet/Json_c.d \
./Ethernet/www_c.d \

OBJS_OS_FORMAT += \
./Ethernet/Cryptage_c.obj \
./Ethernet/Cryptagecencode_c.obj \
./Ethernet/Cryptagemd5_c.obj \
./Ethernet/Cryptagerijndael_mode_c.obj \
./Ethernet/Download_c.obj \
./Ethernet/GestionSocket_c.obj \
./Ethernet/Json_c.obj \
./Ethernet/www_c.obj \

C_DEPS_QUOTED += \
"./Ethernet/Cryptage_c.d" \
"./Ethernet/Cryptagecencode_c.d" \
"./Ethernet/Cryptagemd5_c.d" \
"./Ethernet/Cryptagerijndael_mode_c.d" \
"./Ethernet/Download_c.d" \
"./Ethernet/GestionSocket_c.d" \
"./Ethernet/Json_c.d" \
"./Ethernet/www_c.d" \


# Each subdirectory must supply rules for building sources it contributes
Ethernet/Cryptage_c.obj: ../Ethernet/Cryptage.c
	@echo 'Building file: $<'
	@echo 'Executing target #1 $<'
	@echo 'Invoking: ColdFire Compiler'
	"$(CF_ToolsDirEnv)/mwccmcf" @@"Ethernet/Cryptage.args" -o "Ethernet/Cryptage_c.obj" "$<" -MD -gccdep
	@echo 'Finished building: $<'
	@echo ' '

Ethernet/%.d: ../Ethernet/%.c
	@echo 'Regenerating dependency file: $@'
	
	@echo ' '

Ethernet/Cryptagecencode_c.obj: ../Ethernet/Cryptagecencode.c
	@echo 'Building file: $<'
	@echo 'Executing target #2 $<'
	@echo 'Invoking: ColdFire Compiler'
	"$(CF_ToolsDirEnv)/mwccmcf" @@"Ethernet/Cryptagecencode.args" -o "Ethernet/Cryptagecencode_c.obj" "$<" -MD -gccdep
	@echo 'Finished building: $<'
	@echo ' '

Ethernet/Cryptagemd5_c.obj: ../Ethernet/Cryptagemd5.c
	@echo 'Building file: $<'
	@echo 'Executing target #3 $<'
	@echo 'Invoking: ColdFire Compiler'
	"$(CF_ToolsDirEnv)/mwccmcf" @@"Ethernet/Cryptagemd5.args" -o "Ethernet/Cryptagemd5_c.obj" "$<" -MD -gccdep
	@echo 'Finished building: $<'
	@echo ' '

Ethernet/Cryptagerijndael_mode_c.obj: ../Ethernet/Cryptagerijndael_mode.c
	@echo 'Building file: $<'
	@echo 'Executing target #4 $<'
	@echo 'Invoking: ColdFire Compiler'
	"$(CF_ToolsDirEnv)/mwccmcf" @@"Ethernet/Cryptagerijndael_mode.args" -o "Ethernet/Cryptagerijndael_mode_c.obj" "$<" -MD -gccdep
	@echo 'Finished building: $<'
	@echo ' '

Ethernet/Download_c.obj: ../Ethernet/Download.c
	@echo 'Building file: $<'
	@echo 'Executing target #5 $<'
	@echo 'Invoking: ColdFire Compiler'
	"$(CF_ToolsDirEnv)/mwccmcf" @@"Ethernet/Download.args" -o "Ethernet/Download_c.obj" "$<" -MD -gccdep
	@echo 'Finished building: $<'
	@echo ' '

Ethernet/GestionSocket_c.obj: ../Ethernet/GestionSocket.c
	@echo 'Building file: $<'
	@echo 'Executing target #6 $<'
	@echo 'Invoking: ColdFire Compiler'
	"$(CF_ToolsDirEnv)/mwccmcf" @@"Ethernet/GestionSocket.args" -o "Ethernet/GestionSocket_c.obj" "$<" -MD -gccdep
	@echo 'Finished building: $<'
	@echo ' '

Ethernet/Json_c.obj: ../Ethernet/Json.c
	@echo 'Building file: $<'
	@echo 'Executing target #7 $<'
	@echo 'Invoking: ColdFire Compiler'
	"$(CF_ToolsDirEnv)/mwccmcf" @@"Ethernet/Json.args" -o "Ethernet/Json_c.obj" "$<" -MD -gccdep
	@echo 'Finished building: $<'
	@echo ' '

Ethernet/www_c.obj: ../Ethernet/www.c
	@echo 'Building file: $<'
	@echo 'Executing target #8 $<'
	@echo 'Invoking: ColdFire Compiler'
	"$(CF_ToolsDirEnv)/mwccmcf" @@"Ethernet/www.args" -o "Ethernet/www_c.obj" "$<" -MD -gccdep
	@echo 'Finished building: $<'
	@echo ' '


