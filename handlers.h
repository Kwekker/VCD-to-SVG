#ifndef HANDLERS_H_
#define HANDLERS_H_

#include <stdio.h>
#include "vcd.h"


int handleComment(FILE *file, vcd_t* vcd);
int handleDate(FILE *file, vcd_t* vcd);
int handleEnddefinitions(FILE *file, vcd_t* vcd);
int handleScope(FILE *file, vcd_t* vcd);
int handleTimescale(FILE *file, vcd_t* vcd);
int handleUpscope(FILE *file, vcd_t* vcd);
int handleVar(FILE *file, vcd_t* vcd);
int handleVersion(FILE *file, vcd_t* vcd);
int handleDumpall(FILE *file, vcd_t* vcd);
int handleDumpoff(FILE *file, vcd_t* vcd);
int handleDumpon(FILE *file, vcd_t* vcd);
int handleDumpvars(FILE *file, vcd_t* vcd);


#endif