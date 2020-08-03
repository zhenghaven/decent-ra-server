cmake_minimum_required(VERSION 3.11)

#==========================================================
#   Options for Decent Server Project
#==========================================================
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(APPLE)
	message(FATAL_ERROR "Mac OS is not supported for now.")
endif()

#==========================================================
#   Start Decent Server Project
#==========================================================
project(DecentServer)

#==========================================================
#   Setup options
#==========================================================

set(BUILD_SHARED_LIBS OFF)
set(BUILD_STATIC_LIBS ON)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_NO_SYSTEM_PATHS ON)

if (WIN32)
	macro(get_WIN32_WINNT version)
		if(CMAKE_SYSTEM_VERSION)
			set(ver ${CMAKE_SYSTEM_VERSION})
			string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
			string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
			# Check for Windows 10, b/c we'll need to convert to hex 'A'.
			if("${verMajor}" MATCHES "10")
				set(verMajor "A")
				string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
			endif()
			# Remove all remaining '.' characters.
			string(REPLACE "." "" ver ${ver})
			# Prepend each digit with a zero.
			string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
			set(${version} "0x${ver}")
		endif()
	endmacro()

	get_WIN32_WINNT(win_ver)
	message(STATUS "CMAKE_SYSTEM_VERSION: " "${CMAKE_SYSTEM_VERSION}")
	message(STATUS "_WIN32_WINNT: " "${win_ver}")
	add_definitions(-D_WIN32_WINNT=${win_ver})
endif(WIN32)

if(MSVC)
	set(COMMON_OPTIONS /W3 /wd4996 /we4239 /we4002 /we4700 /we4305 /EHsc /MP /d2FH4-)
	set(DEBUG_OPTIONS /MTd /Od /Zi /DDEBUG)
	set(SIMULATE_OPTIONS /DSIMULATING_ENCLAVE)
	set(RELEASE_OPTIONS /MT /Ox /Oi /Ob2 /fp:fast /GR- /DEDEBUG)# /DNDEBUG
	
	set(COMMON_ENCLAVE_CXX_OPTIONS )
	
	set(ENCLAVE_LINKER_OPTIONS "/NODEFAULTLIB /NOENTRY")
	set(APP_DEBUG_LINKER_OPTIONS "/NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:msvcrtd.lib")
	set(APP_RELEASE_LINKER_OPTIONS "/NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:msvcrtd.lib")
	
	set(COMMON_APP_DEFINES WIN32_LEAN_AND_MEAN CURL_STATICLIB BOOST_DATE_TIME_NO_LIB)
	set(COMMON_ENCLAVE_DEFINES ENCLAVE_ENVIRONMENT)
	
	set(Additional_Sys_Lib )
else()
	set(COMMON_OPTIONS -pthread)
	set(DEBUG_OPTIONS -O0 -g -DDEBUG -UNDEBUG -UEDEBUG)
	set(SIMULATE_OPTIONS -DSIMULATING_ENCLAVE)
	set(RELEASE_OPTIONS -O2 -DEDEBUG -UDEBUG) #-DNDEBUG defined by default
	
	set(COMMON_ENCLAVE_CXX_OPTIONS -std=c++11)
	
	set(ENCLAVE_LINKER_OPTIONS "")
	set(APP_DEBUG_LINKER_OPTIONS "")
	set(APP_RELEASE_LINKER_OPTIONS "")
	
	set(COMMON_APP_DEFINES CURL_STATICLIB BOOST_DATE_TIME_NO_LIB)
	set(COMMON_ENCLAVE_DEFINES ENCLAVE_ENVIRONMENT)
	
	set(Additional_Sys_Lib rt pthread)
endif()

set(DEBUG_OPTIONS ${COMMON_OPTIONS} ${DEBUG_OPTIONS})
set(DEBUGSIM_OPTIONS ${COMMON_OPTIONS} ${DEBUG_OPTIONS} ${SIMULATE_OPTIONS})
set(RELEASE_OPTIONS ${COMMON_OPTIONS} ${RELEASE_OPTIONS})

if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")
endif()

add_compile_options(
	"$<$<CONFIG:Debug>:${DEBUG_OPTIONS}>"
	"$<$<CONFIG:DebugSimulation>:${DEBUGSIM_OPTIONS}>"
	"$<$<CONFIG:Release>:${RELEASE_OPTIONS}>"
)

