# Makefile.am for Spinnaker4 Device Adapter
# Build configuration for Micro-Manager Autoconf/Automake build system

# Include paths
AM_CPPFLAGS = \
	-I$(top_srcdir)/MMDevice \
	$(SPINNAKER_CPPFLAGS) \
	$(MMDEVAPI_CFLAGS)

# Link flags
AM_LDFLAGS = \
	-module \
	-avoid-version \
	-no-undefined \
	$(SPINNAKER_LDFLAGS)

# Device adapter library
deviceadapter_LTLIBRARIES = libmmgr_dal_Spinnaker4.la

# Source files
libmmgr_dal_Spinnaker4_la_SOURCES = \
	Spinnaker4Camera.cpp \
	Spinnaker4Camera.h

# Link libraries
libmmgr_dal_Spinnaker4_la_LIBADD = \
	$(MMDEVAPI_LIBS) \
	$(SPINNAKER_LIBS)

# Additional linker flags for macOS
if HAVE_DARWIN
libmmgr_dal_Spinnaker4_la_LDFLAGS += \
	-framework CoreFoundation \
	-framework IOKit \
	-weak_framework System
endif
