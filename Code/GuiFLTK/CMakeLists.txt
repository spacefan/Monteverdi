# Sources of non-templated classes.

file(GLOB OTBGuiFLTK_SRCS "*.cxx" )

add_library(OTBGuiFLTK ${OTBGuiFLTK_SRCS})
target_link_libraries(OTBGuiFLTK ${OTB_LIBRARIES} ${MVD_VISU_GUI_LIBRARIES})
if(Monteverdi_LIBRARY_PROPERTIES)
  set_target_properties(OTBGuiFLTK PROPERTIES ${Monteverdi_LIBRARY_PROPERTIES})
endif()

install(TARGETS OTBGuiFLTK
        EXPORT MonteverdiTargets
        RUNTIME DESTINATION ${Monteverdi_INSTALL_BIN_DIR} COMPONENT RuntimeLibraries
        LIBRARY DESTINATION ${Monteverdi_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
        ARCHIVE DESTINATION ${Monteverdi_INSTALL_LIB_DIR} COMPONENT Development
)

file(GLOB __files1 "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB __files2 "${CMAKE_CURRENT_SOURCE_DIR}/*.txx")
install(FILES ${__files1} ${__files2}
        DESTINATION ${Monteverdi_INSTALL_INCLUDE_DIR}/GuiFLTK
        COMPONENT Development)
