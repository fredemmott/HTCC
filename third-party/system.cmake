set(
  SYSTEM_LIBRARIES
  D2D1
  D3D11
  Dinput8
  Dxguid
  DXGI
)

foreach(LIBRARY ${SYSTEM_LIBRARIES})
  add_library("System::${LIBRARY}" INTERFACE IMPORTED GLOBAL)
  set_property(
    TARGET "System::${LIBRARY}"
    PROPERTY IMPORTED_LIBNAME "${LIBRARY}"
  )
endforeach()
