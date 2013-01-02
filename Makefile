SRC=src
BIN=bin

all:
	make -C ${SRC}
	mv ${SRC}/rubic ${BIN}/rubic

tests: 
	make -C ${SRC} tests
	mv ${SRC}/tests ${BIN}/tests

clean:
	make -C ${SRC} clean

mrproper:
	@make clean
	@rm -f ${BIN}/* 
