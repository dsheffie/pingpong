OBJ = pingpong.o
CC = gcc

DEP = $(OBJ:.o=.d)
OPT = -O3
EXE = pingpong

.PHONY : all clean

all: $(EXE)

$(EXE) : $(OBJ)
	$(CC) $(OBJ) $(LIBS) -o $(EXE)

%.o: %.c
	$(CC) -MMD $(OPT) -c $< 


-include $(DEP)

clean:
	rm -rf $(EXE) $(OBJ) $(DEP)
