project(example)

add_executable(zntk_ar zntk_ar.cpp)

target_link_libraries(zntk_ar
    fastplayer_va
)