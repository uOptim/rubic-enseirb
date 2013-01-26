SRC=src
TEST=tests
BIN=bin

.PHONY:all tests runtests clean mrproper

all:
	make -C ${SRC}
	mv ${SRC}/rubic ${BIN}/rubic

tests: all
	make -C ${SRC} tests
	mv ${SRC}/tests ${BIN}/tests
	@for FILE in `ls tests | grep "^test_.*\.rb$$"`; do                 \
	    echo tests/$$FILE                                            && \
	    ${BIN}/rubic < tests/$$FILE > tests/$$FILE.ll                && \
	    llc -filetype=obj tests/$$FILE.ll                            && \
	    rm -f tests/*.ll                                             && \
	    gcc tests/$$FILE.o ${SRC}/builtins.o -o ${BIN}/tests/$$FILE;    \
	    if [ $$? -ne 0 ]; then break; fi;                               \
	done

runtests: tests
	@for FILE in `ls ${BIN}/tests/`; do \
		${BIN}/tests/$$FILE;            \
	done

clean:
	make -C ${SRC} clean
	make -C ${TEST} clean

mrproper:
	@make clean
	@rm -f ${BIN}/* 
