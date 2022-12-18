main:main.cpp lex.yy.c
	$(CXX) -o $@ $^

clean:
	rm -f main

.PHONY: clean main