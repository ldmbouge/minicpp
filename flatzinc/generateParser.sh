flex -L -o lexer.yy.cpp lexer.lxx
bison -l -t -o parser.tab.cpp -d parser.yxx