find_package(Git REQUIRED)

####################################################################################################
#
# Adds a Git submodule directory to CMake, assuming the Git submodule directory is a CMake project.
#
# Usage in CMakeLists.txt:
#
# include(AddGitSubmodule.cmake)
# add_git_submodule(mysubmod_dir)
#
# Source:
# https://gist.github.com/scivision/bb1d47a9529e153617414e91ff5390af
#
####################################################################################################
function(git_submodule_check dir)
   if(NOT EXISTS "${dir}/CMakeLists.txt")
      execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${dir}
         WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
         COMMAND_ERROR_IS_FATAL ANY
      )
   endif()
   #add_subdirectory(${dir} EXCLUDE_FROM_ALL)
endfunction()
