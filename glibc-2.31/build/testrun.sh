#!/bin/bash
builddir=`dirname "$0"`
GCONV_PATH="${builddir}/iconvdata"

usage () {
  echo "usage: $0 [--tool=strace] PROGRAM [ARGUMENTS...]" 2>&1
  echo "       $0 --tool=valgrind PROGRAM [ARGUMENTS...]" 2>&1
  exit 1
}

toolname=default
while test $# -gt 0 ; do
  case "$1" in
    --tool=*)
      toolname="${1:7}"
      shift
      ;;
    --*)
      usage
      ;;
    *)
      break
      ;;
  esac
done

if test $# -eq 0 ; then
  usage
fi

case "$toolname" in
  default)
    exec   env GCONV_PATH="${builddir}"/iconvdata LOCPATH="${builddir}"/localedata LC_ALL=C  "${builddir}"/elf/ld-linux-x86-64.so.2 --library-path "${builddir}":"${builddir}"/math:"${builddir}"/elf:"${builddir}"/dlfcn:"${builddir}"/nss:"${builddir}"/nis:"${builddir}"/rt:"${builddir}"/resolv:"${builddir}"/mathvec:"${builddir}"/support:"${builddir}"/crypt:"${builddir}"/nptl ${1+"$@"}
    ;;
  strace)
    exec strace  -EGCONV_PATH=/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/iconvdata  -ELOCPATH=/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/localedata  -ELC_ALL=C  /home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/elf/ld-linux-x86-64.so.2 --library-path /home/ubuntu/opencilk-project/cheetah/glibc-2.31/build:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/math:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/elf:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/dlfcn:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/nss:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/nis:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/rt:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/resolv:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/mathvec:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/support:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/crypt:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/nptl ${1+"$@"}
    ;;
  valgrind)
    exec env GCONV_PATH=/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/iconvdata LOCPATH=/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/localedata LC_ALL=C valgrind  /home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/elf/ld-linux-x86-64.so.2 --library-path /home/ubuntu/opencilk-project/cheetah/glibc-2.31/build:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/math:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/elf:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/dlfcn:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/nss:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/nis:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/rt:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/resolv:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/mathvec:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/support:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/crypt:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/nptl ${1+"$@"}
    ;;
  container)
    exec env GCONV_PATH=/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/iconvdata LOCPATH=/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/localedata LC_ALL=C  /home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/elf/ld-linux-x86-64.so.2 --library-path /home/ubuntu/opencilk-project/cheetah/glibc-2.31/build:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/math:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/elf:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/dlfcn:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/nss:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/nis:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/rt:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/resolv:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/mathvec:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/support:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/crypt:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/nptl /home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/support/test-container env GCONV_PATH=/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/iconvdata LOCPATH=/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/localedata LC_ALL=C  /home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/elf/ld-linux-x86-64.so.2 --library-path /home/ubuntu/opencilk-project/cheetah/glibc-2.31/build:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/math:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/elf:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/dlfcn:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/nss:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/nis:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/rt:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/resolv:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/mathvec:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/support:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/crypt:/home/ubuntu/opencilk-project/cheetah/glibc-2.31/build/nptl ${1+"$@"}
    ;;
  *)
    usage
    ;;
esac
