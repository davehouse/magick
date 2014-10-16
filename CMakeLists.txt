CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )

PROJECT( MagickPackage )

# FILE( GLOB GLEW lib/glew/*.c lib/glew/*.h )

IF( APPLE )

    # Create a nibless project per http://blog.hyperjeff.net/code?id=292
    
    FIND_LIBRARY( COCOA_LIBRARY Cocoa )
    
	SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated" )

    SET( PLATFORM_LIBS
        ${COCOA_LIBRARY} )

ELSEIF( UNIX )

    SET( PLATFORM_LIBS
		"`Magick++-config --ldflags --libs`" )

	SET( CMAKE_CXX_FLAGS
		"${CMAKE_CXX_FLAGS}"
		"`Magick++-config --cxxflags --cppflags`" )

ENDIF()

INCLUDE_DIRECTORIES(
)

#ADD_LIBRARY( MagickEcosystem
#)

# INCLUDE_DIRECTORIES( BEFORE lib/vectormath/cpp )
ADD_EXECUTABLE( slice ${CONSOLE_SYSTEM} slice/slice.cpp )
TARGET_LINK_LIBRARIES( slice MagickEcosystem ${PLATFORM_LIBS} )
