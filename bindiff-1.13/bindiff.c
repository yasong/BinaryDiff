/*
 |
 | $Source: /home/hugh/sources/misc/bindiff/bindiff.c,v $
 | $Revision: 1.13 $  $Author: hugh $
 | $State: Exp $  $Locker: hugh $
 | $Date: 2008/03/31 01:12:47 $
 |
 */

char License[] = "bindiff is distributed under the Artistic License, "
                 "included in the source code distribution.";


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define TRUE  1
#define FALSE 0

#define DIFF_LEN 16

#define min(a, b) (a < b ? a : b)

char version[] = "@(#)bindiff $Revision: 1.13 $";

struct diff_info {
	off_t diff_start;       // offset in file
	off_t diff_end;
	struct diff_info *next; // next set of diffs
	};

struct file_info {
	char *name;                  // name of file
	int fd;                      // file descriptor
	size_t length;               // length of file
	unsigned char *buffer;       // contents of file
	off_t cp;                    // current index
	off_t ahead;                 // look ahead index
	struct diff_info *diff_list;
	};


struct file_info f1, f2;

struct lines {
	struct lines *next;
	unsigned char buffer[DIFF_LEN];
	unsigned char length;
	unsigned int byte_offset;
	};

struct lines *file_1, *pnt1;
struct lines *file_2, *pnt2;
struct lines *line_malloc();

char *file_1_name = NULL;
char *file_2_name = NULL;

char buffer1;
char buffer2;

int len1;
int len2;

int file_1_fd;
int file_2_fd;

int nodiffs=TRUE;
int stdin_used = FALSE;
int buff_counter = 0;

struct stat file_1_stat;
struct stat file_2_stat;

extern int errno;
// extern char *sys_errlist[];
extern int sys_nerr;

int header_printed = FALSE;

int end_of_file_1 = FALSE;
int end_of_file_2 = FALSE;
int end_of_files = FALSE;

int min_same = 8;

void 
usage()
{
	fprintf(stderr, "usage: bindiff [--version] [--help] [-s num] file1 file2\n"
	                "       --help            usage info (this information\n"
	                "       -s number         number of bytes same to end diff\n"
	                "       --version         get version information\n");
	exit(1);
}

void 
read_file(struct file_info *f)
{
	int count = 0;
	int retval;

	f->buffer = (char *)malloc(f->length);
	if (f->buffer == NULL)
	{
		fprintf(stderr, "unable to allocate memory\n");
		exit(1);
	}
	do
	{
		retval = read(f->fd, f->buffer + count, min(1024, (f->length - count)));
		if (retval == -1)
		{
			perror("error on read: ");
			exit(1);
		}
		count += retval;
	}
	while (count < f->length);
}

struct diff_info *
add2list(struct file_info *f, struct diff_info *dl)
{
	struct diff_info *d;
	if (f->diff_list == NULL)
	{
		f->diff_list = d = dl;
	}
	else
	{
		d = f->diff_list;
		while (d->next != NULL)
			d = d->next;
		d->next = dl;
		d = d->next;
	}
	d->next = NULL;
	return(d);
}

