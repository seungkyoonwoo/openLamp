###########################################
# Makefile for SPEED MMDBMS 0.0.1
###########################################

.KEEP_STATE:

###########################################
# Environment setting
###########################################
-include config.mk

DB_LIB_NAME=openml
export DB_LIB_NAME

ifeq ($(COMPILE_MODE),mode64)
    B=64
    OPTION += -D_64BIT
else
    B=32
endif

# for gnu make
OSNAME := $(shell /bin/uname)
NODENAME := $(shell /bin/uname -n)

ifeq ($(OSNAME),SunOS)
    WHOAMI := $(shell /usr/ucb/whoami)
else
    WHOAMI := $(shell /usr/bin/whoami)
endif

ifeq ($(OSNAME),SunOS)
    ifeq ($(COMPILE_MODE),mode64)
        ifeq ($(COMPILER),cc)
            CCBIN = "/opt/SUNWspro/bin/cc -xarch=v9 -mt -xCC -D_64BIT"
            LDBIN = "/usr/ccs/bin/ld -64 -G -z textoff"
            CMODE_DBG = "-g -D_REENTRANT -DMDB_DEBUG $(OPTION)"
            CMODE_REL = "-xO5 -D_REENTRANT -UMDB_DEBUG $(OPTION)"
            CMODE_PIC = -KPIC
            SYSLIBS = "-L/usr/lib/64 -lpthread -lsocket -lnsl -lm"
        else ifeq ($(COMPILER),gcc)
            CCBIN = "gcc -m64 -D_64BIT -D_GCC"
            LDBIN = "/usr/ccs/bin/ld -64 -G"
            CMODE_DBG = "-g -Wall -Wreturn-type -D_REENTRANT -DMDB_DEBUG $(OPTION)"
            CMODE_REL = "-O3 -D_REENTRANT -UMDB_DEBUG $(OPTION)"
            CMODE_PIC = -fPIC
            SYSLIBS = "-L/usr/lib/64 -lpthread -lsocket -lnsl -lm"
	else
	    echo "Undified COMPILER: $(COMPILER))"
        endif
    else
        ifeq ($(COMPILER),cc)
            CCBIN = "/opt/SUNWspro/bin/cc -mt -xCC"
            LDBIN = "/opt/SUNWspro/bin/cc -mt -G"
            CMODE_DBG = "-g -D_REENTRANT -DMDB_DEBUG $(OPTION)"
            CMODE_REL = "-xO5 -UMDB_DEBUG -D_REENTRANT $(OPTION)"
            CMODE_PIC = -KPIC
            SYSLIBS = "-lpthread -lsocket -lnsl -lm"
        else ifeq ($(COMPILER),ccache-cc)
            CCBIN = "ccache /opt/SUNWspro/bin/cc -mt -xCC"
            LDBIN = "/opt/SUNWspro/bin/cc -mt -G"
            CMODE_DBG = "-g -D_REENTRANT -DMDB_DEBUG $(OPTION)"
            CMODE_REL = "-xO5 -UMDB_DEBUG -D_REENTRANT $(OPTION)"
            CMODE_PIC = -KPIC
            SYSLIBS = "-lpthread -lsocket -lnsl -lm"
        else ifeq ($(COMPILER),gcc)
            CCBIN = "gcc -D_GCC"
            LDBIN = "ld -G"
            CMODE_DBG = "-g -Wall -D_REENTRANT -DMDB_DEBUG $(OPTION)"
            CMODE_REL = "-O3 -D_REENTRANT -UMDB_DEBUG $(OPTION)"
            CMODE_PIC = -fPIC
            SYSLIBS = "-lpthread -lsocket -lnsl -lm"
	else ifeq ($(COMPILER),ccache-gcc)
            CCBIN = "ccache gcc -m$B -D_GCC"
            LDBIN = "gcc -m$B -shared"
            CMODE_DBG = "-g -Wall -D_REENTRANT -DMDB_DEBUG $(OPTION)"
            CMODE_REL = "-O3 -D_REENTRANT -UMDB_DEBUG $(OPTION)"
            CMODE_PIC = -fPIC
            SYSLIBS = "-lpthread -lsocket -lnsl -lm"
        esle ifeq ($(COMPILER),gprof)
            CCBIN = "gcc -D_GCC -pg"
            LDBIN = "ld -G"
            CMODE_DBG = "-g -Wall -D_REENTRANT -DMDB_DEBUG $(OPTION)"
            CMODE_REL = "-D_REENTRANT -UMDB_DEBUG $(OPTION)"
            CMODE_PIC = -fPIC
            SYSLIBS = "-lpthread -lsocket -lnsl -lm"
        else ifeq ($(COMPILER),lint)
            CCBIN = "lint"
            LDBIN = 
            CMODE_DBG = "-g -DMDB_DEBUG $(OPTION)"
            CMODE_REL = "-UMDB_DEBUG $(OPTION)"
            CMODE_PIC = 
            SYSLIBS = 
	else
	    echo "Undified COMPILER: $(COMPILER))"
        endif
    endif
