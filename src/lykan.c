#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gnuplot.h"
#include "progressbar.h"

#define VERSION "v0.2"

#define BUFFER_SIZE 512
#define BLUE "\e[0;34m"
#define RED "\x1b[31m"
#define RESET "\x1b[0m"

#define COL(X) "\033[100D\033["#X"C"
#define UP(X) "\033["#X"A"
#define DOWN(X) "\033["#X"B"
#define RIGHT(X) "\033["#X"C"
#define LEFT(X) "\033["#X"D"

volatile sig_atomic_t running;


static inline void
help(char *bin)
{
	printf("lykan - Leak Analyzer %s\n"
	       "by Michael Constantine Dimopoulos\n"
	       "https://mcdim.xyz  <mk@mcdim.xyz>\n\n"

	       "Usage: %s [FILENAME]\n", VERSION, bin);
	exit(EXIT_FAILURE);
}

static void
die(char *str, int exit_code)
{
	if (exit_code == 0)
		printf("%s\n", str);
	else
		fprintf(stderr, str);
	exit(exit_code);
}

void handler(int signum){
	printf("Ctrl+c pressed\n");
	running = 0;
}

void
make_dirs()
{
	struct stat st = {0};
	if (stat("data", &st) == -1) {
		mkdir("data/", 0700);

		if (stat("data/logs", &st) == -1) {
			mkdir("data/logs/", 0700);
		}

		if (stat("data/charts", &st) == -1) {
			mkdir("data/charts/", 0700);
		}
	}
}

static void
statprint(char *name, double stat, int num)
{
	printf("%3s: %7.3f% " BLUE " (%d) " RESET "\n", name, stat, num);
}


static inline double 
get_perc(int count, int total)
{
	return (double)count * 100.0 / (double)total;
}


static int
get_lines(FILE *f)
{
	int ch;
	int count=0;

	do {
	        ch = fgetc(f);
		if(ch == '\n') count++;   

	} while( ch != EOF );    

	return count;
}

//static void
//top_passwords(char *buffer, char *passwords[], int passwords_count[], int length)
//{
//	int flag = 0;
//
//	for (int i=0; i<length; i++) {
//		if (strcmp(passwords[i],buffer) == 0) {
//			passwords_count[i]++;
//			flag = 1;
//		}
//	}
//		
//	if (flag == 0) {
//		// expand arrays //
//		strcpy(pass, passwords[length]);
//		passwords_count[length]++;
//		length++;
//	}
//}

static void
length_stat(char *buffer, int length[])
{

	int len = strlen(buffer);

	if (len < 30)
		length[len]++;
		//length[len-1]++;
	else 
		length[30]++;
}

static int
issymbol(char c)
{
	if (c == '!' || c == '@' || c == '#'
	 || c == '$' || c == '%' || c == '^'
	 || c == '&' || c == '*' || c == '('
	 || c == ')' || c == '-' || c == '{'
	 || c == '}' || c == '[' || c == ']'
	 || c == ':' || c == ';' || c == '"'
	 || c == '\''|| c == '<' || c == '>'
	 || c == '.' || c == '/' || c == '?'
	 || c == '~' || c == '`' || c == ' ')
		return 1;
	else
		return 0;
}

static void
charset_stat(char *buffer, int chars[])
{
	int lower=0, upper=0, num=0, symb=0, other=0;
	for (int i = 0; i < strlen(buffer); i++) {
		if (islower(buffer[i]) != 0)
			chars[0]++;
		else if (isupper(buffer[i]) != 0)
			chars[1]++;
		else if (isdigit(buffer[i]) != 0)
			chars[2]++;
		else if (issymbol(buffer[i]) != 0)
			chars[3]++;
		else
			chars[4]++;
	}
}

static void
symbols_stat(char *buffer, int symbols[], char symbols_name[])
{
	int len = strlen(buffer)
	for (int i = 0; i < len; i++) {
		for (int j = 0; j < 89; j++) {
			if (buffer[i] == symbols_name[j])
				symbols[j]++;
		}
	}
}

static void
symbol_bsort(int min, int max, int symbols[], char symbols_name[])
{
	for (int i = min; i < max - 1; i++) {
		//for (int j = min; j < max - j - 1; j++) {
		for (int j = min; j < i; j++) {
			if (symbols[j] <= symbols[j+1]) {
				int swap = symbols[j];
				symbols[j] = symbols[j+1];
				symbols[j+1] = swap;

				char swapc = symbols_name[j];
				symbols_name[j] = symbols_name[j+1];
				symbols_name[j+1] = swapc;
			}
		}
	}
}

void
on_length(int *length, int count)
{
	printf("Length:\n------------------------\n");
	FILE *f2 = fopen("data/logs/length.txt", "w");

	for (int i=0; i<30; i++) {
		char snum[13];
		snprintf(snum, 13, "%d", i);
		double perc = get_perc(length[i], count);

		statprint(snum, perc, length[i]);

		if (i <= 30)
			fprintf(f2, "%d %d %.2f%\n", i, i, perc);

	}
	fclose(f2);
	statprint("30+", (double)length[30]*100.0/(double)length[30]*100.0/(double)count, length[30]);

	gnuplot("length", "Length", "red");

	printf("\n\n");
}

