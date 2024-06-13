function(get_git_version _var _pattern)
    if(NOT GIT_FOUND)
    # to avoid "Could NOT find Git (missing: GIT_EXECUTABLE) use CMAKE_FIND_ROOT_PATH_BOTH"
        find_package(Git QUIET CMAKE_FIND_ROOT_PATH_BOTH)
    endif()

    if(NOT GIT_FOUND)
        set(${_var} "unset" PARENT_SCOPE)
        return()
    endif()
    
    execute_process(COMMAND
        "${GIT_EXECUTABLE}" describe --match "${_pattern}" HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE res
        OUTPUT_VARIABLE out
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(NOT res EQUAL 0)
        set(${_var} "unset" PARENT_SCOPE)
        return()
    endif()
    
    execute_process(COMMAND
        "${GIT_EXECUTABLE}" update-index -q --refresh
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        ERROR_QUIET)
    execute_process(COMMAND
        "${GIT_EXECUTABLE}" diff-index --name-only HEAD --
        RESULT_VARIABLE res
        ERROR_QUIET)
    if(NOT res EQUAL 0)
        set(out "${out}-dirty")
    endif()

    set(${_var} "${out}" PARENT_SCOPE)
endfunction()
