.PHONY: run clean

run: main 
	@./main

objs = card.o player.o main.o

main: $(objs)
	g++ $(objs) -o $@

$(objs):%.o:%.cpp 
	g++ -c -std=c++17 $< -o $@

clean: 
	-rm $(objs) main