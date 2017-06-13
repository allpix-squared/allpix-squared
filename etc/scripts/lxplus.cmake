# Set Eigen3 location
SET(Eigen3_DIR "$ENV{Eigen3_DIR}" CACHE PATH "Path to Eigen3 CMake script")

# Set CLHEP locations
# FIXME: This should not be a fixed directory...
SET(CLHEP_ROOT_DIR "/cvmfs/sft.cern.ch/lcg/releases/clhep/2.3.1.1-6ba0c/x86_64-slc6-gcc62-opt/")
SET(CLHEP_INCLUDE_DIR "${CLHEP_ROOT_DIR}/include" CACHE PATH "CLHEP include directory" FORCE)
SET(CLHEP_LIBRARY "${CLHEP_ROOT_DIR}/lib/libCLHEP.so" CACHE PATH "CLHEP library directory" FORCE)