endif #SunOS

ifeq ($(OSNAME),Linux)
    ifeq ($(COMPILER),gprof)
        CCBIN = "gcc -m$B -D_GCC -pg"
        LDBIN = "gcc -m$B -shared -pg"
        ARBIN = "ar"
        CCOPT =
    else ifeq ($(COMPILER),ppc860)
        CCBIN = "ppc_8xx-g++ -D_GCC"
        LDBIN = "ppc_8xx-g++ -shared"
        ARBIN = "ar"
        CCOPT = -x c
    else ifeq ($(COMPILER),gcc)
        CCBIN = "gcc -m$B -D_GCC"
        LDBIN = "gcc -m$B -shared -Wl,-Map=mapfile"
        ARBIN = "ar"
        CCOPT = -Wuninitialized -Wunused -fsigned-char
        OPTION += -Wdeclaration-after-statement
    else ifeq ($(COMPILER),clang)
        CCBIN = "clang -m$B -D_GCC"
        LDBIN = "clang -m$B -shared -Wl,-Map=mapfile"
        ARBIN = "ar"
        CCOPT = -Wuninitialized -Wunused -fsigned-char
        OPTION += -Wdeclaration-after-statement
        CMODE_DBG = "-g -Wall -D_REENTRANT -DMDB_DEBUG $(OPTION)"
        CMODE_REL = "-O3 -D_REENTRANT -UMDB_DEBUG $(OPTION)"
