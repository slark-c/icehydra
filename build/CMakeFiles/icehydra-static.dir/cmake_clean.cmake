file(REMOVE_RECURSE
  "libicehydra.a"
  "libicehydra.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/icehydra-static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
