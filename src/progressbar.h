#ifndef PROGBAR_H
#define PROGBAR_H

#define BLUE "\e[0;34m"
#define RED "\x1b[31m"
#define RESET "\x1b[0m"

void update_progress_bar(double current_perc, int width);

#endif
