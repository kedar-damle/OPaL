/// @file alex.c
#include <stdio.h>
#include <stdlib.h>     /* fclose */
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <argp.h>

#include "../include/opal.h"

/// Get build number from compiler
static void argp_print_version(FILE *stream, struct argp_state *state)
{
  fprintf(stream, "Version: %.2f\n", __VERSION_NUM);
}
void (*argp_program_version_hook)(FILE *stream, struct argp_state *state) =
    argp_print_version;

const char *argp_program_bug_address =
    "https://github.com/mckerracher/OPaL/issues";

/// Program documentation
static char doc[] = "alex - OPaL Compiler preprocessor";
static char args_doc[] = "FILE";            ///< Arguments we accept
static struct argp_option options[] =       ///< The options we understand
  {
    { "debug", 'd', 0, 0, "Log debug messages" },
    { "log", 'l', "FILE", 0, "Save log to FILE instead of 'log/oc_log'" },
    { "output", 'o', "FILE", 0, "Output to FILE instead of standard ouput" },
    { 0 }
  };

struct arguments
{
  char *args[1]; /* source file */
  char *logfile;
  char *destfile;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
   * know is a pointer to our arguments structure.
   */
  struct arguments *arguments = state->input;

  switch (key)
    {

    case 'd':
      LOG_LEVEL = DEBUG;
      break;

    case 'l':
      arguments->logfile = arg;
      break;

    case 'o':
      arguments->destfile = arg;
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num >= 1)      // Too many arguments
          argp_usage (state);
      arguments->args[state->arg_num] = arg;
      break;

