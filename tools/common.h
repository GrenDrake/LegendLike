#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>

void edit_text(int y, int x, char *text);
void edit_text_key(char *text, int key);
void force_bool(int *value);
int is_name_char(int c);
uint32_t read32(FILE *fp);
uint16_t read16(FILE *fp);
uint8_t  read8(FILE  *fp);
int sign(int x);
void write32(FILE *fp, uint32_t value);
void write16(FILE *fp, uint16_t value);
void write8(FILE  *fp, uint8_t  value);

#endif
