.PHONY: run clean

objs = card.o player.o main.o

main: $(objs)
	g++ $(objs) -o $@

run: main 
	@./main

$(objs):%.o:%.cpp 
	g++ -c -std=c++17 $< -o $@

clean: 
	-rm $(objs) main