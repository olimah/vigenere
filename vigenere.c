/*
 * Copyright (c) 2020 Oliver Mahmoudi
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
#include <getopt.h>

#define LOWER 		0x20
#define UPPER 		0x7E
#define LOOP 		(UPPER - LOWER + 1)
#define ASCII_A		0x41
#define ASCII_Z		0x5A
#define MAXINPUT 	1024
#define OPTSTRING 	"dehi:o:pst"
#define ENCRYPT		"encrypt"
#define DECRYPT		"decrypt"

FILE *fp_read = NULL, *fp_write = NULL;

int encrypt_char(int, int);
int decrypt_char(int, int);
void process_message(char *, char *, char *);
void get_secret_phrase(char *);
void print_tabula_recta();
void usage();

int
encrypt_char(int _char_to_encrypt, int _secret_char)
{
	int cte = _char_to_encrypt;
	int sct = _secret_char;
	int encch;

	if(cte == '\n' || cte == '\t')
		return cte;

	encch = cte + sct - LOWER;
	if(encch > UPPER)
		encch = encch - LOOP;

	return encch;
}

int
decrypt_char(int _char_to_decrypt, int _secret_char)
{
	int ctd = _char_to_decrypt;
	int sct = _secret_char;
	int decch;

	if(ctd == '\n' || ctd == '\t')
		return ctd;

	decch = ctd - sct + LOWER;
	if(decch < LOWER)
		decch = decch + LOOP;

	return decch;
}

void
process_message(char *_text, char *_secret, char *_action)
{
	int i, j, n, textlen, secretlen;
	char *text = NULL, *secret = NULL, *action = NULL;

	j = 0;
	secret = _secret;
	secretlen = strlen(secret);
	action = _action;

	if(fp_read && !strcmp(action, ENCRYPT)) { /* fp_read => *_text == NULL */
		while ((n = fgetc(fp_read)) != EOF ) {
			if(fp_write)
				putc(encrypt_char(n, secret[j]), fp_write);
			else
				putc(encrypt_char(n, secret[j]), stdout);

			j++;
			if(j == secretlen)
				j = 0;
		}

		fclose(fp_read);
		if(fp_write)
			fclose(fp_write);
	} else if(fp_read && !strcmp(action, DECRYPT)) { /* fp_read => *_text == NULL */
		while ((n = fgetc(fp_read)) != EOF ) {
			if(fp_write)
				putc(decrypt_char(n, secret[j]), fp_write);
			else
				putc(decrypt_char(n, secret[j]), stdout);

			j++;
			if(j == secretlen)
				j = 0;
		}

		fclose(fp_read);
		if(fp_write)
			fclose(fp_write);
	} else if(!fp_read && !strcmp(action, ENCRYPT)) { /* !fp_read => *_text != NULL */
		text = _text;
		textlen = strlen(text);

		i = 0;
		while(i < textlen) {
			if(fp_write)
				putc(encrypt_char(text[i], secret[j]), fp_write);
			else
				putc(encrypt_char(text[i], secret[j]), stdout);

			i++; j++;
			if(j == secretlen)
				j = 0;
		}

		if(fp_write) {
			putc('\n', fp_write);
			fclose(fp_write);
		} else
			putchar('\n');
	} else if(!fp_read && !strcmp(action, DECRYPT)) { /* !fp_read => *_text != NULL */
		text = _text;
		textlen = strlen(text);

		i = 0;
		while(i < textlen) {
			if(fp_write)
				putc(decrypt_char(text[i], secret[j]), fp_write);
			else
				putc(decrypt_char(text[i], secret[j]), stdout);

			i++; j++;
			if(j == secretlen)
				j = 0;
		}

		if(fp_write) {
			putc('\n', fp_write);
			fclose(fp_write);
		} else
			putchar('\n');
	}
}

void
get_secret_phrase(char *_secret_phrase)
{
	struct termios t_new, t_old;
	char *buffer = NULL;
	size_t bufsize = 0;
	int i = 0;

	if(tcgetattr(STDIN_FILENO, &t_old) < 0) {
		fprintf(stderr, "tcgetattr() failed\n");
		exit(-1);
	}

	t_new = t_old;
	t_new.c_lflag &= ~ECHO;

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &t_new) < 0) {
		fprintf(stderr, "tcsetattr() failed\n");
		exit(-1);
	}

	printf("Enter your secret phrase: ");
	getline(&buffer, &bufsize, stdin);

	while(i < MAXINPUT - 1 && buffer[i] != '\n') {
		_secret_phrase[i] = buffer[i];
		i++;
	}
	_secret_phrase[i] = '\0';
	putchar('\n');

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &t_old) < 0) {
		fprintf(stderr, "tcsetattr() failed\n");
		exit(-1);
	}
}