#        SYSLIBS = "-lpthread -lsocket -lnsl -lm"
        SYSLIBS = ""
    else ifeq ($(COMPILER),ccache)
        CCBIN = "ccache gcc -m$B -D_GCC"
        LDBIN = "gcc -m$B -shared -Wl,-Map=mapfile"
        ARBIN = "ar"
        CCOPT = -Wuninitialized -Wunused -fsigned-char
    else ifeq ($(COMPILER),ccache-gcc)
        CCBIN = "ccache gcc -m$B -D_GCC"
        LDBIN = "gcc -m$B -shared -Wl,-Map=mapfile"
        ARBIN = "ar"
        CCOPT = -Wuninitialized -Wunused -fsigned-char
    else ifeq ($(COMPILER),arm-gcc-android)
        CCBIN = "$(ANDROID_TOOLCHAIN)/bin/arm-linux-androideabi-gcc -D_GCC -mabi=aapcs-linux -fsigned-char"
        LDBIN = "$(ANDROID_TOOLCHAIN)/bin/arm-linux-androideabi-gcc -shared -Wl,-Map=mapfile"
        ARBIN = "$(ANDROID_TOOLCHAIN)/bin/arm-linux-androideabi-ar"
        OPTION += -DANDROID_OS
    else ifeq ($(COMPILER),i686-protector-linux-gcc)
        CCBIN = "/opt/toolchains/i686-protector-linux-gnu/bin/i686-protector-linux-gnu-gcc -D_GCC -fsigned-char"
        LDBIN = "/opt/toolchains/i686-protector-linux-gnu/bin/i686-protector-linux-gnu-gcc -shared -Wl,-Map=mapfile"
        ARBIN = "/opt/toolchains/i686-protector-linux-gnu/bin/i686-protector-linux-gnu-ar"
    else ifeq ($(COMPILER),purecov)
        CCBIN = "purecov gcc -m$B -D_GCC"
        LDBIN = "gcc -m$B -shared -Wl,-Map=mapfile"
        CCOPT = 
    endif

    CMODE_PIC =-fPIC
    ifeq ($(PTHREAD),shared)
        ifeq ($(COMPILER),arm-gcc-android)
            SYSLIBS = "-lm -ldl"
            CMODE_DBG += "-Werror-implicit-function-declaration -Wextra -DMDB_DEBUG $(OPTION) $(CCOPT)" 
            CMODE_REL += "-Werror-implicit-function-declaration -Wextra $(OPTION) $(CCOPT)" 
        else
            SYSLIBS = "-lpthread -lnsl -lm"
            CMODE_DBG += "-Wall -Werror-implicit-function-declaration -Wextra -DMDB_DEBUG $(OPTION) $(CCOPT) -fvisibility=hidden"
            CMODE_REL += "-Werror-implicit-function-declaration -Wextra $(OPTION) $(CCOPT) -fvisibility=hidden"
            CMODE_REL_G += "-Werror-implicit-function-declaration -Wextra $(OPTION) $(CCOPT) -fvisibility=hidden"
        endif
    else
        SYSLIBS = "-L/usr/lib/nptl -lnsl -lm /usr/lib/nptl/libpthread.a /usr/lib/nptl/libc.a"
        CMODE_DBG += "-Wall -DMDB_DEBUG -I/usr/include/nptl $(OPTION) $(CCOPT)"
        CMODE_REL += "-UMDB_DEBUG -I/usr/include/nptl $(OPTION) $(CCOPT) -fvisibility=hidden"
        CMODE_REL_G += "-UMDB_DEBUG -I/usr/include/nptl $(OPTION) $(CCOPT) -fvisibility=hidden"
    endif
    ifeq ($(COMPILER),gprof)
        CMODE_DBG = "-g -Wall -DMDB_DEBUG $(OPTION) $(CCOPT) -fvisibility=hidden"
        CMODE_REL = "-UMDB_DEBUG $(OPTION) $(CCOPT) -fvisibility=hidden"
    endif
endif

ifeq ($(OSNAME),CYGWIN_NT-5.1)
    OSNAME = Cygwin
endif

ifeq ($(OSNAME),Cygwin)
    ifeq ($(COMPILER),gprof)
        CCBIN = "gcc -m$B -D_GCC -pg"
        LDBIN = "gcc -m$B -shared -pg"
        CCOPT = 
    else
        ifeq ($(COMPILER),ppc860)
            CCBIN = "ppc_8xx-g++ -D_GCC -DCYGWIN -Dlinux"
            LDBIN = "ppc_8xx-g++ -shared"
            CCOPT = -x c
        else
            CCBIN = "gcc -m$B -D_GCC -DCYGWIN -Dlinux"
            LDBIN = "gcc -m$B -shared -Wl,-Map=mapfile"
            #LDBIN = "ld -G"
            CCOPT = 
        endif
    endif
    ifeq ($(COMPILER),ccache)
        CCBIN = "ccache gcc -m$B -D_GCC -DCYGWIN -Dlinux"
        LDBIN = "gcc -m$B -shared -Wl,-Map=mapfile"
        CCOPT = 
    endif
    ifeq ($(COMPILER),ccache-gcc)
        CCBIN = "ccache gcc -m$B -D_GCC -DCYGWIN -Dlinux"
        LDBIN = "gcc -m$B -shared -Wl,-Map=mapfile"
        CCOPT = 
    endif

    ifeq ($(COMPILER),arm-gcc)
        CCBIN = "/usr/local/arm/3.4/bin/arm-linux-gcc -D_GCC -DCYGWIN -Dlinux -fsigned-char"
        LDBIN = "/usr/local/arm/3.4/bin/arm-linux-gcc -shared -Wl,-Map=mapfile"
        ARBIN = "/usr/local/arm/3.4/bin/arm-linux-ar"
    endif

    CMODE_PIC = 
    ifeq ($(PTHREAD),shared)
        ifeq ($(COMPILER),arm-gcc-android)
            SYSLIBS = "-lm"
        else
            SYSLIBS = "-lpthread -lm"
        endif
        CMODE_DBG = "-g -Wall -DMDB_DEBUG $(OPTION) $(CCOPT)"
        CMODE_REL = "-g -O2 -UMDB_DEBUG $(OPTION) $(CCOPT)"
    else
        SYSLIBS = "-L/usr/lib/nptl -lm /usr/lib/nptl/libpthread.a /usr/lib/nptl/libc.a"
        CMODE_DBG = "-g -Wall -DMDB_DEBUG -I/usr/include/nptl $(OPTION) $(CCOPT)"
        CMODE_REL = "-g -O2 -UMDB_DEBUG -I/usr/include/nptl $(OPTION) $(CCOPT)"
    endif
    ifeq ($(COMPILER),gprof)
        CMODE_DBG = "-g -Wall -DMDB_DEBUG $(OPTION) $(CCOPT)"
        CMODE_REL = "-UMDB_DEBUG $(OPTION) $(CCOPT)"
    endif