void
on_basic_charsets(int *chars, char **chars_name, int chars_total)
{
	printf(COL(27) UP(35) "Basic character sets:" COL(27) DOWN(1) "---------" LEFT(9) DOWN(1));
	FILE *f3 = fopen("data/logs/chars.txt", "w");
	for (int i=0; i<5; i++) {
		double perc = get_perc(chars[i], chars_total);

		printf("%-10s: %6.3f% "BLUE" (%d)"RESET COL(27) DOWN(1), chars_name[i], perc, chars[i]);

		//if (i <= 30)
			fprintf(f3, "%d \"%s\" %f\n", i, chars_name[i], perc);
	}
	fclose(f3);
	printf(DOWN(25));

	gnuplot("chars", "Basic character sets", "blue");

	printf("\n\n");
}

void
on_letters(int *symbols, char *symbols_name, int chars_total)
{
	printf("Letters:\n---------\n");
	FILE *f4 = fopen("data/logs/symb_letters.txt", "w");
	for (int i=0; i<20; i++) {
		double perc = get_perc(symbols[i], chars_total);

		printf("%c: %6.3f% "BLUE" (%d)"RESET"\n", symbols_name[i], perc, symbols[i]);

		fprintf(f4, "%d '%c' %f\n", i, symbols_name[i], perc);
	}
	fclose(f4);

	gnuplot("symb_letters", "Most popular characters (letters)", "blue");

	printf("\n\n");
}

void
on_numbers(int *symbols, char *symbols_name, int chars_total)
{
	printf(COL(27) UP(24) "Numbers:" LEFT(8) DOWN(1) "---------" LEFT(9) DOWN(1));
	FILE *f5 = fopen("data/logs/symb_numbers.txt", "w");
	for (int i=52; i<62; i++) {
		double perc = get_perc(symbols[i], chars_total);

		printf("%c: %6.3f% "BLUE" (%d)"RESET DOWN(1) COL(27), symbols_name[i], perc, symbols[i]);

		fprintf(f5, "%d '%c' %f\n", i-52, symbols_name[i], perc);
	}
	fclose(f5);

	gnuplot("symb_numbers", "Most popular characters (numbers)", "blue");

	printf("\n\n");
}

void
on_special(int *symbols, char *symbols_name, int chars_total)
{
	printf(COL(54) UP(14) "Special:" LEFT(8) DOWN(1) "---------" LEFT(9) DOWN(1));
	FILE *f6 = fopen("data/logs/symb_special.txt", "w");
	for (int i=62; i<89; i++) {
		double perc = get_perc(symbols[i], chars_total);

		printf("%c: %6.3f% "BLUE" (%d)"RESET DOWN(1) COL(54), symbols_name[i], perc, symbols[i]);

		fprintf(f6, "%d '%c' %f\n", i-62, symbols_name[i], perc);
	}
	fclose(f6);

	gnuplot("symb_special", "Most popular characters (symbols)", "blue");

	printf("\n\n");
}

void
main(int argc, char **argv)
{
	if (argc < 2)
		help(argv[0]);

	FILE *f = fopen(argv[1], "r");

	if (!f)
		die("File could not be opened\n", EXIT_FAILURE);

	char buffer[BUFFER_SIZE];


	int count = 0;
	int total = get_lines(f);
	int length[31] = {0};
	/* al, au, n, s, al_au, al_n, al_s, au_n, au_s, n_s; */
	//int charset[10] = {1};
	/* al, au, n, s */
	int symbols[89] = {0};
	char symbols_name[89] = {
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
		'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
		'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
		'W', 'X', 'Y', 'Z',
		'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
		'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-',
		'{', '}', '[', ']', ':', ';', '"', '\'','<', '>', '.',
		'/', '?', '~', '`', ' '
       	};
	int chars[5] = {0};
	char *chars_name[5] = {
		"Lower-case",
		"Upper-case",
		"Numbers",
		"Symbols",
		"Other",
	};


	FILE *f1 = fopen(argv[1], "r");

	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	printf("\e[?25l");

	running = 1;
	signal(SIGINT, handler);
	while ((fgets(buffer, sizeof(buffer), f1) != NULL) && running == 1) {

		if (count % 10000 == 0 || count == total-1) {
			double current_perc = (double)count / (double)total * 100.0;
			update_progress_bar(current_perc, w.ws_col/2);
		}

		strtok(buffer, "\n");

		length_stat(buffer, length);
		charset_stat(buffer, chars);
		symbols_stat(buffer, symbols, symbols_name);

		count++;
	}

	fclose(f1);

	printf("\n\n");
	printf("\e[?25h");


	make_dirs();

	int chars_total = chars[0] + chars[1] + chars[2] + chars[3] + chars[4];


	//******** LENGTH ********//
	on_length(length, count);


	//******** BASIC CHARSET ********//
	on_basic_charsets(chars, chars_name, chars_total);

	symbol_bsort(0, 52, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(62, 89, symbols, symbols_name);


	//******** SYMBOLS ********//
	printf("Symbols:\n-----------------------\n");


	//******** LETTERS ********//
	on_letters(symbols, symbols_name, chars_total);


	//******** NUMBERS ********//
	on_numbers(symbols, symbols_name, chars_total);


	//******** SPECIAL ********//
	on_special(symbols, symbols_name, chars_total);



	exit(EXIT_SUCCESS);
}







/*
	symbol_bsort(0, 52, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(62, 89, symbols, symbols_name);
	/*for (int i = 52; i < 62 - 1; i++) {
		//for (int j = min; j < max - j - 1; j++) {
		for (int j = 52; j < 62; j++) {
			if (symbols[j] < symbols[j+1]) {
				int swap = symbols[j];
				symbols[j] = symbols[j+1];
				symbols[j+1] = swap;

				char swapc = symbols_name[j];
				symbols_name[j] = symbols_name[j+1];
				symbols_name[j+1] = swapc;
			}
		}
	}*/
