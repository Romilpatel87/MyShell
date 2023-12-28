CFLAGS = -g -Wall


shell: mysh.c
    gcc $(CFLAGS) -o mysh mysh.c $(LFLAGS)

clean:
    rm shell