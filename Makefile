SRCDIR=src
TESTSDIR=tests
BINDIR=bin

.PHONY:all tests runtests clean mrproper

all:
	make -C ${SRCDIR}
	cp ${SRCDIR}/rubic2llvm ./rubic2llvm
	cp ${SRCDIR}/builtins.o bin/builtins.o

tests: all
	make -C ${SRCDIR} tests
	mv ${SRCDIR}/tests ${BINDIR}/tests
	@for FILE in `ls tests | grep "^test_.*\.rb$$"`; do                 \
	    echo tests/$$FILE                                            && \
	    ${BINDIR}/rubic < tests/$$FILE > tests/$$FILE.ll                && \
	    llc -filetype=obj tests/$$FILE.ll                            && \
	    rm -f tests/*.ll                                             && \
	    gcc tests/$$FILE.o ${SRCDIR}/builtins.o -o ${BINDIR}/tests/$$FILE;    \
	    if [ $$? -ne 0 ]; then break; fi;                               \
	done

runtests: tests
	@for FILE in `ls ${BINDIR}/tests/`; do \
		${BINDIR}/tests/$$FILE;            \
	done

clean:
	make -C ${SRCDIR} clean
	make -C ${TESTSDIR} clean

mrproper:
	@make clean
	@rm -f ${BINDIR}/* 
