AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

EXTRA_DIST = \
	data/EcoliK12.fasta \
	data/ecoli.header.jf27 \
	data/kat.hist \
	data/sect_length_test.fa \
	data/sect_test.fa \
	data/ecoli_r1.1K.fastq \
	data/ecoli_r2.1K.fastq \
	data/unknown.dat \
	test_comp.sh \
	test_gcp.sh \
	test_hist.sh \
	test_sect.sh

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	-rm -rf temp

# additional include paths necessary to compile the C++ library
AM_CXXFLAGS = \
	-fwrapv -m64 -Wall -Wno-deprecated-declarations -Wno-unused-function \
	-Wno-unused-parameter -Wno-unused-variable -Wno-missing-field-initializers \
	-Wno-sign-compare \
	@AM_CXXFLAGS@

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib/include \
	-I$(top_srcdir)/deps/seqan-library-2.0.0/include \
	-I$(top_srcdir)/deps/jellyfish-2.2.0/include \
	-I$(top_srcdir)/deps/boost/build/include \
	-DDATADIR=\"$(srcdir)/data\" \
	-DKAT_SITE_PKGS='"$(libdir)/python$(PYTHON_VERSION)/site-packages"' \
	@AM_CPPFLAGS@

TEST_EXTENSIONS = .sh
SH_LOG_COMPILER = $(SHELL)
AM_SH_LOG_FLAGS =

TESTS = check_unit_tests test_hist.sh test_gcp.sh test_sect.sh test_comp.sh

check_PROGRAMS = check_unit_tests

noinst_HEADERS = \
	gtest/gtest.h \
	gtest/src/gtest-all.cc \
	gtest/src/gtest_main.cc


check_unit_tests_SOURCES = \
	check_jellyfish.cc \
	check_spectra_helper.cc \
	check_compcounters.cc \
	check_main.cc

check_unit_tests_LDFLAGS = \
	-static \
	-L. \
	-L../lib \
	-L../deps/jellyfish-2.2.0 \
	-L../deps/boost/build/lib \
	@AM_LDFLAGS@

check_unit_tests_LDADD = \
	libgtest.la \
	-lgtest \
	-lkat \
	-lkat_jellyfish \
	../deps/boost/build/lib/libboost_timer.a \
    ../deps/boost/build/lib/libboost_chrono.a \
    ../deps/boost/build/lib/libboost_filesystem.a \
    ../deps/boost/build/lib/libboost_program_options.a \
    ../deps/boost/build/lib/libboost_system.a \
	@AM_LIBS@

include gtest.mk
