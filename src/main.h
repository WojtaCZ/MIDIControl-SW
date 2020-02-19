#ifndef MAIN_H
#define MAIN_H

char version[] = "1.0.0";
char cmd[255], name[255];
char *consoleLine;
char * line;

char **cmd_completition(const char *, int, int);
char *cmd_completition_gen(const char *, int);

#endif