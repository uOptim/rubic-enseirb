SRCDIR=src
TESTSDIR=tests
BINDIR=bin

PRG=rubic2llvm

.PHONY:all tests runtests clean mrproper

all:
	make -C ${SRCDIR}
	cp ${SRCDIR}/${PRG} ./${PRG}
	cp ${SRCDIR}/${PRG} ${BINDIR}/${PRG}
	cp ${SRCDIR}/builtins.o bin/builtins.o

tests: all
	#make -C ${SRCDIR} tests
	#mv ${SRCDIR}/tests ${BINDIR}/tests
	@for FILE in `ls tests | grep "^test_.*\.rb$$"`; do                 \
	    echo tests/$$FILE                                            && \
	    ${BINDIR}/${PRG} < tests/$$FILE > tests/$$FILE.ll                && \
	    llc -filetype=obj tests/$$FILE.ll                            && \
	    rm -f tests/*.ll                                             && \
	    gcc tests/$$FILE.o ${SRCDIR}/builtins.o -o ${BINDIR}/tests/$$FILE;    \
	    if [ $$? -ne 0 ]; then break; fi;                               \
	done

runtests: 
	@for FILE in `ls ${BINDIR}/tests/`; do                     \
	    echo ${BINDIR}/tests/$$FILE && ${BINDIR}/tests/$$FILE; \
	done

clean:
	make -C ${SRCDIR} clean
	make -C ${TESTSDIR} clean

mrproper:
	@make clean
	@rm -f ${BINDIR}/${PRG} ${BINDIR}/tests/*
