ADD_EXECUTABLE(allpix allpix.cpp)
TARGET_LINK_LIBRARIES(allpix AllpixCore)

# FIXME: should not be needed
TARGET_LINK_LIBRARIES(allpix geometry_test)
TARGET_LINK_LIBRARIES(allpix geometry_tgeo)
TARGET_LINK_LIBRARIES(allpix deposition_simple)
TARGET_LINK_LIBRARIES(allpix visualization_test)
TARGET_LINK_LIBRARIES(allpix deposit_reader_test)
TARGET_LINK_LIBRARIES(allpix detector_histogrammer_test)
TARGET_LINK_LIBRARIES(allpix example)

INSTALL(TARGETS allpix
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib)