void
print_tabula_recta()
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

void
usage()
{
	printf("usage:\nvigenere [-ht] [-o outputfile] -d|-e -s|secret_phrase message\n"
		"vigenere [-ht] [-o outputfile] -d|-e -i inputfile -s|secret_phrase\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int opt, dflag, eflag, sflag;
	char secretphrase[MAXINPUT];

	dflag = eflag = sflag = 0;

    static struct option long_options[] = {
  		{"decrypt",   no_argument, 0,  'd'},
	    {"encrypt",  no_argument, 0,  'e'},
    	{"help",  no_argument, 0,  'h'},
  		{"inputfile",   required_argument, 0,  'i'},
	    {"outputfile",  required_argument, 0,  'o'},
	    {"secret-phrase",  no_argument, 0,  's'},
    	{"tabula-recta",  no_argument, 0,  't'},
	    {0, 0, 0, 0}
    };

	while ((opt = getopt_long(argc, argv, OPTSTRING, long_options, NULL)) != -1) {
		switch(opt) {
			case 'd':
					dflag = 1;
					break;
			case 'e':
					eflag = 1;
					break;
			case 'h':
					usage();
					break;
			case 'i':
					if (!(fp_read = fopen(optarg, "r"))) {
						fprintf(stderr, "Cannot open file: %s\n", optarg);
						return(EXIT_FAILURE);
					}
					break;
			case 'o':
					if (!(fp_write = fopen(optarg, "w"))) {
						fprintf(stderr, "Cannot open file: %s\n", optarg);
						return(EXIT_FAILURE);
					}
					break;
			case 's':
					sflag = 1;
					break;
			case 't':
					print_tabula_recta();
					return(EXIT_SUCCESS);
					break;
			default:
					usage();
		}
	}
	argc -= optind;
	argv += optind;

	/*
	 * Safety checks
	 */
	if (!fp_read && !sflag && argc < 2)
		usage();
	else if (!fp_read && sflag && argc < 1)
		usage();
	else if (fp_read && !sflag && argc < 1)
		usage();
	else if (!fp_read && !sflag && !strlen(argv[0])
			|| !fp_read && !sflag && !strlen(argv[1])) {
		fprintf(stderr, "Cannot have zero length arguments!\n");
		usage();
	} else if (dflag && eflag) {
		fprintf(stderr, "Cannot decrypt and encrypt at the same time!\n");
		usage();
	} else if (!dflag && !eflag) {
		fprintf(stderr, "Need to either decrypt or encrypt!\n");
		usage();
	}

	/*
	 * Go...
	 */
	if (sflag && dflag && !fp_read) {
		get_secret_phrase(secretphrase);
		process_message(argv[0], secretphrase, DECRYPT);
	} else if (sflag && dflag && fp_read) {
		get_secret_phrase(secretphrase);
		process_message(NULL, secretphrase, DECRYPT);
	} else if (!sflag && dflag && !fp_read) {
		process_message(argv[1], strncpy(secretphrase, argv[0], strlen(argv[0])), DECRYPT);
	} else if (!sflag && dflag && fp_read) {
		process_message(NULL, strncpy(secretphrase, argv[0], strlen(argv[0])), DECRYPT);
	} else if (sflag && eflag && !fp_read) {
		get_secret_phrase(secretphrase);
		process_message(argv[0], secretphrase, ENCRYPT);
	} else if (sflag && eflag && fp_read) {
		get_secret_phrase(secretphrase);
		process_message(NULL, secretphrase, ENCRYPT);
	} else if (!sflag && eflag && !fp_read) {
		process_message(argv[1], strncpy(secretphrase, argv[0], strlen(argv[0])), ENCRYPT);
	} else /* !sflag && eflag && fp_read */
		process_message(NULL, strncpy(secretphrase, argv[0], strlen(argv[0])), ENCRYPT);

	return(EXIT_SUCCESS);
}
