#!/usr/bin/env bash

shell_session_update() {
  echo "invoked shell_session_update"
}

rts_brew_install() {
  if ! brew ls --version $1 &>/dev/null; then
	  travis_retry brew install $1
	fi
}

rts_before_install() {
	set -x
	export BOOST_TXZ="boost_1_64_0.tar.xz"
  export CTEST_EXT_COLOR_OUTPUT=TRUE

  if [[ "${COMPILER}" != "" ]]; then
    export CXX="${COMPILER}"
  fi

	cmake --version
  ${CXX} -v
  python --version
	set +x
}

rts_install() {
	set -x
  mkdir -p opt/downloads
  mkdir -p opt/cmake
  mkdir -p build/opt/boost-cmake

  if [[ -f "opt/downloads/${BOOST_TXZ}" ]]; then # fetch boost from cache
    cp "opt/downloads/${BOOST_TXZ}" "build/opt/boost-cmake/${BOOST_TXZ}"
  fi

  if [[ "${TRAVIS_OS_NAME}" == "linux" ]]; then # fetch cmake from cache/network
    if [[ ! -d "${TRAVIS_BUILD_DIR}/opt/cmake/bin" ]]; then
      CMAKE_URL="https://cmake.org/files/v3.7/cmake-3.7.2-Linux-x86_64.tar.gz"
      travis_retry wget --no-check-certificate --quiet -O - "${CMAKE_URL}" | tar --strip-components=1 -xz -C opt/cmake
    fi
    export PATH="${TRAVIS_BUILD_DIR}/opt/cmake/bin:${PATH}"
  else
    rts_brew_install cmake
  fi 

  cmake --version

  if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then # fetch doxygen
    rts_brew_install doxygen
  fi
	set +x
}

rts_script() {
  # pushd/popd are done outside of set -x, otherwise we wind up tracing the travis cd hook on osx
	echo "++pushd build"
  pushd build

	set -x
  cmake -DCMAKE_CXX_COMPILER="${CXX}" .. ${CMAKE_OPTIONS}
  cmake --build . -- ${BUILD_OPTIONS}
  ctest --output-on-failure --parallel 4
	
  if [[ ! -f "../opt/downloads/${BOOST_TXZ}" && -f "opt/boost-cmake/${BOOST_TXZ}" ]]; then # cache boost
    cp "opt/boost-cmake/${BOOST_TXZ}" "../opt/downloads/${BOOST_TXZ}"
  fi
	set +x
	
	echo "++popd"
	popd
}

rts_trap() {
  set -e
  $*
  set +e
}