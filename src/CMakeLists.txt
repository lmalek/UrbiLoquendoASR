set(URBI_ROOT_DIR "/usr" CACHE PATH "Urbi root directory.")

find_package (Urbi REQUIRED)
find_package (OpenCV REQUIRED)
find_package (Boost REQUIRED)

link_directories (${BOOST_LIBRARYDIR} /opt/Loquendo/LASR/bin)

if (WIN32)
  add_definitions( -DBOOST_ALL_DYN_LINK )
endif (WIN32)

include_directories (${URBI_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} /opt/Loquendo/LASR/includes)

add_library (ULoquendoASR MODULE ULoquendoASR.cpp)

target_link_libraries (ULoquendoASR ${URBI_LIBRARIES} ${OpenCV_LIBS} ${Boost_LIBRARIES} LoqASR)

set_target_properties (ULoquendoASR PROPERTIES
  VERSION 0.0.1
  SOVERSION 0.0.1
  PREFIX "")

install (TARGETS ULoquendoASR  DESTINATION lib/gostai/uobjects COMPONENT libraries)
