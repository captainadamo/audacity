/*
** Copyright (C) 1999-2002 Erik de Castro Lopo <erikd@zip.com.au>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/	 


#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>

#include	<sndfile.h>

#define	 BUFFER_LEN      1024


typedef	struct
{	char	*infilename, *outfilename ;
	SF_INFO	infileinfo, outfileinfo ;
} OptionData ;

typedef struct
{	char *ext ;
	int  len  ;
	int  format ;
} OUTPUT_FORMAT_MAP ;

static void copy_data (SNDFILE *outfile, SNDFILE *infile, int len) ;


static OUTPUT_FORMAT_MAP format_map [] = 
{
	{	"aif",		3,	SF_FORMAT_AIFF	},
	{	"wav", 		0,	SF_FORMAT_WAV	},
	{	"au",		0,	SF_FORMAT_AU	},
	{	"snd",		0,	SF_FORMAT_AU	},
	{	"svx",		0,	SF_FORMAT_SVX	},
	{	"paf",		0,	SF_ENDIAN_BIG | SF_FORMAT_PAF	},
	{	"fap",		0,	SF_ENDIAN_LITTLE | SF_FORMAT_PAF	},
	{	"nist", 	0,	SF_FORMAT_NIST	},
	{	"ircam",	0,	SF_FORMAT_IRCAM	},
	{	"sf",		0, 	SF_FORMAT_IRCAM	},
	{	"voc",		0, 	SF_FORMAT_VOC	},
	{	"w64", 		0, 	SF_FORMAT_W64	},
	{	"raw",		0,	SF_FORMAT_RAW	},
	{	"mat4", 	0,	SF_FORMAT_MAT4	},
	{	"mat5", 	0, 	SF_FORMAT_MAT5 	},
	{	"mat",		0, 	SF_FORMAT_MAT4 	}
} ; /* format_map */

static int	
guess_output_file_type (char *str, int format)
{	char	buffer [16], *cptr ;
	int		k ;
	
	format &= SF_FORMAT_SUBMASK ;

	if (! (cptr = strrchr (str, '.')))
		return 0 ;

	strncpy (buffer, cptr + 1, 15) ;
	buffer [15] = 0 ;
	
	for (k = 0 ; buffer [k] ; k++)
		buffer [k] = tolower ((buffer [k])) ;
		
	for (k = 0 ; k < (int) (sizeof (format_map) / sizeof (format_map [0])) ; k++)
	{	if (format_map [k].len > 0 && 
			strncmp (buffer, format_map [k].ext, format_map [k].len) == 0)
				return format_map [k].format | format ;
		else if (strcmp (buffer, format_map [k].ext) == 0)
				return format_map [k].format | format ;
		} ;

	return	0 ;
} /* guess_output_file_type */


static void	
print_usage (char *progname)
{	SF_FORMAT_INFO	info ;

	int k ;

	printf ("\nUsage : %s [encoding] <input file> <output file>\n", progname) ;
	puts ("\n"
		"    where [encoding] may be one of the following:\n\n"
		"        -pcm16     : force the output to 16 bit pcm\n"
		"        -pcm24     : force the output to 24 bit pcm\n"
		"        -pcm32     : force the output to 32 bit pcm\n"
		"        -float32   : force the output to 32 bit floating point\n"
		"        -ima-adpcm : force the output IMA ADPCM (WAV only)\n"
		) ;
	puts (
		"        -ms-adpcm  : force the output MS ADPCM (WAV only)\n"
		"        -gsm610    : force the GSM6.10 (WAV only)\n"
		"        -dwvw12    : force the output to 12 bit DWVW (AIFF only)\n"
		"        -dwvw16    : force the output to 16 bit DWVW (AIFF only)\n"
		"        -dwvw24    : force the output to 24 bit DWVW (AIFF only)\n"
		) ;

	puts (
		"    The format of the output file is determined by the file extension of the\n"
		"    output file name. The following extensions are currently understood:\n"
		) ;
		
	for (k = 0 ; k < (int) (sizeof (format_map) / sizeof (format_map [0])) ; k++)
	{	info.format = format_map [k].format ;
		sf_command (NULL, SFC_GET_FORMAT_INFO, &info, sizeof (info)) ;
		printf ("        %-10s : %s\n",  format_map [k].ext, info.name) ;
		} ;

	puts ("") ;
} /* print_usage */

