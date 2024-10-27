objects = fpt.o 
main : $(objects)
	g++ -o main $(objects) -lgmp

fpt.o : fpt.cpp  
	g++ -c fpt.cpp 

clean :
	rm *.o