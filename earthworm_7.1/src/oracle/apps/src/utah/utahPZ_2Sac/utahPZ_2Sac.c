
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define		MAX_PZ	100
#define		MAXLINE	80
#define		MAX_COMMENT	20*MAXLINE

/*
 * Usage: utahPZ_2Sac response_file output_dir
 *              sta.chan.resp.stime
 *
 *  Output:  response file in SAC format with the name
 *              sta.chan.resp
 *
 */
main (int argc, char **argv)
{

	FILE 	*fp;
	char	filename[256];
	int		i, NumPoles, NumZeros, np, nz, status;
	double	Constant;
	double	Poles[MAX_PZ][2];
	double	Zeros[MAX_PZ][2];
	char	line[MAXLINE];
	char	Comment[MAX_COMMENT];
	char	word[21];
	enum states {Key, Pole, Zero};
	enum states state = Key;



	if (argc != 3)
	{
		fprintf (stderr, "Usage: utahPZ_2Sac response_file output_directory\n");
		exit (0);
	}


	/* Open the incoming file */
fprintf (stderr, "Reading file: %s\n", argv[1]);
	if ((fp = fopen (argv[1], "rt")) == NULL)
	{
		fprintf (stderr, "Could not open file: %s\n", argv[1]);
		exit (0);
	}


	/* Initial comment */

	sprintf (Comment, "* This file was generated automatically by utahPZ_2Sac.\n"
					  "* The original response file was %s.\n"
					  "* \n", argv[1]);
	
	np = 0;
	nz = 0;
	status = 0;
	
	while (fgets (line, MAXLINE, fp) != NULL)
	{
		switch (state)
		{
			case Key:  /* Looking for next keyword */
				if (sscanf (line, "%20s", word) == 0)
					continue;
				if ( word[0] == '*' || word[0] == '#') 
				{
					strcat (Comment, line);
					continue;  /* a comment line */
				}
      
				if ( strcmp (word, "CONSTANT") == 0)
				{
					if (sscanf(line, "%*s %lf", &Constant) == 0 )
					{
						fprintf (stderr, "Invalid or missing constant; Abort.\n");
						fclose (fp);
						exit (0);
					}
					status |= 1;  /* Found the constant or gain */
				}
				else if ( strcmp(word, "ZEROS") == 0)
				{
					if (sscanf (line, "%*s %d", &NumZeros) == 0 || NumZeros < 0)
					{
						/* invalid or missing number after ZEROS keyword */
						fprintf (stderr, "Invalid or missing number of zeros; Abort.\n");
						fclose (fp);
						exit (0);
					}
					if (NumZeros > 0)
					{
						for (i = 0; i < NumZeros; i++)
						{
							Zeros[i][0] = 0.0;
							Zeros[i][1] = 0.0;
						}

						state = Zero;  /* There are some zeros; go find them */
						continue;
					}
					status |= 2;  /* Got the number of zeros: none */
				}
				else if ( strcmp (word, "POLES") == 0)
				{
					if (sscanf(line, "%*s %d", &NumPoles) == 0 || NumPoles < 0)
					{
						/* invalid or missing number after POLES keyword */
						fprintf (stderr, "Invalid or missing number of poles; Abort.\n");
						fclose (fp);
						exit (0);
					}
					if (NumPoles > 0)
					{
						for (i = 0; i < NumPoles; i++)
						{
							Poles[i][0] = 0.0;
							Poles[i][1] = 0.0;
						}
          
						state = Pole;  /* There are some poles; go find them */
						continue;
					}
					status |= 4;  /* Got the number of poles: none */
				}
				else
				{
					/* Invalid keyword */
					fprintf (stderr, "Invalid keyword; Abort.\n");
					fclose (fp);
					exit (0);
				}
				break;
			case Zero:
				/* Looking for Zeros */
				if (nz >= NumZeros)
				{
					/* Too many zeros! */
					fprintf (stderr, "Too many zeros; Abort.\n");
					fclose (fp);
					exit (0);
				}
				if (sscanf(line, "%lf %lf", &Zeros[nz][0], &Zeros[nz][1]) != 2)
				{
					/* 
					 * Couldn't read a line of zeros:
					 * Assume that zeros are set to 0.0, and that poles follow.
					 * Read the number of poles, and continue.
					 */
					fprintf (stderr, "Couldn't read a line of zeros; Assume 0.0.\n");

					if (sscanf(line, "%*s %d", &NumPoles) == 0 || NumPoles < 0)
					{
						/* invalid or missing number after POLES keyword */
						fprintf (stderr, "Invalid or missing number of poles; Abort.\n");
						fclose (fp);
						exit (0);
					}
					if (NumPoles > 0)
					{
						for (i = 0; i < NumPoles; i++)
						{
							Poles[i][0] = 0.0;
							Poles[i][1] = 0.0;
						}
          
						state = Pole;  /* There are some poles; go find them */
						continue;
					}
					status |= 4;  /* Got the number of poles: none */
				}
				if (++nz == NumZeros)
				{
					/* Found all the zeros we expected */
					status |= 2;
					state = Key;
					continue;
				}
				break;
			case Pole:
				/* Looking for poles */
				if (np >= NumPoles)
				{
					/* Too many poles! */
					fprintf (stderr, "Too many poles; Abort.\n");
					fclose (fp);
					exit (0);
				}
				if (sscanf(line, "%lf %lf", &Poles[np][0], &Poles[np][1]) != 2)
				{
					/* Couldn't read a line of poles */
					fprintf (stderr, "Couldn't read a line of poles; Abort.\n");
					fclose (fp);
					exit (0);
				}
				if (++np == NumPoles)
				{
					/* Found all the poles we expected */
					status |= 4;
					state = Key;
					continue;
				}
				break;
		} /* end switch */
	} /* end while */

	fclose (fp);


	/* Add comment about the displacement units */
	strcat (Comment, "* Displacement in nanometers\n");




	/* Write out the outgoing file */
	sprintf (filename, "%s/", argv[2]);
	strncat (filename, argv[1], 7);
	strcat (filename, ".uu.resp");

fprintf (stderr, "Writing file: %s\n", filename);

	if ((fp = fopen (filename, "wt")) == NULL)
	{
		fprintf (stderr, "Couldn't open file: %s\n", filename);
		exit (0);
	}

	fprintf (fp, "%s", Comment);
	fprintf (fp, "CONSTANT\t%f\n", (Constant/1000.0));

	fprintf (fp, "ZEROS\t%d\n", NumZeros);
	for (i = 0; i < NumZeros; i++)
		fprintf (fp, "\t%f  %f\n", Zeros[i][0], Zeros[i][1]);

	fprintf (fp, "POLES\t%d\n", NumPoles);
	for (i = 0; i < NumPoles; i++)
		fprintf (fp, "\t%f  %f\n", Poles[i][0], Poles[i][1]);

	fclose (fp);

	return 1;

}