int
main (int argc, char *argv[])
{	char 		*progname, *infilename, *outfilename ;
	SNDFILE	 	*infile, *outfile ;
	SF_INFO	 	sfinfo ;
	int			k, outfilemajor, outfileminor = 0 ;

	progname = strrchr (argv [0], '/') ;
	progname = progname ? progname + 1 : argv [0] ;
		
	if (argc < 3 || argc > 5)
	{	print_usage (progname) ;
		return  1 ;
		} ;
		
	infilename = argv [argc-2] ;
	outfilename = argv [argc-1] ;
		
	if (! strcmp (infilename, outfilename))
	{	printf ("Error : Input and output filenames are the same.\n\n") ;
		print_usage (progname) ;
		return  1 ;
		} ;
		
	if (infilename [0] == '-')
	{	printf ("Error : Input filename (%s) looks like an option.\n\n", infilename) ;
		print_usage (progname) ;
		return  1 ;
		} ;
	
	if (outfilename [0] == '-')
	{	printf ("Error : Output filename (%s) looks like an option.\n\n", outfilename) ;
		print_usage (progname) ;
		return  1 ;
		} ;
		
	for (k = 1 ; k < argc - 2 ; k++)
	{	if (! strcmp (argv [k], "-pcm16"))
		{	outfileminor = SF_FORMAT_PCM_16 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-pcm24"))
		{	outfileminor = SF_FORMAT_PCM_24 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-pcm32"))
		{	outfileminor = SF_FORMAT_PCM_32 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-float32"))
		{	outfileminor = SF_FORMAT_FLOAT ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-ima-adpcm"))
		{	outfileminor = SF_FORMAT_IMA_ADPCM ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-ms-adpcm"))
		{	outfileminor = SF_FORMAT_MS_ADPCM ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-gsm610"))
		{	outfileminor = SF_FORMAT_GSM610 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-dwvw12"))
		{	outfileminor = SF_FORMAT_DWVW_12 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-dwvw16"))
		{	outfileminor = SF_FORMAT_DWVW_16 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-dwvw24"))
		{	outfileminor = SF_FORMAT_DWVW_24 ;
			continue ;
			} ;
		} ;

	if (! (infile = sf_open (infilename, SFM_READ, &sfinfo)))
	{	printf ("Not able to open input file %s.\n", infilename) ;
		puts (sf_strerror (NULL)) ;
		return  1 ;
		} ;
	
	if (! (sfinfo.format = guess_output_file_type (outfilename, sfinfo.format)))
	{	printf ("Error : Not able to determine output file type for %s.\n", outfilename) ;
		return 1 ;
		} ;
	
	outfilemajor = sfinfo.format & (SF_FORMAT_TYPEMASK | SF_FORMAT_ENDMASK) ;

	if (outfileminor)
		sfinfo.format = outfilemajor | outfileminor ;
	else
		sfinfo.format = outfilemajor | (sfinfo.format & SF_FORMAT_SUBMASK) ;
		
	if (! sf_format_check (&sfinfo))
	{	printf ("Error : output file format is invalid (0x%08X).\n", sfinfo.format) ;
		return 1 ;
		} ;	

	/* Open the output file. */
	if (! (outfile = sf_open (outfilename, SFM_WRITE, &sfinfo)))
	{	printf ("Not able to open output file %s.\n", outfilename) ;
		return  1 ;
		} ;
		
	copy_data (outfile, infile, BUFFER_LEN / sfinfo.channels) ;
		
	sf_close (infile) ;
	sf_close (outfile) ;
	
	return 0 ;
} /* main */

static void    
copy_data (SNDFILE *outfile, SNDFILE *infile, int len)
{	static double	data [BUFFER_LEN] ;
	long	readcount ;

	readcount = len ;
	while (readcount == len)
	{	readcount = sf_read_double (infile, data, len) ;
		sf_write_double (outfile, data, readcount) ;
		} ;

	return ;
} /* copy_data */

