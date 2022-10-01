#include <stdlib.h>
#include <stdio.h>

void gnuplot(char *name, char *desc, char *color);


void
gnuplot(char *name, char *desc, char *color)
{
	char buffer[2048];
	snprintf(buffer, 2048,
	         "cat data/logs/%s.txt| gnuplot -p -e "
	         "\"set terminal png;"
	         "set boxwidth 0.7;"
	         "set style fill solid;"
	         "set title \'%s\';"
	         "unset key;"
	         "set border 1+2 back;"
	         "plot '-' using 1:3:xtic(2) with boxes lt rgb \'%s\'\" "
	         "2>/dev/null > data/charts/%s.png", name, desc, color, name);
	system(buffer);
}
