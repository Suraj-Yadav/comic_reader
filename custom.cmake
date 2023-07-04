function(parse_git_tag)
    execute_process(
        COMMAND git describe --tags
        OUTPUT_VARIABLE GIT_TAG
    )
    set(VERSION_STRING "Unknown" PARENT_SCOPE)
    if(NOT ${GIT_TAG} STREQUAL "")
        string(STRIP ${GIT_TAG} GIT_TAG_1)
        set(VERSION_STRING ${GIT_TAG_1} PARENT_SCOPE)
    endif()
endfunction()