endif

MAKEOPT_DBG = CC=$(CCBIN) LD=$(LDBIN) AR=$(ARBIN) CMODE=$(CMODE_DBG) \
              CMODE_PIC=$(CMODE_PIC) OSNAME=$(OSNAME) SYSLIBS=$(SYSLIBS) \
              ENABLE_YACC=$(ENABLE_YACC) JNI=$(JNI) \
              COMPILER=$(COMPILER) ARBIN=$(ARBIN) DB_LIB_NAME=$(DB_LIB_NAME) \
              HAVE_READLINE=$(HAVE_READLINE)

MAKEOPT_REL = CC=$(CCBIN) LD=$(LDBIN) AR=$(ARBIN) CMODE=$(CMODE_REL) \
              CMODE_PIC=$(CMODE_PIC) OSNAME=$(OSNAME) SYSLIBS=$(SYSLIBS) \
              ENABLE_YACC=$(ENABLE_YACC) JNI=$(JNI) \
              COMPILER=$(COMPILER) ARBIN=$(ARBIN) DB_LIB_NAME=$(DB_LIB_NAME) \
              HAVE_READLINE=$(HAVE_READLINE)

MAKEOPT_REL_G = CC=$(CCBIN) LD=$(LDBIN) AR=$(ARBIN) CMODE=$(CMODE_REL_G) \
              CMODE_PIC=$(CMODE_PIC) OSNAME=$(OSNAME) SYSLIBS=$(SYSLIBS) \
              ENABLE_YACC=$(ENABLE_YACC) JNI=$(JNI) \
              COMPILER=$(COMPILER) ARBIN=$(ARBIN) DB_LIB_NAME=$(DB_LIB_NAME) \
              HAVE_READLINE=$(HAVE_READLINE)

###########################################
# General setting
###########################################
		
all: mobile_debug
release: mobile_release
release_g: mobile_release_g

mobile: mobile_debug

