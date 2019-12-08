##################################################################################
# MIT License                                                                    #
#                                                                                #
# Copyright (c) 2018 Maxime Pinard                                               #
#                                                                                #
# Permission is hereby granted, free of charge, to any person obtaining a copy   #
# of this software and associated documentation files (the "Software"), to deal  #
# in the Software without restriction, including without limitation the rights   #
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      #
# copies of the Software, and to permit persons to whom the Software is          #
# furnished to do so, subject to the following conditions:                       #
#                                                                                #
# The above copyright notice and this permission notice shall be included in all #
# copies or substantial portions of the Software.                                #
#                                                                                #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    #
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  #
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  #
# SOFTWARE.                                                                      #
##################################################################################


function(missing_external varout dependency)
    get_filename_component(dir_path "${CMAKE_SOURCE_DIR}/external/${dependency}" REALPATH)
    if(EXISTS "${dir_path}")
        if(IS_DIRECTORY "${dir_path}")
            file(GLOB files "${dir_path}/*")
            list(LENGTH files len)
            if(len EQUAL 0)
                set(varout true PARENT_SCOPE)
            endif()
        else()
            set(varout true PARENT_SCOPE)
        endif()
    else()
        set(varout true PARENT_SCOPE)
    endif()
    set(varout false PARENT_SCOPE)
endfunction()

function(target_set_output_directory target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Invalid argument: ${target} is not a target")
    endif()
    cmake_parse_arguments(ARGS "" "RUNTIME;LIBRARY;ARCHIVE" "" ${ARGN})
    if(NOT (ARGS_RUNTIME OR ARGS_LIBRARY OR ARGS_ARCHIVE))
        if(${ARGC} GREATER 2)
            message(FATAL_ERROR "Invalid arguments")
        endif()
        set(ARGS_RUNTIME ${ARGS_UNPARSED_ARGUMENTS})
        set(ARGS_LIBRARY ${ARGS_UNPARSED_ARGUMENTS})
        set(ARGS_ARCHIVE ${ARGS_UNPARSED_ARGUMENTS})
    else()
        if(ARGS_UNPARSED_ARGUMENTS)
            message(FATAL_ERROR "Invalid arguments: ${ARGS_UNPARSED_ARGUMENTS}")
        endif()
    endif()

    foreach(type IN ITEMS RUNTIME LIBRARY ARCHIVE)
        if(ARGS_${type})
            set_target_properties(${target} PROPERTIES ${type}_OUTPUT_DIRECTORY ${ARGS_${type}})
            foreach(mode IN ITEMS DEBUG RELWITHDEBINFO RELEASE)
                set_target_properties(${target} PROPERTIES ${type}_OUTPUT_DIRECTORY_${mode} ${ARGS_${type}})
            endforeach()
        endif()
    endforeach()
endfunction()

function(configure_folder input_folder output_folder)
    if(NOT EXISTS ${output_folder})
        file(MAKE_DIRECTORY ${output_folder})
    endif()
    file(GLOB_RECURSE files "${input_folder}/*")
    foreach(file ${files})
        file(RELATIVE_PATH relative_file ${input_folder} ${file})
        configure_file(${file} "${output_folder}/${relative_file}" ${ARGN})
    endforeach()
endfunction()

function(get_files output)
    split_args(dirs "OPTIONS" options ${ARGN})
    set(glob GLOB)
    has_item(has_recurse "recurse" ${options})
    if(has_recurse)
        set(glob GLOB_RECURSE)
    endif()
    set(files)
    foreach(it ${dirs})
        if(IS_DIRECTORY ${it})
            set(patterns
                    "${it}/*.c"
                    "${it}/*.cc"
                    "${it}/*.cpp"
                    "${it}/*.cxx"
                    "${it}/*.h"
                    "${it}/*.hpp"
                    )
            file(${glob} tmp_files ${patterns})
            list(APPEND files ${tmp_files})
            get_filename_component(parent_dir ${it} DIRECTORY)
            group_files(Sources "${parent_dir}" ${tmp_files})
        else()
            list(APPEND files ${it})
            get_filename_component(dir ${it} DIRECTORY)
            group_files(Sources "${dir}" ${it})
        endif()
    endforeach()
    set(${output} ${files} PARENT_SCOPE)
endfunction()

function(split_args left delimiter right)
    set(delimiter_found false)
    set(tmp_left)
    set(tmp_right)
    foreach(it ${ARGN})
        if("${it}" STREQUAL ${delimiter})
            set(delimiter_found true)
        elseif(delimiter_found)
            list(APPEND tmp_right ${it})
        else()
            list(APPEND tmp_left ${it})
        endif()
    endforeach()
    set(${left} ${tmp_left} PARENT_SCOPE)
    set(${right} ${tmp_right} PARENT_SCOPE)
endfunction()

function(has_item output item)
    set(tmp_output false)
    foreach(it ${ARGN})
        if("${it}" STREQUAL "${item}")
            set(tmp_output true)
            break()
        endif()
    endforeach()
    set(${output} ${tmp_output} PARENT_SCOPE)
endfunction()

function(group_files group root)
    foreach(it ${ARGN})
        get_filename_component(dir ${it} PATH)
        file(RELATIVE_PATH relative ${root} ${dir})
        set(local ${group})
        if(NOT "${relative}" STREQUAL "")
            set(local "${group}/${relative}")
        endif()
        # replace '/' and '\' (and repetitions) by '\\'
        string(REGEX REPLACE "[\\\\\\/]+" "\\\\\\\\" local ${local})
        source_group("${local}" FILES ${it})
    endforeach()
endfunction()