    case ARGP_KEY_END:
      if (state->arg_num < 1)       // Not enough arguments
          argp_usage (state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return EXIT_SUCCESS;
}

static struct argp argp =
  { options, parse_opt, args_doc, doc };

/**
 * @brief       Main function for pre-processor alex
 * @details     Calls the function remove_comments() and proc_includes()
 * to process the user input file. Calls the build_symbol_table() to build
 * symbol table and and writes to the destination.
 *
 * @param[in]   argc    Number of command line arguments
 * @param[in]   argv    Vector of individual command line argument strings
 * @return      The error return code of the function.
 *
 * @retval      EXIT_SUCCESS    On success
 * @retval      EXIT_FAILURE    On error
 * @retval      errno           On system call failure
 */

int
main (int argc, char **argv)
{

  /// Create structure to process command line arguments
  struct arguments arguments =
    {
      .destfile = NULL,
      .logfile = NULL
    };

  /// Parse arguments
  argp_parse (&argp, argc, argv, 0, 0, &arguments);

  /// Populate variables for source, destination, log file
  source_fn = strdup (arguments.args[0]);
  dest_fn = arguments.destfile ? strdup (arguments.destfile) : NULL;
  log_fn =
      arguments.logfile ? strdup (arguments.logfile) : strdup ("log/oc_log");

  /// Open log file in append mode, else exit program
  sprintf (perror_msg, "log_fd = fopen(%s, 'a')", log_fn);
  log_fd = fopen (log_fn, "a");
  if (log_fd == NULL)
    {
      _FAIL;
      perror (perror_msg);
      return (opal_exit (EXIT_FAILURE));
    }

  logger (DEBUG, "Log: %s", log_fn);
  logger (DEBUG, "source_fn: '%s'", source_fn);

  /// If source file does not exist, print error and exit
  sprintf (perror_msg, "access('%s', F_OK)", source_fn);
  logger (DEBUG, perror_msg);
  if (access (source_fn, F_OK) == EXIT_SUCCESS)
    _PASS;
  else
    {
      _FAIL;
      perror (perror_msg);
      return (errno);
    }

  /// If source file can not be read, print error and exit
  sprintf (perror_msg, "access('%s', R_OK)", source_fn);
  logger (DEBUG, perror_msg);
  if (access (source_fn, R_OK) == EXIT_SUCCESS)
    _PASS;
  else
    {
      _FAIL;
      perror (perror_msg);
      return (errno);
    }

  /// If destination is file
  if (dest_fn)
    {
      logger (DEBUG, "dest_fn: %s", dest_fn);

      /// Check if destination file exists
      sprintf (perror_msg, "access('%s', F_OK)", dest_fn);
      logger (DEBUG, perror_msg);
      if (access (dest_fn, F_OK) == EXIT_SUCCESS)
        {
          _PASS;

          /// If destination file can't be written, print error and exit
          sprintf (perror_msg, "access('%s', W_OK)", dest_fn);
          logger (DEBUG, perror_msg);
          if (access (dest_fn, W_OK) == EXIT_SUCCESS)
            _PASS;
          else
            {
              _FAIL;
              perror (perror_msg);
              return (errno);
            }
        }

      /// Open destination file in 'wb' mode
      sprintf (perror_msg, "dest_fd = fopen('%s', 'wb')", dest_fn);
      logger (DEBUG, perror_msg);
      dest_fd = fopen (dest_fn, "wb");
      if (dest_fd)
        _PASS;
      else
        {
          _FAIL;
          perror (perror_msg);
          return (errno);
        }
    }
  /// Else, destination is STDOUT
  else
    {
      logger (DEBUG, "Destination: STDOUT");
      dest_fd = stdout;
    }

  /// Open source file in read-only mode
  sprintf (perror_msg, "source_fd = fopen('%s', 'r')", source_fn);
  logger (DEBUG, perror_msg);
  source_fd = fopen (source_fn, "r");
  if (source_fd != NULL)
    _PASS;
  else
    {
      _FAIL;
      perror (perror_msg);
      return (errno);
    }

  /// Call MARC functions to pre-process source file
  banner ("MARC start.");

  /// Create and open temp destination file for remove_comments()
  char *rc_tmp = "tmp/marc_rc.tmp";
  logger (DEBUG, "rc_tmp: '%s'", rc_tmp);

  /// If temp file can not be written, print error and exit
  sprintf (perror_msg, "rc_fd = fopen('%s', 'wb')", rc_tmp);
  logger (DEBUG, perror_msg);
  FILE *rc_fd = fopen (rc_tmp, "wb");
  if (rc_fd != NULL)
    _PASS;
  else
    {
      _FAIL;
      perror (perror_msg);
      return (errno);
    }

  /// Remove comments from source with rem_comments(), write to rc_tmp
  retVal = rem_comments (source_fd, rc_fd);
  if (retVal != EXIT_SUCCESS)
      return (opal_exit (retVal));

  /// Close source file descriptor source_fd if not NULL
  sprintf (perror_msg, "fclose(source_fd)");
  logger (DEBUG, perror_msg);
  if (source_fd)
    {
      if (fclose (source_fd) == EXIT_SUCCESS)
        _PASS;
      else
        {
          _FAIL;
          perror (perror_msg);
          return (errno);
        }
    }

  /// Close rem_comments() temp file descriptor rc_fd if not NULL
  sprintf (perror_msg, "fclose(rc_fd)");
  logger (DEBUG, perror_msg);
  if (rc_fd)
    {
      if (fclose (rc_fd) == EXIT_SUCCESS)
        _PASS;
      else
        {
          _FAIL;
          perror (perror_msg);
          return (errno);
        }
    }

  /// Open rem_comments() temp file in read mode, else print error and exit
  sprintf (perror_msg, "rc_fd = fopen('%s', 'r')", rc_tmp);
  logger (DEBUG, perror_msg);
  rc_fd = fopen (rc_tmp, "r");
  if (rc_fd != NULL)
    _PASS;
  else
    {
      _FAIL;
      perror (perror_msg);
      return (errno);
    }

  /// Create and open temp destination file for proc_includes()
  char *pi_tmp = "tmp/marc_pi.tmp";
  logger (DEBUG, "pi_tmp: '%s'", pi_tmp);

  /// If temp file can not be written, print error and exit
  sprintf (perror_msg, "pi_fd = fopen('%s', 'wb')", pi_tmp);
  logger (DEBUG, perror_msg);
  FILE *pi_fd = fopen (pi_tmp, "wb");
  if (pi_fd != NULL)
    _PASS;
  else
    {
      _FAIL;
      perror (perror_msg);
      return (errno);
    }

  /// Process #include directives from source with proc_includes()
  retVal = proc_includes (rc_fd, pi_fd);
  if (retVal != EXIT_SUCCESS)
    {
      return (opal_exit (retVal));
    }

  /// Close rem_comments temp file descriptor if not NULL
  if (rc_fd)
    {
      sprintf (perror_msg, "fclose(rc_fd)");
      logger (DEBUG, perror_msg);

      if (fclose (rc_fd) == EXIT_SUCCESS)
        _PASS;
      else
        {
          _FAIL;
          perror (perror_msg);
          return (errno);
        }
    }

  /// Close proc_includes() temp file descriptor if not NULL
  if (pi_fd)
    {
      sprintf (perror_msg, "fclose(pi_fd)");
      logger (DEBUG, perror_msg);

      if (fclose (pi_fd) == EXIT_SUCCESS)
        _PASS;
      else
        {
          _FAIL;
          perror (perror_msg);
          return (errno);
        }
    }

  /// Open proc_includes() temp file in read mode, else print error and exit
  sprintf (perror_msg, "pi_fd = fopen('%s', 'r')", pi_tmp);
  logger (DEBUG, perror_msg);
  pi_fd = fopen (pi_tmp, "r");
  if (pi_fd != NULL)
    _PASS;
  else
    {
      _FAIL;
      perror (perror_msg);
      return (errno);
    }

  /// Open rem_comments() temp file in write mode, else print error and exit
  sprintf (perror_msg, "rc_fd = fopen('%s', 'wb')", rc_tmp);
  logger (DEBUG, perror_msg);
  rc_fd = fopen (rc_tmp, "wb");
  if (rc_fd != NULL)
    _PASS;
  else
    {
      _FAIL;
      perror (perror_msg);
      return (errno);
    }

  /// Remove comments from includs files with rem_comments(), write to rc_tmp
  retVal = rem_comments (pi_fd, rc_fd);
  if (retVal != EXIT_SUCCESS)
    {
      return (opal_exit (retVal));
    }

  /// Close proc_includes() temp file descriptor, else print error and exit
  sprintf (perror_msg, "fclose(pi_fd)");
  logger (DEBUG, perror_msg);
  if (fclose (pi_fd) == EXIT_SUCCESS)
    _PASS;
  else
    {
      _FAIL;
      perror (perror_msg);
      return (errno);
    }

  /// Close rem_comments() temp file descriptor, else print error and exit
  sprintf (perror_msg, "fclose(rc_fd)");
  logger (DEBUG, perror_msg);
  if (fclose (rc_fd) == EXIT_SUCCESS)
    _PASS;
  else
    {
      _FAIL;
      perror (perror_msg);
      return (errno);
    }

  /// Start lexical analyzer code
  banner ("ALEX start.");

  /// Open rem_comments() temp file in read mode, else print error and exit
  sprintf (perror_msg, "source_fd = fopen('%s', 'r')", rc_tmp);
  logger (DEBUG, perror_msg);
  source_fd = fopen (rc_tmp, "r");
  if (source_fd != NULL)
    _PASS;
  else
    {
      _FAIL;
      perror (perror_msg);
      return (errno);
    }

  /// Create symbol table linked list
  logger (DEBUG, "Create symbol_table linked list node.");
  lexeme_s *symbol_table = (lexeme_s*) calloc (1, sizeof(lexeme_s));

  int symbol_ct = 0;                ///< Numbber of lexemes identified

  /// Build symbol table using rem_comments() temp file as source
  retVal = build_symbol_table(symbol_table, &symbol_ct);
  if (retVal != EXIT_SUCCESS)
    {
      return (opal_exit (retVal));
    }

  logger (DEBUG, "assert(symbol_ct > 0)");
  assert(symbol_ct > 0); _PASS;

  /// Print symbol table with print_symbol_table() to destination
  retVal = print_symbol_table (symbol_table, dest_fd);
  if (retVal != EXIT_SUCCESS)
    {
      return (opal_exit (retVal));
    }

  /// Free memory used by symbol_table
  // TODO: Walk linked list and free every node when we have it
  free(symbol_table);
  symbol_table = NULL;

  /// Mark local pointers to NULL
  if (source_fn)
  {
      free(source_fn);
      source_fn = NULL;
  }

  if (dest_fn)
  {
      free(dest_fn);
      dest_fn = NULL;
  }

  if (log_fn)
    {
      free(log_fn);
      log_fn = NULL;
    }

  /// source_fd and dest_fd closed by opal_exit()
  return (opal_exit (EXIT_SUCCESS));
}