int 
find_match(struct file_info *a, struct file_info *b)
{
	int off = 0;
	int match_found = FALSE;
	struct diff_info *dl1, *dl2;
	int diffcount = 0;
	struct diff_info one[2], two[2];
	int match1=FALSE, match2=FALSE;
	int off1=0, off2=0;

		dl1 = (struct diff_info *)malloc(sizeof (struct diff_info));
		dl2 = (struct diff_info *)malloc(sizeof (struct diff_info));
		a->ahead = dl1->diff_start = a->cp;
		b->ahead = dl2->diff_start = b->cp;
		match_found = FALSE;

	// start looking for matches
	// first start with 'a' and look forward in 'b' to a match
	while (!match_found)
	{
		while (((int)*(int *)&(a->buffer[a->ahead]) != (int)*(int *)&(b->buffer[b->ahead])) && 
		       (a->ahead < a->length) && (b->ahead < b->length))
		{
			b->ahead++;
		}
		if ((a->buffer[a->ahead] == b->buffer[b->ahead]) 
		    && (b->ahead < b->length) && (a->ahead < a->length))
		{
			int off = 1;
			if (a->ahead < a->length)
			{
				while ((a->buffer[a->ahead + off] == b->buffer[b->ahead + off]) 
					&& ((a->ahead + off) < a->length) && ((b->ahead + off) < b->length))
				{
					off++;
				}
				if ((off > min_same) || ((a->ahead + off) == a->length) || ((b->ahead + off) == b->length))
				{
					one[0].diff_end = a->ahead;
					one[0].diff_start = a->cp;
					one[1].diff_start = b->cp;
					one[1].diff_end = b->ahead;
					off1 = off;
					match_found = match1 = TRUE;
					break;
				}
			}
			if (((a->ahead + off) < a->length) && ((b->ahead + off) < b->length))
			{
				b->ahead++;
			}
			else if ((a->ahead + off) < a->length)
			{
				a->ahead++;
				b->ahead = b->cp;
			}
		}
		else
		{
			if ((a->ahead < a->length) && (b->cp < b->length))
			{
				a->ahead++;
				b->ahead = b->cp;
			}
			else
			{
				break;
			}
		}
	}
	
	if (!match_found)
	{
		one[0].diff_end = a->length;
		one[0].diff_start = a->cp;
		one[1].diff_start = b->cp;
		one[1].diff_end = b->length;
		off1 = 0;
		match1 = FALSE;
	}

	// now start with 'b' and look forward in 'a' to a match
	a->ahead = a->cp;
	b->ahead = b->cp;

	match_found = FALSE;
	while (!match_found)
	{
		while (((int)*(int *)&(a->buffer[a->ahead]) != (int)*(int *)&(b->buffer[b->ahead])) && 
		       (a->ahead < a->length) && (b->ahead < b->length))
		{
			a->ahead++;
		}
		if ((a->buffer[a->ahead] == b->buffer[b->ahead]) 
		    && (b->ahead < b->length) && (a->ahead < a->length))
		{
			int off = 1;
			if (b->ahead < b->length)
			{
				while ((a->buffer[a->ahead + off] == b->buffer[b->ahead + off]) 
					&& ((a->ahead + off) < a->length) && ((b->ahead + off) < b->length))
				{
					off++;
				}
				if ((off > min_same) || ((a->ahead + off) == a->length) || ((b->ahead + off) == b->length))
				{
					two[0].diff_end = a->ahead;
					two[0].diff_start = a->cp;
					two[1].diff_start = b->cp;
					two[1].diff_end = b->ahead;
					off2 = off;
					match_found = match2 = TRUE;
					break;
				}
			}
			if (((a->ahead + off) < a->length) && ((b->ahead + off) < b->length))
			{
				a->ahead++;
			}
			else if ((b->ahead + off) < b->length)
			{
				b->ahead++;
				a->ahead = a->cp;
			}
		}
		else
		{
			if ((b->ahead < b->length) && (a->cp < a->length))
			{
				b->ahead++;
				a->ahead = a->cp;
			}
			else
			{
				break;
			}
		}
	}

	if (!match_found)
	{
		two[0].diff_end = a->length;
		two[0].diff_start = a->cp;
		two[1].diff_start = b->cp;
		two[1].diff_end = b->length;
		off2 = 0;
		match2 = FALSE;
	}

	match_found = TRUE;

	if (match1 && match2)
	{
		// if first match in a/b longer than match in b/a
		if (off1 > off2)
		{
			dl1->diff_end = one[0].diff_end;
			dl2->diff_end = one[1].diff_end;
		}
		else
		{
			dl1->diff_end = two[0].diff_end;
			dl2->diff_end = two[1].diff_end;
		}
	}
	else if (match1)
	{
		dl1->diff_end = one[0].diff_end;
		dl2->diff_end = one[1].diff_end;
	}
	else if (match2)
	{
		dl1->diff_end = two[0].diff_end;
		dl2->diff_end = two[1].diff_end;
	}
	else
	{
		dl1->diff_end = a->length;
		dl2->diff_end = b->length;
	}

	add2list(a, dl1);
	add2list(b, dl2);

	a->cp = dl1->diff_end;
	b->cp = dl2->diff_end;
	return(match_found);
}