check_dir:
	-@if [ ! -d ./bin ]; then mkdir ./bin; fi
	-@if [ ! -d ./lib ]; then mkdir ./lib; fi
	-@if [ -d ./include ]; then rm -rf ./include; fi
	-@if [ ! -d ./include ]; then mkdir ./include; fi
	-@cp ./src/include/*.h ./include
	-@(test -f ./bin/recreatedb || cp ./src/shscript/recreatedb  ./bin/recreatedb)
	-@rm -f ./src/version_mmdb.h
	-@(cd ./src; ln -s ../version_mmdb.h .)

sqlyacc_d:
	@if [ -d ./src/server/sql ]; then (cd ./src/server/sql; make yacc_debug _LIB_SUFFIX=a $(MAKEOPT_DBG)) ; fi
	@if [ -d ./src/server/sql ]; then (cd ./src/server/sql; make yacc_debug _LIB_SUFFIX=so $(MAKEOPT_DBG)) ; fi

sqlyacc_r:
	@if [ -d ./src/server/sql ]; then (cd ./src/server/sql; make yacc_release _LIB_SUFFIX=a $(MAKEOPT_REL)) ; fi
	@if [ -d ./src/server/sql ]; then (cd ./src/server/sql; make yacc_release _LIB_SUFFIX=so $(MAKEOPT_REL)) ; fi

sqlyacc_rg:
	@if [ -d ./src/server/sql ]; then (cd ./src/server/sql; make yacc_release _LIB_SUFFIX=a $(MAKEOPT_REL_G)) ; fi
	@if [ -d ./src/server/sql ]; then (cd ./src/server/sql; make yacc_release _LIB_SUFFIX=so $(MAKEOPT_REL_G)) ; fi

mode32:
	@rm -f config.tmp
	@sed "/COMPILE_MODE/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILE_MODE = mode32" 1>> config.mk

mode64:
	@rm -f config.tmp
	@sed "/COMPILE_MODE/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILE_MODE = mode64" 1>> config.mk

cc:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = cc" 1>> config.mk

gcc:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = gcc" 1>> config.mk

clang:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = clang" 1>> config.mk

gprof:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = gprof" 1>> config.mk

lint:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = lint" 1>> config.mk

arm-gcc:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = arm-gcc" 1>> config.mk

ccache: ccache-gcc

ccache-cc:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = ccache-cc" 1>> config.mk

ccache-gcc:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = ccache-gcc" 1>> config.mk

arm-gcc-android:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = arm-gcc-android" 1>> config.mk

i686-protector-linux-gcc:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = i686-protector-linux-gcc" 1>> config.mk
								
spthread:
	@rm -f config.tmp
	@sed "/PTHREAD /d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "PTHREAD = shared" 1>> config.mk
apthread:
	@rm -f config.tmp
	@sed "/PTHREAD /d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "PTHREAD = static" 1>> config.mk

purecov:
	@rm -f config.tmp
	@sed "/COMPILE/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILE_MODE = mode32" 1>> config.mk
	@echo "COMPILER = purecov" 1>> config.mk

DO_NOT_FSYNC:
	@rm -f config.tmp
	@sed "/DO_NOT_FSYNC/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "OPTION += -DDO_NOT_FSYNC" 1>> config.mk

INDEX_BUFFERING:
	@rm -f config.tmp
	@sed "/INDEX_BUFFERING/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "OPTION += -DINDEX_BUFFERING" 1>> config.mk

BRAND=openML

ifeq ($(JNI),1)
    JNI_FEATURE+=-jni
endif

distclean: clean
	@if [ -d ./OS/elinux ]; then (cd ./OS/elinux; make distclean) ; fi
	-@echo -n "ccache... "
	-@ccache -C

clean:
	@if [ -d ./src/server/sql ]; then (cd ./src/server/sql; make distclean ENABLE_YACC=$(ENABLE_YACC)) ; fi
	@if [ -d ./src/utils ]; then (cd ./src/utils; make clean) ; fi
	@if [ -d ./src/jni ]; then (cd ./src/jni; make clean) ; fi
	@if [ -d ./OS/elinux ]; then (cd ./OS/elinux; make clean) ; fi
	-rm -rf ./bin/* ./lib/* ./include/*

sqlclean:
	@if [ -d ./src/server/sql ]; then (cd ./src/server/sql; make distclean) ; fi

changelog:
	cvs2cl.pl -r -t -T -b

changelogF:
	cvs2cl.pl -r -t -T -b -F $(BRAND)
	-@rm -f ChangeLog.sql.bak ChangeLog.bak
	
ctags:
	-ctags -h ".c.h.l.y" -R src version_mmdb.h

machdep:
	echo $(OSNAME)_$(COMPILE_MODE)_$(COMPILER).mk

mobile_debug: check_dir sqlyacc_d libmobiledbms_d mobile_utils_d mobile_jni_d notice_debug

mobile_release: check_dir sqlyacc_r libmobiledbms_r mobile_utils_r mobile_jni_r notice_release
	-@rm -f ./bin/logdump

mobile_release_g: check_dir sqlyacc_rg libmobiledbms_rg mobile_utils_rg mobile_jni_rg notice_release_g
	-@rm -f ./bin/logdump

libmobiledbms_d:
ifeq ($(OSNAME),Linux)
	@if [ -d ./OS/elinux ]; then (cd ./OS/elinux; make all $(MAKEOPT_DBG)) ; fi
else
    ifeq ($(OSNAME),SunOS)
	@if [ -d ./OS/elinux ]; then (cd ./OS/elinux; make all $(MAKEOPT_DBG)) ; fi
  else
    ifeq ($(OSNAME),Cygwin)
	  @if [ -d ./OS/elinux ]; then (cd ./OS/elinux; make all $(MAKEOPT_DBG)) ; fi
    else
	  echo "Not supported mobiledb on $(OSNAME)"
    endif
  endif
endif

libmobiledbms_r:
ifeq ($(OSNAME),Linux)
	@if [ -d ./OS/elinux ]; then (cd ./OS/elinux; make all $(MAKEOPT_REL)) ; fi
else
  ifeq ($(OSNAME),SunOS)
	@if [ -d ./OS/elinux ]; then (cd ./OS/elinux; make all $(MAKEOPT_REL)) ; fi
  else
    ifeq ($(OSNAME),Cygwin)
	  @if [ -d ./OS/elinux ]; then (cd ./OS/elinux; make all $(MAKEOPT_REL)) ; fi
    else
	  	echo "Not supported mobiledb on $(OSNAME)"
    endif
  endif
endif

libmobiledbms_rg:
ifeq ($(OSNAME),Linux)
	@if [ -d ./OS/elinux ]; then (cd ./OS/elinux; make all $(MAKEOPT_REL_G)) ; fi
else
ifeq ($(OSNAME),SunOS)
	@if [ -d ./OS/elinux ]; then (cd ./OS/elinux; make all $(MAKEOPT_REL_G)) ; fi
else
	echo "Not supported mobiledb on $(OSNAME)"
endif
endif

mobile_utils_d:
	@if [ -d ./src/utils ]; then (cd ./src/utils; make -f Makefile all $(MAKEOPT_DBG)) ; fi

mobile_utils_r:
	@if [ -d ./src/utils ]; then (cd ./src/utils; make -f Makefile all $(MAKEOPT_REL)) ; fi

mobile_utils_rg:
	@if [ -d ./src/utils ]; then (cd ./src/utils; make -f Makefile all $(MAKEOPT_REL_G)) ; fi

mobile_jni_d:
ifeq ($(JNI),1)
	@if [ -d ./src/jni ]; then (cd ./src/jni; make all $(MAKEOPT_DBG)) ; fi
endif

mobile_jni_r:
ifeq ($(JNI),1)
	@if [ -d ./src/jni ]; then (cd ./src/jni; make all $(MAKEOPT_REL)) ; fi
endif

mobile_jni_rg:
ifeq ($(JNI),1)
	@if [ -d ./src/jni ]; then (cd ./src/jni; make all $(MAKEOPT_REL_G)) ; fi
endif

tee:
	make -f Makefile 2>&1 | sed -u -e '/warning: null argument where non-null required/d' | tee make.log

release_tee:
	make -f Makefile release 2>&1 | sed -u -e '/warning: null argument where non-null required/d' | tee make.log

notice_debug: notice
	@echo "  >> End of Debug Mode Compilation"

notice_release: notice
	@echo "  >> End of Release Mode Compilation"

notice_release_g: notice
	@echo "  >> End of Release-G Mode Compilation"

notice:
	@echo "    OSNAME:            $(OSNAME)"
	@echo "    COMPILER:          $(COMPILER)"
	@echo "    COMPILE_MODE:      $(COMPILE_MODE)"
	@echo "    OPTION:            $(OPTION)"
	@echo "    ENABLE_YACC:       $(ENABLE_YACC)"
	@echo "    JNI:               $(JNI)"
