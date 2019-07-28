/*
 * Copyright (c) 2019 Oliver Mahmoudi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define LOWER 32
#define UPPER 126
#define LOOP (UPPER-LOWER+1)
#define ASCII_A	65
#define ASCII_Z	90
#define MAXINPUT 1024
#define OPTSTRING "dehi:o:pst"

char *file_to_read = NULL , *file_to_write = NULL;
FILE *fp_read = NULL, *fp_write = NULL;
int iflag, oflag, pflag;

int encrypt_char(char, char);
int decrypt_char(char, char);
void process_message(char *, char *, char *);
int get_ciphermsg(char *);
void print_tabula_recta();
void usage();

int encrypt_char(char _char_to_encrypt, char _secret_char)
{
	char cte = _char_to_encrypt;
	char sct = _secret_char;
	int encch;

	if(cte == ' ' && pflag == 1)
		encch = cte;
	else {
		encch = cte + sct - LOWER;
		if(encch > UPPER)
			encch = encch-LOOP;
	}

	return encch;
}

int decrypt_char(char _char_to_decrypt, char _secret_char)
{
	char ctd = _char_to_decrypt;
	char sct = _secret_char;
	int decch;

	if(ctd == ' ' && pflag == 1)
		decch = ctd;
	else {
		decch = ctd - sct + LOWER;
		if(decch < LOWER)
			decch = decch + LOOP;
	}

	return decch;
}

void process_message(char *_text, char *_secret, char *_action)
{
	int i, j, n, ch, textlen, secretlen;
	char *text = NULL, *secret = NULL, *action = NULL;

	j = 0;
	secret = _secret;
	secretlen = strlen(secret);
	action = _action;

	if(iflag == 1) {
		while ((n = fgetc(fp_read)) != EOF ) {
			if(n == '\n') {
				if(oflag == 1)
					putc(n, fp_write);	
				else
					putchar('\n');
			} else {
				if(strcmp(action, "encrypt") == 0)
					ch = encrypt_char(n, secret[j]);
				else if(strcmp(action, "decrypt") == 0)
					ch = decrypt_char(n, secret[j]);

				if(oflag == 1)
					putc(ch, fp_write);	
				else
					putchar(ch);

				j++;
				if(j == secretlen)
					j = 0;
			}
		}

		fclose(fp_read);
		if(fp_write != NULL)
			fclose(fp_write);
	} else {
		text = _text;
		textlen = strlen(text);

		i = 0;
		while(i < textlen) {
			if(strcmp(action, "encrypt") == 0)
				ch = encrypt_char(text[i], secret[j]);
			else if(strcmp(action, "decrypt") == 0)
				ch = decrypt_char(text[i], secret[j]);

			putc(ch, stdout);
			i++;
			j++;
			if(j == secretlen)
				j = 0;
		}
		printf("\n");
	}
}

int get_ciphermsg(char *_ciphermsg)
{
	struct termios t_new, t_old;
	char *buffer = _ciphermsg;
	size_t n, bufsize = MAXINPUT;
	int i = 0;

	if(tcgetattr(STDIN_FILENO, &t_old) < 0) {
		printf("tcgetattr() failed");
		exit(-1);
	}

	t_new = t_old;
	t_new.c_lflag &= ~ECHO;

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &t_new) < 0) {
		printf("tcsetattr() failed");
		exit(-1);
	}

	printf("Please enter your ciphertext: ");
	n = getline(&buffer, &bufsize, stdin);

	while(_ciphermsg[i] != '\n')
		i++;

	_ciphermsg[i] = '\0';

	putchar('\n');

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &t_old) < 0) {
		printf("tcsetattr() failed");
		exit(-1);
	}

	return n;
}

void print_tabula_recta()
{
	int i, j, k, l;

	/* prepare the top part of the table */
	printf("   | ");
	for (i = ASCII_A; i <= ASCII_Z; i++)
		printf("%c  ", i);
	putchar('\n');

	/* outer rim */
	for (i = 1; i <= 82; i++)
		printf("-");
	putchar('\n');

	/* print the actual table to stdout */
	for (i = 0; i <= 25; i++) {
		printf(" %c | ", i + ASCII_A);
		for (j = ASCII_A; j <= ASCII_Z; j++) {
			k = i;
			l = j;

			l = l + k;
			if(l <= ASCII_Z)
				printf("%c  ", l);
			else {
				l = l - (ASCII_Z - ASCII_A + 1);
				printf("%c  ", l);
			}
		}
	putchar('\n');
	}
}

void usage()
{
	printf("usage:\ndevignere [-hpt] [-o outputfile] -d|-e message -s|cipherword\n"
		"devignere [-hpt] [-o outputfile] -d|-e -i inputfile -s|cipherword\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int opt, dflag, eflag, sflag;
	char ciphermsg[MAXINPUT];

	dflag = eflag = iflag = oflag = sflag = 0;

	while ((opt = getopt(argc, argv, OPTSTRING)) != -1) {
		switch(opt) {
			case 'd':
					dflag = 1;
					break;
			case 'e':
					eflag = 1;
					break;
			case 'h':
					usage();
					return(EXIT_SUCCESS);
					break;
			case 'i':
					file_to_read = optarg;
					if ((fp_read = fopen(file_to_read, "r")) == NULL) {
						printf("Cannot open file: %s\n", file_to_read);
						return(EXIT_FAILURE);
					}
					iflag = 1;
					break;
			case 'o':
					file_to_write = optarg;
					if ((fp_write = fopen(file_to_write, "w")) == NULL) {
						printf("Cannot open file: %s\n", file_to_write);
						return(EXIT_FAILURE);
					}
					oflag = 1;
					break;
			case 'p':
					pflag = 1;
					break;
			case 's':
					pflag = 1;
					break;
			case 't':
					print_tabula_recta();
					return(EXIT_SUCCESS);
					break;
			default:
					usage();
					return(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	if(iflag == 0 && sflag == 0 && argc < 2)
		usage();
	else if(iflag == 0 && sflag == 1 && argc < 1)
		usage();
	else if(iflag == 1 && sflag == 0 && argc < 1)
		usage();

/*
	if(iflag == 0 && sflag == 0 && strlen(argv[0]) == 0 || strlen(argv[1]) == 0) {
		printf("Cannot have zero length arguments.\n");
		usage();
	}
*/

	if(dflag == 1 && eflag == 1) {
		printf("Cannot decrypt and encrypt at the same time.\n");
		usage();
	}

	if(sflag == 1)
		get_ciphermsg(ciphermsg);
	else if(iflag == 1)
		strncpy(ciphermsg, argv[0], strlen(argv[0]));
	else
		strncpy(ciphermsg, argv[1], strlen(argv[1]));

	if(dflag == 1 && iflag == 0)
		process_message(argv[0], ciphermsg, "decrypt");
	else if(dflag == 1 && iflag == 1)
		process_message(NULL, ciphermsg, "decrypt");
	else if(eflag == 1 && iflag == 0)
		process_message(argv[0], ciphermsg, "encrypt");
	else if(eflag == 1 && iflag == 1)
		process_message(NULL, ciphermsg, "encrypt");

	return(0);
}