main(argc, argv)
int argc;
char *argv[];
{
	int result;
	int diff_counter;
	int done = FALSE;

#ifdef DEBUG
	if (!strcmp(argv[argc -1], "-DEBUG"))
		{
/*
 |	The following code will start an hpterm which will in turn execute 
 |	a debugger and "adopt" the process, so it can be debugged.
 */

			int child_id;
			char command_line[256];

			child_id = getpid();
			if (!fork())
			{
				sprintf(command_line, 
				    "hpterm -fg wheat -bg DarkSlateGrey -n %s -e xdb %s -P %d", 
					"hello",  argv[0], child_id);
				execl("/bin/sh", "sh", "-c", command_line, NULL);
				fprintf(stderr, "could not exec new window\n");
				exit(1);
			}

/*
 |	When in debugger, set child_id to zero (p child_id=0) to continue 
 |	executing the software.
 */

			while (child_id != 0)
				;
			argc--;
		}
#endif

	get_options(argc, argv);

	if (!stdin_used)
	{
		result = stat(f1.name, &file_1_stat);
		if (result == -1)
		{
			perror("stat call: ");
			f1.length = 0;
		}
		else
		{
			f1.length = file_1_stat.st_size;
		}
		result = stat(f2.name, &file_2_stat);
		if (result == -1)
		{
			perror("stat call: ");
			f2.length = 0;
		}
		else
		{
			f2.length = file_2_stat.st_size;
		}

		if (file_1_stat.st_size != file_2_stat.st_size)
		{
			standard_header();
			printf("files are not the same size!\n");
			printf("< %d bytes\n> %d bytes\n\n", file_1_stat.st_size, file_2_stat.st_size);
		}
	}

	read_file(&f1);
	read_file(&f2);
	f1.cp = f2.cp = f1.ahead = f2.ahead = 0;
	f1.diff_list = f2.diff_list = NULL;

	while (!done)
	{
		char match_found;

		while ((f1.buffer[f1.cp] == f2.buffer[f2.cp]) && 
		       ((f1.cp < f1.length) && (f2.cp < f2.length)))
		{
			f1.cp++;
			f2.cp++;
		}
		match_found = find_match( &f1, &f2);
		if ((f1.cp == f1.length) || (f2.cp == f2.length))
		{
			done = TRUE;
		}
	}
	print_diffs();
}

get_options(argc, argv)
int argc;
char *argv[];
{
	int count;

	count = 1;
	while (count < argc)
	{
		if (!strcmp(argv[count], "-"))
		{
			if (file_1_name == NULL)
			{
				f1.name = "-";
				f1.fd = 0;
				stdin_used = TRUE;
			}
			else if (file_2_name == NULL)
			{
				if (stdin_used)
				{
					fprintf(stderr, "stdin cannot be used for both files!\n");
					exit(1);
				}
				f2.name = "-";
				f2.fd = 0;
				stdin_used = TRUE;
			}
		}
		else if (!strcmp(argv[count], "--version"))
		{
			fprintf(stdout, "%s\n", version);
			exit(0);
		}
		else if ((!strcmp(argv[count], "-s")) || (!strcmp(argv[count], "--same")))
		{
			count++;
			min_same = atoi(argv[count]);
			if (min_same == 0)
			{
				fprintf(stderr, "error: improper value for minimum same size\n");
				exit(0);
			}
		}
		else if (!strcmp(argv[count], "--help"))
		{
			usage();
		}
		else
		{
			if (f1.name == NULL)
			{
				f1.name = argv[count];
				f1.fd = open(f1.name, O_RDONLY, 0);
				if (f1.fd == -1)
				{
					fprintf(stderr, "unable to open file '%s'\n", f1.name);
					exit(1);
				}
			}
			else if (f2.name == NULL)
			{
				f2.name = argv[count];
				f2.fd = open(f2.name, O_RDONLY, 0);
				if (f2.fd == -1)
				{
					fprintf(stderr, "unable to open file '%s'\n", f2.name);
					exit(1);
				}
			}
		}
		count++;
	}
}