#Remove all standard libraries dependency here so that enclave DLL can be 
# compiled properly. And it will be added back later for non-enclave apps.
set(COMMON_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES_INIT}")
separate_arguments(COMMON_STANDARD_LIBRARIES)
set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_C_STANDARD_LIBRARIES "")

if(MSVC) 
	#Removed Basic Runtime Checks in MSVC
	STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
	STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
endif()

set(CMAKE_CXX_FLAGS_DEBUGSIMULATION ${CMAKE_CXX_FLAGS_DEBUG})
set(CMAKE_C_FLAGS_DEBUGSIMULATION ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_EXE_LINKER_FLAGS_DEBUGSIMULATION ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
set(CMAKE_SHARED_LINKER_FLAGS_DEBUGSIMULATION ${CMAKE_SHARED_LINKER_FLAGS_DEBUG})
set(CMAKE_STATIC_LINKER_FLAGS_DEBUGSIMULATION ${CMAKE_STATIC_LINKER_FLAGS_DEBUG})

set(CMAKE_CONFIGURATION_TYPES Release Debug DebugSimulation)
set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug DebugSimulation)

#-------
# Flag results:
#-------
message(STATUS "Final set of flags")
message(STATUS "==================================================")
message(STATUS "CMake Standard Libraries:")
message(STATUS "${CMAKE_CXX_STANDARD_LIBRARIES_INIT}")
message(STATUS "CMake CXX Flags:")
message(STATUS "${CMAKE_CXX_FLAGS}")
message(STATUS "CMake CXX Flags (Debug):")
message(STATUS "${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMake CXX Flags (Release):")
message(STATUS "${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMake CXX Link Flags:")
message(STATUS "${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMake CXX Link Flags (Debug):")
message(STATUS "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
message(STATUS "CMake CXX Link Flags (Release):")
message(STATUS "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
message(STATUS "==================================================")
message(STATUS "")

#==========================================================
#   Add submodules
#==========================================================

##Header only libraries
set(TCLAP_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/libs/tclap/include)


set(DECENT_API_DECENT_SERVER ON CACHE BOOL "Add decent server module." FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/decent-ra-api)

#Intel SGX SDK
include(${CMAKE_CURRENT_LIST_DIR}/libs/decent-ra-api/cmake/FindSgxSdk.cmake)

#==========================================================
#   Add files
#==========================================================

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_LIST_DIR}/sources/*.[ch]* ${CMAKE_CURRENT_LIST_DIR}/sources/*.edl)

# DecentServer Files:
set(SOURCEDIR_App ${CMAKE_CURRENT_LIST_DIR}/sources/app)
set(SOURCEDIR_Enclave ${CMAKE_CURRENT_LIST_DIR}/sources/enclave)
file(GLOB_RECURSE SOURCES_App ${SOURCEDIR_App}/*.[ch]*)
file(GLOB_RECURSE SOURCES_Enclave ${SOURCEDIR_Enclave}/*.[ch]*)
file(GLOB_RECURSE SOURCES_EDL ${SOURCEDIR_Enclave}/*.edl)

set(SOURCEDIR_SGX_Enabler ${CMAKE_CURRENT_LIST_DIR}/sources/SGX_Enabler)
file(GLOB_RECURSE SOURCES_SGX_Enabler ${SOURCEDIR_SGX_Enabler}/*.[ch]*)

set(SOURCEDIR_IasSim ${CMAKE_CURRENT_LIST_DIR}/sources/IasSim)
file(GLOB_RECURSE SOURCES_IasSim ${SOURCEDIR_IasSim}/*.[ch]*)

#==========================================================
#   Setup filters
#==========================================================

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/sources FILES ${SOURCES})

#==========================================================
#   G++ linking flags
#==========================================================

if(MSVC)
	set(WHOLE_ARCHIVE_FLAG_BEGIN "")
	set(WHOLE_ARCHIVE_FLAG_END "")
	set(GROUP_FLAG_BEGIN "")
	set(GROUP_FLAG_END "")
else()
	set(WHOLE_ARCHIVE_FLAG_BEGIN -Wl,--whole-archive)
	set(WHOLE_ARCHIVE_FLAG_END -Wl,--no-whole-archive)
	set(GROUP_FLAG_BEGIN -Wl,--start-group)
	set(GROUP_FLAG_END -Wl,--end-group)
endif()

if(MSVC)
	set(Enclave_Bin_Path "${CMAKE_BINARY_DIR}/$<CONFIG>")
else()
	set(Enclave_Bin_Path "${CMAKE_BINARY_DIR}")
endif()

#==========================================================
#   Enclave Projects
#==========================================================

set(Proj_Name DecentServer)

###########################################################
### EDL
###########################################################

set(EDL_T_SOURCE_OUTPUT ${SOURCEDIR_Enclave}/Enclave_t.h ${SOURCEDIR_Enclave}/Enclave_t.c)
set(EDL_U_SOURCE_OUTPUT ${SOURCEDIR_App}/Enclave_u.h ${SOURCEDIR_App}/Enclave_u.c)

add_custom_command(OUTPUT ${EDL_T_SOURCE_OUTPUT}
	COMMAND "${INTEL_SGX_EDGER_PATH}"  
	--trusted "${SOURCEDIR_Enclave}/Enclave.edl" 
	--search-path "${SOURCEDIR_Enclave}" 
	--search-path "${DECENT_API_EDL_DIR}" 
	--search-path "${INTEL_SGX_SDK_INCLUDE_DIR}"
	WORKING_DIRECTORY "${SOURCEDIR_Enclave}"
	DEPENDS "${SOURCEDIR_Enclave}/Enclave.edl"
	COMMENT "Processing EDL for enclave..."
)

add_custom_command(OUTPUT ${EDL_U_SOURCE_OUTPUT}
	COMMAND "${INTEL_SGX_EDGER_PATH}"  
	--untrusted "${SOURCEDIR_Enclave}/Enclave.edl" 
	--search-path "${SOURCEDIR_Enclave}" 
	--search-path "${DECENT_API_EDL_DIR}" 
	--search-path "${INTEL_SGX_SDK_INCLUDE_DIR}"
	WORKING_DIRECTORY "${SOURCEDIR_App}"
	DEPENDS "${SOURCEDIR_Enclave}/Enclave.edl"
	COMMENT "Processing EDL for app..."
)

add_custom_target(${Proj_Name}_EDL DEPENDS ${EDL_T_SOURCE_OUTPUT} ${EDL_U_SOURCE_OUTPUT})
set_target_properties(${Proj_Name}_EDL PROPERTIES FOLDER "DecentServer")

###########################################################
### Enclave
###########################################################

set(${Proj_Name}_Enclave_File "${CMAKE_SHARED_LIBRARY_PREFIX}${Proj_Name}_Enclave$<$<CONFIG:Debug>:${CMAKE_DEBUG_POSTFIX}>.signed${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(${Proj_Name}_Enclave_Lib "${CMAKE_SHARED_LIBRARY_PREFIX}${Proj_Name}_Enclave$<$<CONFIG:Debug>:${CMAKE_DEBUG_POSTFIX}>${CMAKE_SHARED_LIBRARY_SUFFIX}")

add_library(${Proj_Name}_Enclave SHARED ${SOURCES_COMMON} ${SOURCES_COMMON_EDL} ${SOURCES_COMMON_ENCLAVE} ${SOURCES_Enclave} ${SOURCES_EDL})
#defines:
target_compile_definitions(${Proj_Name}_Enclave PRIVATE ${COMMON_ENCLAVE_DEFINES})
#compiler flags:
target_compile_options(${Proj_Name}_Enclave PRIVATE ${INTEL_SGX_SDK_C_FLAGS} $<$<COMPILE_LANGUAGE:CXX>:${INTEL_SGX_SDK_CXX_FLAGS} ${COMMON_ENCLAVE_CXX_OPTIONS}>)
#linker flags:
set_target_properties(${Proj_Name}_Enclave PROPERTIES LINK_FLAGS "${ENCLAVE_LINKER_OPTIONS} ${INTEL_SGX_SDK_LINKER_FLAGS_T}")
set_target_properties(${Proj_Name}_Enclave PROPERTIES FOLDER "DecentServer")

add_custom_command(TARGET ${Proj_Name}_Enclave
	POST_BUILD
	COMMAND "${INTEL_SGX_SIGNER_PATH}" sign 
	-key "${CMAKE_CURRENT_LIST_DIR}/Enclave_private.pem" 
	-enclave "${Enclave_Bin_Path}/${${Proj_Name}_Enclave_Lib}" 
	-out "${CMAKE_BINARY_DIR}/${${Proj_Name}_Enclave_File}" 
	-config "${SOURCEDIR_Enclave}/Enclave.config.xml"
)

target_link_libraries(${Proj_Name}_Enclave 
	${WHOLE_ARCHIVE_FLAG_BEGIN} 
	IntelSGX::Trusted::switchless 
	IntelSGX::Trusted::rts 
	${WHOLE_ARCHIVE_FLAG_END}
	${GROUP_FLAG_BEGIN}
	IntelSGX::Trusted::stdc 
	IntelSGX::Trusted::cxx 
	IntelSGX::Trusted::service 
	IntelSGX::Trusted::key_exchange 
	IntelSGX::Trusted::crypto 
	IntelSGX::Trusted::file_system 
	DecentRa_Server_Enclave 
	mbedcrypto_enclave 
	mbedx509_enclave 
	mbedtls_enclave 
	${GROUP_FLAG_END}
)

add_dependencies(${Proj_Name}_Enclave ${Proj_Name}_EDL)

###########################################################
### App
###########################################################

add_executable(${Proj_Name}_App ${SOURCES_COMMON} ${SOURCES_COMMON_EDL} ${SOURCES_COMMON_APP} ${SOURCES_App} ${SOURCES_EDL})
#includes:
target_include_directories(${Proj_Name}_App PRIVATE ${TCLAP_INCLUDE_DIR})
#defines:
target_compile_definitions(${Proj_Name}_App PRIVATE ${COMMON_APP_DEFINES} ENCLAVE_FILENAME="${${Proj_Name}_Enclave_File}" TOKEN_FILENAME="${Proj_Name}_Enclave.token")
#linker flags:
set_target_properties(${Proj_Name}_App PROPERTIES LINK_FLAGS_DEBUG "${APP_DEBUG_LINKER_OPTIONS}")
set_target_properties(${Proj_Name}_App PROPERTIES LINK_FLAGS_DEBUGSIMULATION "${APP_DEBUG_LINKER_OPTIONS}")
set_target_properties(${Proj_Name}_App PROPERTIES LINK_FLAGS_RELEASE "${APP_RELEASE_LINKER_OPTIONS}")
set_target_properties(${Proj_Name}_App PROPERTIES FOLDER "DecentServer")

target_link_libraries(${Proj_Name}_App 
	${COMMON_STANDARD_LIBRARIES} 
	IntelSGX::Untrusted::Libs
	DecentRa_Server_App 
	jsoncpp_lib_static 
	libcurl 
	mbedtls 
	Boost::filesystem
	Boost::system
	${Additional_Sys_Lib}
)

add_dependencies(${Proj_Name}_App ${Proj_Name}_Enclave)

###########################################################
### SGX_Enabler
###########################################################

add_executable(SGX_Enabler ${SOURCES_SGX_Enabler})
#includes:
target_include_directories(SGX_Enabler PRIVATE ${TCLAP_INCLUDE_DIR})
#defines:
target_compile_definitions(SGX_Enabler PRIVATE ${COMMON_APP_DEFINES})
#linker flags:
set_target_properties(SGX_Enabler PROPERTIES LINK_FLAGS_DEBUG "${APP_DEBUG_LINKER_OPTIONS}")
set_target_properties(SGX_Enabler PROPERTIES LINK_FLAGS_DEBUGSIMULATION "${APP_DEBUG_LINKER_OPTIONS}")
set_target_properties(SGX_Enabler PROPERTIES LINK_FLAGS_RELEASE "${APP_RELEASE_LINKER_OPTIONS}")
set_target_properties(SGX_Enabler PROPERTIES FOLDER "Enabler")

target_link_libraries(SGX_Enabler
	${COMMON_STANDARD_LIBRARIES}
	IntelSGX::Untrusted::capable
	${Additional_Sys_Lib}
)

###########################################################
### IAS Simulator
###########################################################

add_executable(IasSimulator ${SOURCES_IasSim})
#includes:
target_include_directories(IasSimulator PRIVATE ${TCLAP_INCLUDE_DIR})
#defines:
target_compile_definitions(IasSimulator PRIVATE ${COMMON_APP_DEFINES})
#linker flags:
set_target_properties(IasSimulator PROPERTIES LINK_FLAGS_DEBUG "${APP_DEBUG_LINKER_OPTIONS}")
set_target_properties(IasSimulator PROPERTIES LINK_FLAGS_DEBUGSIMULATION "${APP_DEBUG_LINKER_OPTIONS}")
set_target_properties(IasSimulator PROPERTIES LINK_FLAGS_RELEASE "${APP_RELEASE_LINKER_OPTIONS}")
set_target_properties(IasSimulator PROPERTIES FOLDER "IasSimulator")

target_link_libraries(IasSimulator
	${COMMON_STANDARD_LIBRARIES}
	DecentRa_Server_App
	${Additional_Sys_Lib}
)
