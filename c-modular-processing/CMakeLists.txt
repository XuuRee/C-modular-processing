cmake_minimum_required(VERSION 3.5)
project(hw04)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g -Wall -Wextra -pedantic")

set(HW04_MODULE_SOURCE module-cache.c module-decorate.c module-magic.c module-tolower.c module-toupper.c)
set(HW04_SOURCE main.c config.c log.c query.c)
set(HW04_MODULE_HEADERS module.h module-cache.h module-decorate.h module-magic.h module-tolower.h module-toupper.h)
set(HW04_HEADERS config.h functions.h log.h query.h)
add_executable(hw04 ${HW04_SOURCE} ${HW04_MODULE_SOURCE} ${HW04_MODULE_HEADERS} ${HW04_HEADERS})
target_compile_definitions(hw04 PRIVATE __USE_MINGW_ANSI_STDIO=1)

