# Reuse the AES/SHA-1 implementation already tracked by the pinned RecompCore.
# Build only these local-disc primitives, not another TLS or networking library.
if(NOT TARGET bluewake_wwhd_disc_crypto)
    get_filename_component(_wwhd_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
    get_filename_component(_wwhd_runtime "${GXRUNTIME_DIR}/.." ABSOLUTE)
    set(_wwhd_mbed "${_wwhd_runtime}/Externals/mbedtls")
    add_library(bluewake_wwhd_disc_crypto STATIC
        ${_wwhd_root}/runtime/host/src/wwhd/disc_crypto.cpp
        ${_wwhd_mbed}/library/aes.c
        ${_wwhd_mbed}/library/sha1.c
        ${_wwhd_mbed}/library/platform_util.c)
    target_compile_features(bluewake_wwhd_disc_crypto PRIVATE cxx_std_17)
    target_include_directories(bluewake_wwhd_disc_crypto PRIVATE
        ${_wwhd_root}/runtime/host/src ${_wwhd_mbed}/include)
    target_compile_definitions(bluewake_wwhd_disc_crypto PRIVATE
        MBEDTLS_CONFIG_FILE="wwhd/disc_crypto_config.h")
    set_target_properties(bluewake_wwhd_disc_crypto PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