standard_header()
{
	if (header_printed)
		return;

	printf("                                   binary diff\n\n");
	printf("< %s\n> %s\n\n", f1.name, f2.name);
	header_printed = TRUE;
}

get_bytes()
{
	len1 = read(file_1_fd, &buffer1, 1);
	len2 = read(file_2_fd, &buffer2, 1);

	if (len1 < 1)
		end_of_file_1 = TRUE;

	if (len2 < 1)
		end_of_file_2 = TRUE;

	if ((len1 > 0) || (len2 > 0))
	{
		buff_counter++;
		return(1);
	}
	else
		return(0);
}

struct lines *
line_malloc()
{
	struct lines *tmp;

	tmp = (struct lines *)malloc(sizeof(struct lines));
	tmp->length = 0;
	tmp->next = NULL;
	return(tmp);
}

print_diffs()
{
	struct diff_info *tmp1, *tmp2;
	char counter;

	standard_header();

	tmp1 = f1.diff_list;
	tmp2 = f2.diff_list;

	/*
	 |	print out different bytes
	 */

	while (tmp1 != NULL)
	{
		int count = 0;
		int len = tmp1->diff_end - tmp1->diff_start;
		char *ptr = f1.buffer;

		if ((tmp1->diff_start == tmp1->diff_end)
		    && (tmp2->diff_start == tmp2->diff_end))
		{
			tmp1 = tmp1->next;
			tmp2 = tmp2->next;
			continue;
		}

		printf("%d,%d - %d,%d\n", tmp1->diff_start, tmp1->diff_end, tmp2->diff_start, tmp2->diff_end);

		while (count < len)
		{
			printf("< %08x  ", (tmp1->diff_start + count));
			/*
			 |	print out the individual bytes in hex
			 */

			counter = 0;
			while (counter < DIFF_LEN)
			{
				if ((count + counter) < len)
					printf("%02x ", f1.buffer[tmp1->diff_start + count + counter]);
				else
					printf("   ");
				counter++;
			}
			/*
			 |	print out the individual bytes as printing characters
			 */
			counter = 0;
			printf("\"");
			while (counter < DIFF_LEN)
			{
				if (((counter + count) < len) && 
				    ((f1.buffer[tmp1->diff_start + counter + count] >= ' ') && (f1.buffer[tmp1->diff_start + counter + count] <= '~')))
					printf("%c", f1.buffer[tmp1->diff_start + count + counter]);
				else if ((count + counter) < len)
					printf(".");
				else
					printf(" ");
				counter++;
			}
			count += counter;
			printf("\"\n");
		}

	    	count = 0;
	    	len = tmp2->diff_end - tmp2->diff_start;

		if (len > 0)
			printf(" ----------\n");
		else
			printf("\n");

		while (count < len)
		{
			printf("> %08x  ", (tmp2->diff_start + count));
			/*
			 |	print out the individual bytes in hex
			 */

			counter = 0;
			while (counter < DIFF_LEN)
			{
				if ((count + counter) < len)
					printf("%02x ", f2.buffer[tmp2->diff_start + counter + count]);
				else
					printf("   ");
				counter++;
			}
			/*
			 |	print out the individual bytes as printing characters
			 */
			counter = 0;
			printf("\"");
			while (counter < DIFF_LEN)
			{
				if (((count + counter) < len) && 
				    ((f2.buffer[tmp2->diff_start + counter + count] >= ' ') 
				    && (f2.buffer[tmp2->diff_start + counter + count] <= '~')))
					printf("%c", f2.buffer[tmp2->diff_start + count + counter]);
				else if ((count + counter) < len)
					printf(".");
				else
					printf(" ");
				counter++;
			}
			count += counter;
			printf("\"\n");
		}

		tmp1 = tmp1->next;
		tmp2 = tmp2->next;
	}
	printf("\n");
}

