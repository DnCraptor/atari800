add_library(usbhid INTERFACE)
target_sources(usbhid INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/usbhid.c
)
target_link_libraries(usbhid INTERFACE tinyusb_host)
target_include_directories(usbhid INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
)
