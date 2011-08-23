/*
 * Copyright (c) 2002, 2010
 * FREEBSD HACKERS NET (http://www.freebsdhackers.net) All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by FREEBSD HACKERS NET,
 *       http://www.freebsdhackers.net
 * 4. Neither the name "FREEBSD HACKERS NET" nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE FREEBSD HACKERS NET ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL FREEBSD HACKERS NET OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * MAINTAINER: Shane Kinney <modsix@gmail.com>
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include 	<unistd.h>
#include	<errno.h>
#include	<fts.h>

#define	MAXBUFFSIZE	8192

void 	usage();
void 	setup(unsigned long filesize, char *path, int sflag);
void 	traverse(char **argv, int fflag, int qflag, int sflag); 
void 	destroy(int read_fd, int write_fd, char *buf, int sz_buf, unsigned long filesize);
void 	unlink_special(char *path);

int main(int argc, char *argv[]) {

	int ch, fflag, qflag, sflag;
		
	if(argc < 2) {
		usage();
		exit(1);
	}
	
	/* don't want getopt() writing to stderr -- set flags to 0 for good mesasure. */
	opterr = fflag = qflag = sflag = 0;

	while((ch = getopt(argc, argv, "s:fqh")) != -1) {
		switch(ch) {
		case 's': 
			sflag = atoi(optarg);
			if(sflag < 1 || sflag > 20) { // The number of overwrites must between 1 and 20
				usage();
				exit(1);
			}
			break;
		case 'f':
			fflag = 1;
			break;
		case 'q':
			qflag = 1;
			break;
		case 'h':
			usage();
		case '?':
			/* see `man 3 getopt` */
			fprintf(stderr, "Unrecognized option: -%c\n", optopt);
			usage();
		default:
			usage();
		}
	}

	/* If user didn't set a default sflag number, set it to 7 (US DOD Standard). */
	if(!sflag) { 
		sflag = 7;
	}

	argc -= optind;
	argv += optind;

	if(argc < 1) {
		usage();
	}
	
	traverse(argv, fflag, qflag, sflag);	
	return 0;
}

void traverse(char **argv, int fflag, int qflag, int sflag) { 
	FTS		*fts;
	FTSENT		*p;
	int		dirCount, flags;
	unsigned long	filesize;
	flags 		= FTS_PHYSICAL;  
	dirCount 	= 0;

	/* Open the path given from the command line */	
	if(!(fts = fts_open(argv, flags, NULL))) { 
		perror("Error!: ");
		exit(1);
	}

	/* Recursivly attempt to destroy all valid files */
	while((p = fts_read(fts)) != NULL) {
	
		/* Now look the stat of the file and determine the st_mode */
		switch(p->fts_statp->st_mode & S_IFMT) {
			
		case S_IFIFO:	/* named pipe (fifo) */
			if(fflag) {
				if(!qflag) {
					printf("Named Pipe: \t %s \n", p->fts_accpath);
				}
				unlink_special(p->fts_accpath);
			} else if(!qflag) { 
				fprintf(stderr, "Named Pipe: \t %s \t Skipping...no '-f' flag given!\n", p->fts_accpath);
			}
			break;

		case S_IFCHR:	/* character special */
			if(fflag) {
				if(!qflag) {
					printf("Character Special: \t %s \n", p->fts_accpath);
				}
				unlink_special(p->fts_accpath);
			} else if(!qflag) {
				fprintf(stderr, "Character Special: \t %s \t Skipping...no '-f' flag given!\n", p->fts_accpath);
			}
			break;
		
		case S_IFDIR:	/* directory */
			if(!qflag) { 
				printf("Directory: \t %s \n", p->fts_accpath);
			}
			rmdir(p->fts_accpath);		
			break;
		
		case S_IFBLK:	/* block special */
			if(fflag) {
				printf("Block Special: \t %s \n", p->fts_accpath);
				unlink_special(p->fts_accpath);
			} else if(!qflag) {
				fprintf(stderr, "Block Special: \t %s \t Skipping...no '-f' flag given!\n", p->fts_accpath);
			}
			break;
	
		case S_IFREG:	/* regular */
			filesize = p->fts_statp->st_size;
			if(!qflag) {
				printf("Regular File: \t %s \t Size: \t %lu Bytes\n", p->fts_accpath, filesize);
			} 
		
			setup(filesize, p->fts_accpath, sflag);
			break;
		
		case S_IFLNK:	/* symbolic link */
			if(fflag) {
				if(!qflag) {
					printf("Symbolic Link: \t %s \n", p->fts_accpath);
				}
				unlink_special(p->fts_accpath);
			} else if(!qflag) {
				fprintf(stderr, "Symbolic Link: \t %s \t Skipping... no '-f' flag set!\n", p->fts_accpath);
			}
			break;
		
		case S_IFSOCK:	/* socket */
			if(fflag) {
				if(!qflag) {
					printf("Socket: \t %s \n", p->fts_accpath);
				}
				unlink_special(p->fts_accpath);
			} else if(!qflag) {
				fprintf(stderr, "Socket: \t %s \t Skipping... no '-f' flag set!\n", p->fts_accpath);
			}
			break;
#ifdef __FreeBSD__			
		case S_IFWHT:	/* whiteout is only a FreeBSD type */
			if(fflag) {
				if(!qflag) {
					printf("Whiteout: \t %s \n", p->fts_accpath);
				}
				unlink_special(p->fts_accpath);
			} else if(!qflag) {
				fprintf(stderr, "Whiteout: \t %s \t Skipping... no '-f' flag set!\n", p->fts_accpath);
			}
			break;
#endif
		default: 
			if(!qflag) {
				fprintf(stderr, "Type Unknown...\n");
			}
		}
	}
}

