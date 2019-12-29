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

#define LOWER 		0x20
#define UPPER 		0x7E
#define LOOP 		(UPPER - LOWER + 1)
#define ASCII_A		0x41
#define ASCII_Z		0x5A
#define MAXINPUT 	1024
#define OPTSTRING 	"dehi:o:pst"

FILE *fp_read = NULL, *fp_write = NULL;
int iflag, oflag, pflag;

int encrypt_char(char, char);
int decrypt_char(char, char);
void process_message(char *, char *, char *);
int get_secret_phrase(char *);
void print_tabula_recta();
void usage();

int encrypt_char(char _char_to_encrypt, char _secret_char)
{
	char cte = _char_to_encrypt;
	char sct = _secret_char;
	int encch;

	if(cte == ' ' && pflag)
		encch = cte;
	else {
		encch = cte + sct - LOWER;
		if(encch > UPPER)
			encch = encch - LOOP;
	}

	return encch;
}

int decrypt_char(char _char_to_decrypt, char _secret_char)
{
	char ctd = _char_to_decrypt;
	char sct = _secret_char;
	int decch;

	if(ctd == ' ' && pflag)
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

	if(iflag) {
		while ((n = fgetc(fp_read)) != EOF ) {
			if(n == '\n') {
				if(oflag)
					putc(n, fp_write);	
				else
					putchar('\n');
			} else {
				if(!strcmp(action, "encrypt"))
					ch = encrypt_char(n, secret[j]);
				else if(!strcmp(action, "decrypt"))
					ch = decrypt_char(n, secret[j]);

				if(oflag)
					putc(ch, fp_write);	
				else
					putchar(ch);

				j++;
				if(j == secretlen)
					j = 0;
			}
		}

		fclose(fp_read);
		if(fp_write)
			fclose(fp_write);
	} else {
		text = _text;
		textlen = strlen(text);

		i = 0;
		while(i < textlen) {
			if(!strcmp(action, "encrypt"))
				ch = encrypt_char(text[i], secret[j]);
			else if(!strcmp(action, "decrypt"))
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

int get_secret_phrase(char *_secret_phrase)
{
	struct termios t_new, t_old;
	char *buffer = _secret_phrase;
	size_t n, bufsize = MAXINPUT;
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
	n = getline(&buffer, &bufsize, stdin);

	while(_secret_phrase[i] != '\n')
		i++;
	_secret_phrase[i] = '\0';
	putchar('\n');

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &t_old) < 0) {
		fprintf(stderr, "tcsetattr() failed\n");
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
	printf("usage:\ndevignere [-hpst] [-o outputfile] -d|-e message secret_phrase\n"
		"devignere [-hpst] [-o outputfile] -d|-e -i inputfile secret_phrase\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int opt, dflag, eflag, sflag;
	char secretphrase[MAXINPUT];

	dflag = eflag = sflag = iflag = oflag = pflag = 0;

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
					break;
			case 'i':
					if (!(fp_read = fopen(optarg, "r"))) {
						fprintf(stderr, "Cannot open file: %s\n", optarg);
						return(EXIT_FAILURE);
					}
					iflag = 1;
					break;
			case 'o':
					if (!(fp_write = fopen(optarg, "w"))) {
						fprintf(stderr, "Cannot open file: %s\n", optarg);
						return(EXIT_FAILURE);
					}
					oflag = 1;
					break;
			case 'p':
					pflag = 1;
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
	if (!iflag && !sflag && argc < 2)
		usage();
	else if (!iflag && sflag && argc < 1)
		usage();
	else if (iflag && !sflag && argc < 1)
		usage();
	else if (!iflag && !sflag && !strlen(argv[0])
			|| !iflag && !sflag && !strlen(argv[1])) {
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
	if (sflag && dflag && !iflag) {
		printf("!!!DEBUG sflag is set, iflag is not set, decrypting, argv[0] %s\n", argv[0]);
		get_secret_phrase(secretphrase);
		process_message(argv[0], secretphrase, "decrypt");
	} else if (sflag && dflag && iflag) {
		get_secret_phrase(secretphrase);
		process_message(NULL, secretphrase, "decrypt");
	} else if (!sflag && dflag && !iflag) {
		process_message(argv[0], strncpy(secretphrase, argv[1], strlen(argv[1])), "decrypt");
	} else if (!sflag && dflag && iflag) {
		process_message(NULL, strncpy(secretphrase, argv[0], strlen(argv[0])), "decrypt");
	} else if (sflag && eflag && !iflag) {
		get_secret_phrase(secretphrase);
		process_message(argv[0], secretphrase, "encrypt");
	} else if (sflag && eflag && iflag) {
		get_secret_phrase(secretphrase);
		process_message(NULL, secretphrase, "encrypt");
	} else if (!sflag && eflag && !iflag) {
		process_message(argv[0], strncpy(secretphrase, argv[1], strlen(argv[1])), "encrypt");
	} else /* !sflag && eflag && iflag */
		process_message(NULL, strncpy(secretphrase, argv[0], strlen(argv[0])), "encrypt");

	return 0;
}
