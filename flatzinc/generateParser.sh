flex -L -o lexer.yy.cu lexer.lxx
bison -l -t -o parser.tab.cu -d parser.yxx
mv parser.tab.hu parser.tab.h 