void setup(unsigned long filesize, char *path, int sflag) { 
	struct	stat sb;	
	int		i, fd_rand, fd_zero, fd_userfile;
	char		*buf;
	char		*devrand = "/dev/urandom";
	char		*devzero = "/dev/zero";
	int      	mode = O_WRONLY | O_EXCL;

	int	result = 0;

#if defined(O_NOFOLLOW)
	mode |=  O_NOFOLLOW;
#endif
	
	/* allocate our buffer memory. */
	if((buf = (char *) malloc(MAXBUFFSIZE * sizeof(char))) == NULL) {
		perror("Error ");
		exit(1);
	}

	/* Open /dev/zero get and get a file descriptor for it */
	if((fd_zero = open(devzero, O_RDONLY)) < 0) {
		perror("Error ");
		usage();
		exit(1);
	}

	/* Open our user file, and get a file descriptor for it */
	if((fd_userfile = open(path, mode)) < 0) {
		perror("Error ");
		usage();
		exit(1);
	}

	/* Open /dev/urandom and get a file descriptor for it */
	if((fd_rand = open(devrand, O_RDONLY)) < 0) {
		perror("Error ");
		usage();
		exit(1);
	}

	/* Do each over write (rand & null) 'sflag' times (specified by cli). */
	for(i = 0; i < sflag; i++) { 
		destroy(fd_rand, fd_userfile, buf, MAXBUFFSIZE, filesize);
		destroy(fd_zero, fd_userfile, buf, MAXBUFFSIZE, filesize);
	}

	/* close everything open: */
	close(fd_userfile);
	close(fd_rand);
	close(fd_zero);

	/* Unlink our file */
	if(unlink(path) < 0) {
		perror("Error ");
		exit(1);
	}

	/* Free our malloc()'d memory */
	free(buf);
}

void destroy(int fd_read, int fd_write, char *buf, int sz_buf, unsigned long filesize) {
	
	int 	offset = 0;
	int 	bytes_read;
	int 	cursz_buf = sz_buf;  // current number of bytes to fill buffer with

	/* Seek to the beginning of the file. */
	if(lseek(fd_write, offset, SEEK_SET) < 0) {
		perror("Error ");
		exit(1);
	}

	/* Loop through file */
	while(offset < filesize) {

		/* If the size of the file is less than MAXBUFFSIZE */
		if((filesize - offset) < sz_buf) {
			cursz_buf = filesize - offset;
		}
		
		/* Read bytes from /dev/urandom or /dev/zero */
		if((bytes_read = read(fd_read, buf, cursz_buf)) < 0) {
			perror("Error ");
			exit(1);
		}

		/* Write out the bytes to the file. */
		if(write(fd_write, buf, bytes_read) != bytes_read) {
			perror("Error ");
			exit(1);
		}

		offset += bytes_read;
	}

	/* fsync our data the hard disk. */ 
	if((fsync(fd_write) < 0) && (errno == EIO)) {
		perror("Error ");
		exit(1);
	}
}

void unlink_special(char *path) { 
	/* Unlink the special files we encounter if the -f (fflag) is set. */
	if(unlink(path) < 0) {
		perror("Error ");
		exit(1);
	}
}

void usage() {
	fprintf(stderr, "Usage: destroy [ -s <int> ] [ -f ] [ -q ] [ -h ] <[file(s) | directory]>\n");
	fprintf(stderr, " -s: The number of overwrites to each file.\n"); 
	fprintf(stderr, "\tThe default is set to 5, the minimum allowed is 1, and the maximum allowed is 20.\n");
	fprintf(stderr, " -f: Enable force deletion of special file types.\n");
	fprintf(stderr, " -q: Enable quiet output.\n");
	fprintf(stderr, " -h: Print this help screen.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "View the man page file for all the info: `man 1 destroy`\n");
	fprintf(stderr, "For more help:\n");
	fprintf(stderr, "\tGo to: irc.freebsdhackers.net #freebsd\n");
	fprintf(stderr, "\tOr to: http://www.kinneysoft.com\n");
	fprintf(stderr, "\tOr write to: modsix@gmail.com\n");
	exit(1);
}
