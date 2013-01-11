FNAME=tcc_attachments
gcc -o bin2c bin2c.c
./bin2c -m -d ${FNAME}.h -o ${FNAME}.c include/* libtcc.a libtcc1.a
gcc -c ${FNAME}.c
