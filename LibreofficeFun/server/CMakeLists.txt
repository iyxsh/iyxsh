cmake_minimum_required(VERSION 3.12)
project(libreofficefun VERSION 0.1)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置默认构建类型（单配置生成器）
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Debug 模式配置
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -ggdb")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -ggdb")

# Release 模式配置
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

# 平台相关定义
if(WIN32)
    add_definitions(-DWIN32 -D_WINDOWS)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
elseif(APPLE)
    add_definitions(-DMACOS)
elseif(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES "Android")
        add_definitions(-DANDROID)
    else()
        add_definitions(-DLINUX)
        # GCC/Clang 编译器：指定输入和执行编码
        add_compile_options(-finput-charset=UTF-8)
        add_compile_options(-fexec-charset=UTF-8)
    endif()
endif()

# 源文件
set(SOURCES
    ./leofun/cJSON/cJSON.c
    ./leofun/logger/logger.cpp
    ./leofun/config/json_config.cpp
    ./leofun/error/error_codes.cpp
    ./leofun/filemanager/utils.cpp
    ./leofun/filemanager/ErrorHandler.cpp
    ./leofun/filemanager/unicodetable.cpp
    ./leofun/filemanager/LibreOfficeService.cpp
    ./leofun/filemanager/spreadsheet.cpp
    ./leofun/filemanager/DocumentManager.cpp
    ./leofun/filemanager/filehandlers.cpp
    ./leofun/filemanager/filequeue.cpp
    ./leofun/filemanager/apiops.cpp
    ./leofun/apihandle/handle.cpp
    ./leofun/filemanager/template_index_cache.cpp
    ./leofun/leofun.cpp
)

# 头文件
set(HEADERS
    ./leofun/cJSON/cJSON.h
    ./leofun/logger/logger.h
    ./leofun/config/json_config.h
    ./leofun/error/error_codes.h
    ./leofun/filemanager/utils.h
    ./leofun/filemanager/ErrorHandler.h
    ./leofun/filemanager/unicodetable.h
    ./leofun/filemanager/LibreOfficeService.h
    ./leofun/filemanager/spreadsheet.h
    ./leofun/filemanager/DocumentManager.h
    ./leofun/filemanager/filehandlers.h
    ./leofun/filemanager/filequeue.h
    ./leofun/filemanager/apiops.h
    ./leofun/apihandle/handle.h  
    ./leofun/filemanager/template_index_cache.h    
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${GUI_TYPE} ${SOURCES} ${HEADERS})

# 设置包含目录
# 链接系统库
if(WIN32) # windows
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ./include/Windows/libreoffice)
    set(UNO_LIB_PATH "/usr/local/lib/libreoffice/sdk/lib")
    # LibreOffice UNO C++ 库
    target_link_libraries(${PROJECT_NAME}
        ${UNO_LIB_PATH}/uno_cppuhelper.lib
        ${UNO_LIB_PATH}/uno_cppu.lib
        ${UNO_LIB_PATH}/uno_sal.lib
    )
    #find_library(SWE_LIB NAMES swedll32 PATHS ${UNO_LIB_PATH})
    #target_link_libraries(${PROJECT_NAME} ${UNO_LIB_PATH}/swedll32.lib)
elseif(APPLE) #macos
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ./include/Apple/libreoffice)
    set(UNO_LIB_PATH "/usr/local/lib/libreoffice/sdk/lib")
    # LibreOffice UNO C++ 库
    target_link_libraries(${PROJECT_NAME}
        ${UNO_LIB_PATH}/libuno_cppuhelper.dylib
        ${UNO_LIB_PATH}/libuno_cppu.dylib
        ${UNO_LIB_PATH}/libuno_sal.dylib
    )
    #find_library(SWE_LIB NAMES swedll32 PATHS ${UNO_LIB_PATH})
    #target_link_libraries(${PROJECT_NAME} ${UNO_LIB_PATH}/libswe.a)
elseif(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES "Android")
        target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ./include/Android/libreoffice)
        set(UNO_LIB_PATH "/usr/local/lib/libreoffice/sdk/lib")
        # LibreOffice UNO C++ 库
        target_link_libraries(${PROJECT_NAME}
            ${UNO_LIB_PATH}/libuno_cppuhelpergcc3.so.3
            ${UNO_LIB_PATH}/libuno_cppu.so.3
            ${UNO_LIB_PATH}/libuno_sal.so.3
        )
        #find_library(SWE_LIB NAMES swedll32 PATHS ${UNO_LIB_PATH})
        #target_link_libraries(${PROJECT_NAME} ${UNO_LIB_PATH}/swedll32.a)
    else()    # linux
        #set(UNO_LIB_PATH "/usr/local/lib/libreoffice/sdk/lib")
        target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ./include/FreeBSD/libreoffice ./sdk/FreeBSD/sdk/cppheaders)

        add_definitions(-DLINUX)
        # 添加编译选项
        add_compile_options(-finput-charset=UTF-8)
        add_compile_options(-fexec-charset=UTF-8)
        
        # 查找必需的系统库
        # 如有自定义 OpenSSL 路径请取消下一行注释并设置路径
        # set(OPENSSL_ROOT_DIR "/usr/local/ssl")
        find_package(OpenSSL REQUIRED)
        find_package(Threads REQUIRED)
        message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
        message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
        # 优先查找常见的 libreoffice program 路径
        find_library(UNO_CPPUHELPER NAMES uno_cppuhelper libuno_cppuhelpergcc3.so.3 HINTS /usr/local/lib/libreoffice/program /usr/lib /usr/local/lib)
        find_library(UNO_CPPU NAMES uno_cppu libuno_cppu.so.3 HINTS /usr/local/lib/libreoffice/program /usr/lib /usr/local/lib)
        find_library(UNO_SAL NAMES uno_sal libuno_sal.so.3 HINTS /usr/local/lib/libreoffice/program /usr/lib /usr/local/lib)
        message(STATUS "UNO_CPPUHELPER: ${UNO_CPPUHELPER}")
        message(STATUS "UNO_CPPU: ${UNO_CPPU}")
        message(STATUS "UNO_SAL: ${UNO_SAL}")
        if(NOT UNO_CPPUHELPER OR NOT UNO_CPPU OR NOT UNO_SAL)
            message(FATAL_ERROR "Could not find LibreOffice UNO libraries. Please install LibreOffice and set the correct path in CMakeLists.txt!")
        endif()
        target_link_libraries(${PROJECT_NAME} 
            OpenSSL::SSL
            OpenSSL::Crypto
            ${CMAKE_THREAD_LIBS_INIT}  # 添加pthread支持
            m  # 添加数学库支持
            ${UNO_CPPUHELPER}
            ${UNO_CPPU}
            ${UNO_SAL}
        )
    endif()
endif()
# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
    LIBRARY DESTINATION lib
)
