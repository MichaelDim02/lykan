#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define VERSION "v0.1"
#define BUFFER_SIZE 512
#define BLUE "\e[0;34m"
#define RED "\x1b[31m"
#define RESET "\x1b[0m"


volatile sig_atomic_t running;


static inline void
help(char *bin)
{
	printf("Lycan - Leak Analyzer %s\n\n"
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
update_progress_bar(double current_perc, int width)
{
	printf("\rProgress [");
	int comp = current_perc * width / 100;
	int incomp = width - comp;

	printf(RED);
	for (int i = 0; i < comp-1; i++) {
		printf("=");
	}

	if (incomp == 0)
		printf("=");
	else
		printf(">");

	if (incomp != 0) {
		for (int i = 0; i < incomp; i++) {
			printf(" ");
		}
	}
	printf(RESET);

	fflush(stdout);

	printf("] %.2f%c", current_perc, '%');
}

static void
statprint(char *name, double stat, int num)
{
	printf("%3s: %7.3f% " BLUE " (%d) " RESET "\n", name, stat, num);
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
	for (int i = 0; i < strlen(buffer); i++) {
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
main(int argc, char **argv)
{
	if (argc < 2)
		help(argv[0]);

	FILE *f = fopen(argv[1], "r");

	if (!f)
		die("File could not be opened", EXIT_FAILURE);

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

	struct stat st = {0};
	if (stat("data/", &st) == -1) {
		mkdir("data/", 0700);

		if (stat("data/logs", &st) == -1) {
			mkdir("data/logs/", 0700);
		}

		if (stat("data/charts", &st) == -1) {
			mkdir("data/charts/", 0700);
		}
	}


	printf("Length:\n------------------------\n");
	FILE *f2 = fopen("data/logs/length.txt", "w");

	for (int i=0; i<30; i++) {
		char snum[13];
		snprintf(snum, 13, "%d", i);
		double perc = (double)length[i] * 100.0 / (double)count;
		//double perc = (double)length[i+1] * 100.0 / (double)count;

		statprint(snum, perc, length[i]);
		//statprint(snum, perc, length[i+1]);

		if (i <= 30)
			fprintf(f2, "%d %d %.2f%\n", i, i, perc);

	}
	fclose(f2);
	statprint("30+", (double)length[30]*100.0/(double)length[30]*100.0/(double)count, length[30]);
	system("cat data/logs/length.txt| gnuplot -p -e "
	       "\"set terminal png;"
	       "set boxwidth 0.7;"
	       "set style fill solid;"
	       "set title \'Length\';"
	       "unset key;"
	       "set border 1+2 back;"
	       "plot '-' using 1:3:xtic(2) with boxes lt rgb \'red\'\""
	       " > data/charts/length.png");
	printf("\n\n");



	printf("Basic character sets:\n----------\n");
	FILE *f3 = fopen("data/logs/chars.txt", "w");
	int chars_total = chars[0] + chars[1] + chars[2] + chars[3] + chars[4];
	for (int i=0; i<5; i++) {
		double perc = (double)chars[i] * 100.0 / (double)chars_total;

		printf("%-10s: %6.3f% "BLUE" (%d)"RESET"\n", chars_name[i], perc, chars[i]);

		//if (i <= 30)
			fprintf(f3, "%d \"%s\" %f\n", i, chars_name[i], perc);
	}
	fclose(f3);
	system("cat data/logs/chars.txt| gnuplot -p -e "
	       "\"set terminal png;"
	       "set boxwidth 0.7;"
	       "set style fill solid;"
	       "set title \'Basic character sets\';"
	       "unset key;"
	       "set border 1+2 back;"
	       "plot '-' using 1:3:xtic(2) with boxes lt rgb \'blue\'\""
	       "> data/charts/chars.png");
	printf("\n\n");


	printf("Symbols:\n-----------------------\n");

	symbol_bsort(0, 52, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(52, 62, symbols, symbols_name);
	symbol_bsort(62, 89, symbols, symbols_name);
	symbol_bsort(62, 89, symbols, symbols_name);
	symbol_bsort(62, 89, symbols, symbols_name);
	symbol_bsort(62, 89, symbols, symbols_name);
	symbol_bsort(62, 89, symbols, symbols_name);
	symbol_bsort(62, 89, symbols, symbols_name);
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

	printf("Letters:\n---------\n");
	FILE *f4 = fopen("data/logs/symb_letters.txt", "w");
	for (int i=0; i<20; i++) {
		double perc = (double)symbols[i] * 100.0 / (double)chars_total;

		printf("%c: %6.3f% "BLUE" (%d)"RESET"\n", symbols_name[i], perc, symbols[i]);

		fprintf(f4, "%d '%c' %f\n", i, symbols_name[i], perc);
	}
	fclose(f4);
	system("cat data/logs/symb_letters.txt| gnuplot -p -e "
	       "\"set terminal png;"
	       "set boxwidth 0.7;"
	       "set style fill solid;"
	       "set title \'Most popular characters (letters)\';"
	       "unset key;"
	       "set border 1+2 back;"
	       "plot '-' using 1:3:xtic(2) with boxes lt rgb \'blue\'\""
	       "> data/charts/symb_letters.png");
	printf("\n\n");

	printf("Numbers:\n---------\n");
	FILE *f5 = fopen("data/logs/symb_numbers.txt", "w");
	for (int i=52; i<62; i++) {
		double perc = (double)symbols[i] * 100.0 / (double)chars_total;

		printf("%c: %6.3f% "BLUE" (%d)"RESET"\n", symbols_name[i], perc, symbols[i]);

		fprintf(f4, "%d '%c' %f\n", i-52, symbols_name[i], perc);
	}
	fclose(f5);
	system("cat data/logs/symb_numbers.txt| gnuplot -p -e "
	       "\"set terminal png;"
	       "set boxwidth 0.7;"
	       "set style fill solid;"
	       "set title \'Most popular characters (numbers)\';"
	       "unset key;"
	       "set border 1+2 back;"
	       "plot '-' using 1:3:xtic(2) with boxes lt rgb \'blue\'\""
	       "> data/charts/symb_numbers.png");
	printf("\n\n");

	printf("Special:\n---------\n");
	FILE *f6 = fopen("data/logs/symb_special.txt", "w");
	for (int i=62; i<89; i++) {
		double perc = (double)symbols[i] * 100.0 / (double)chars_total;

		printf("%c: %6.3f% "BLUE" (%d)"RESET"\n", symbols_name[i], perc, symbols[i]);

		fprintf(f4, "%d '%c' %f\n", i-62, symbols_name[i], perc);
	}
	fclose(f6);
	system("cat data/logs/symb_special.txt| gnuplot -p -e "
	       "\"set terminal png;"
	       "set boxwidth 0.7;"
	       "set style fill solid;"
	       "set title \'Most popular characters (numbers)\';"
	       "unset key;"
	       "set border 1+2 back;"
	       "plot '-' using 1:3:xtic(2) with boxes lt rgb \'blue\'\""
	       "> data/charts/symb_special.png");
	printf("\n\n");
	exit(EXIT_SUCCESS);
}
