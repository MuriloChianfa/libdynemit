Name:           libdynemit
Version:        %{?version}%{!?version:1.0.0}
Release:        %{?release}%{!?release:1}%{?dist}
Summary:        SIMD dynamic dispatch library

License:        BSL-1.0
URL:            https://github.com/MuriloChianfa/libdynemit
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc >= 13
BuildRequires:  gcc-c++ >= 13
BuildRequires:  cmake >= 3.16

%description
libdynemit provides automatic SIMD runtime dispatch using ifunc resolvers.
Write portable code that automatically uses the best SIMD instructions
(SSE2, SSE4.2, AVX, AVX2, AVX-512F) available on the target CPU.

This package contains the shared libraries needed to run applications
that use libdynemit.

%package        devel
Summary:        Development files for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries, header files, and
pkg-config support for developing applications that use libdynemit.

Features:
- Automatic CPU feature detection at program startup
- Thread-safe SIMD level detection
- Support for GCC 13+ and Clang 16+
- All-in-one library or modular feature libraries
- C23 and C++17+ compatible

%install
# Install shared library (runtime) - all-in-one library only
mkdir -p %{buildroot}%{_libdir}
cp %{_builddir}/build/libdynemit.so.%{version} %{buildroot}%{_libdir}/
ln -s libdynemit.so.%{version} %{buildroot}%{_libdir}/libdynemit.so.1

# Install all static libraries (devel) - all-in-one, core, and features
cp %{_builddir}/build/libdynemit.a %{buildroot}%{_libdir}/
cp %{_builddir}/build/libdynemit_core.a %{buildroot}%{_libdir}/
cp %{_builddir}/build/libdynemit_*.a %{buildroot}%{_libdir}/

# Install development symlink for shared library
ln -s libdynemit.so.1 %{buildroot}%{_libdir}/libdynemit.so

# Install headers
mkdir -p %{buildroot}%{_includedir}/dynemit
install -m 0644 %{_builddir}/include/dynemit.h %{buildroot}%{_includedir}/
install -m 0644 %{_builddir}/include/dynemit/*.h %{buildroot}%{_includedir}/dynemit/

# Install pkg-config file
install -D -m 0644 %{_builddir}/libdynemit.pc \
    %{buildroot}%{_libdir}/pkgconfig/libdynemit.pc

# Install documentation
install -D -m 0644 %{_builddir}/LICENSE \
    %{buildroot}%{_defaultdocdir}/%{name}/LICENSE
install -D -m 0644 %{_builddir}/README.md \
    %{buildroot}%{_defaultdocdir}/%{name}/README.md

%files
%license %{_defaultdocdir}/%{name}/LICENSE
%doc %{_defaultdocdir}/%{name}/README.md
%{_libdir}/libdynemit.so.%{version}
%{_libdir}/libdynemit.so.1

%files devel
%{_libdir}/libdynemit.so
%{_libdir}/libdynemit*.a
%{_includedir}/dynemit.h
%{_includedir}/dynemit/
%{_libdir}/pkgconfig/libdynemit.pc

%changelog
* Fri Feb 20 2026 MuriloChianfa <murilo.chianfa@outlook.com> - 1.2.0-1
- Version 1.2.0 release
- Major refactoring and 18 new feature modules
- Added coverage and mutation testing CI workflows

* Sun Feb 15 2026 MuriloChianfa <murilo.chianfa@outlook.com> - 1.1.0-1
- Version 1.1.0 release
- Updated to latest stable version

* Sat Jan 10 2026 MuriloChianfa <murilo.chianfa@outlook.com> - 1.0.0-1
- Initial RPM package
- Both shared and static libraries
- Runtime and development packages
- Support for SSE2, SSE4.2, AVX, AVX2, and AVX-512F
- Thread-safe CPU feature detection
- C23 and C++17+ compatible
