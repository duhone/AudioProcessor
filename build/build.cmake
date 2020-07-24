set(root "${CMAKE_CURRENT_LIST_DIR}/..")

set(SRCS
    ${root}/src/main.cpp
)

set(BUILD
    ${root}/build/build.cmake
)

set(TEST_FILES
    ${root}/test_assets/BGM_Menu.wav
)

add_executable(AudioProcessor  
	${SRCS} 
	${BUILD}
	${TEST_FILES}
)

settingsCR(AudioProcessor)
usePCH(AudioProcessor core)
			
target_link_libraries(AudioProcessor 
	cli11
	fmt
	opus
	spdlog
	core
	platform
)

source_group("Test Files" FILES ${TEST_FILES})
	
add_custom_command(TARGET AudioProcessor POST_BUILD       
  COMMAND ${CMAKE_COMMAND} -E copy_if_different  
      ${TEST_FILES}
      $<TARGET_FILE_DIR:AudioProcessor>) 
		