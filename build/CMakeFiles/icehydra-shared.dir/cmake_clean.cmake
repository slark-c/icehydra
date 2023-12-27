file(REMOVE_RECURSE
  "libicehydra.pdb"
  "libicehydra.so"
  "libicehydra.so.0"
  "libicehydra.so.0.4"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/icehydra-shared.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
