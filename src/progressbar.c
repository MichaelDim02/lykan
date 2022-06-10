#include <stdio.h>

#define BLUE "\e[0;34m"
#define RED "\x1b[31m"
#define RESET "\x1b[0m"

void update_progress_bar(double current_perc, int width);

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
