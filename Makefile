.PHONY: run clean

main: $(objs)
	g++ $(objs) -o $@

run: main 
	@./main

objs = card.o player.o main.o

$(objs):%.o:%.cpp 
	g++ -c -std=c++17 $< -o $@

clean: 
	-rm $(objs) main