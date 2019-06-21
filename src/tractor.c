#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tractor.h"

static FILE *log_file = NULL;
static char *bootstrap_string = NULL;
static char *script_file = NULL, *working_dir = NULL, *output_level = NULL, *config_file = NULL, *script_args = NULL;
static int parallelisation_factor = 1, profile_performance = 0, use_colour = 1;

void set_colour (FILE *stream, const int code)
{
    if (use_colour == 1)
        fprintf(stream, "\x1b[%dm", code);
}

char * allocate_and_copy_string (const char *from)
{
    char *to;
    
    if (from != NULL)
    {
        to = (char *) malloc(strlen(from) + 1);
        strcpy(to, from);
    }
    
    return to;
}

void parse_arguments (int argc, const char **argv)
{
    int i, to_drop, current_arg;
    size_t script_args_len = 0, script_args_index = 0;
    
    // First pass: identify and remove flagged options
    current_arg = 1;
    while (current_arg < argc)
    {
        to_drop = 0;
        
        if (strncmp(argv[current_arg],"-",1) != 0)
        {
            if (script_file == NULL)
            {
                script_file = allocate_and_copy_string(argv[current_arg]);
                argv[current_arg] = NULL;
                break;
            }
        }
        else if (strcmp(argv[current_arg], "-w") == 0)
        {
            working_dir = allocate_and_copy_string(argv[current_arg+1]);
            to_drop = 2;
        }
        else if (strcmp(argv[current_arg], "-l") == 0)
        {
            output_level = allocate_and_copy_string(argv[current_arg+1]);
            to_drop = 2;
        }
        else if (strcmp(argv[current_arg], "-c") == 0)
        {
            config_file = allocate_and_copy_string(argv[current_arg+1]);
            to_drop = 2;
        }
        else if (strcmp(argv[current_arg], "-g") == 0)
        {
            log_file = fopen(argv[current_arg+1], "w");
            to_drop = 2;
        }
        else if (strcmp(argv[current_arg], "-p") == 0)
        {
            if (argv[current_arg+1] != NULL)
            {
                parallelisation_factor = atoi(argv[current_arg+1]);
                if (parallelisation_factor < 1 || parallelisation_factor > 99)
                {
                    set_colour(stderr, 33);
                    fprintf(stderr, "Parallelisation factor must be between 1 and 99 - ignoring value of %d\n", parallelisation_factor);
                    set_colour(stderr, 0);
                    parallelisation_factor = 1;
                }
                to_drop = 2;
            }
        }
        else if (strcmp(argv[current_arg], "-f") == 0)
        {
            profile_performance = 1;
            to_drop = 1;
        }
        else if (strcmp(argv[current_arg], "-m") == 0)
        {
            use_colour = 0;
            to_drop = 1;
        }
        else if (strncmp(argv[current_arg],"-",1) == 0)
        {
            set_colour(stderr, 33);
            fprintf(stderr, "Unrecognised option: %s\n", argv[current_arg]);
            set_colour(stderr, 0);
            to_drop = 1;
        }
        
        for (i=0; i<to_drop; i++)
            argv[current_arg+i] = NULL;
        current_arg += to_drop;
    }
    
    // Second pass: sum up the lengths of unflagged arguments
    for (current_arg = 1; current_arg < argc; current_arg++)
    {
        if (argv[current_arg] != NULL)
            script_args_len += strlen(argv[current_arg]) + 1;   // +1 for the space
    }
    
    if (script_args_len > 0)
    {
        script_args = (char *) malloc(script_args_len + 1);
    
        // Third pass: copy unflagged arguments into "script_args"
        for (current_arg = 1; current_arg < argc; current_arg++)
        {
            if (argv[current_arg] != NULL)
            {
                strcpy(script_args + script_args_index, argv[current_arg]);
                script_args_index += strlen(argv[current_arg]);
                strcpy(script_args + script_args_index, " ");
                script_args_index++;
            }
        }
        
        script_args[script_args_index-1] = '\0';
    }
}

int main (int argc, char **argv)
{
    parse_arguments(argc, (const char **) argv);
    
    if (script_file == NULL)
    {
        set_colour(stderr, 31);
        fputs("Error: Script file must be specified\n", stderr);
        set_colour(stderr, 0);
        tidy_up();
        return 1;
    }
    
    char *R_args[3] = { "tractor", "--quiet", "--vanilla" };
    Rf_initialize_R(3, R_args);
    
    // Store the addresses of the default callbacks
    // These will be called by our functions after they have had a chance to intervene
    ptr_R_ReadConsole_default = ptr_R_ReadConsole;
    ptr_R_CleanUp_default = ptr_R_CleanUp;
    
    // Set up our callbacks
    ptr_R_ReadConsole = &read_console;
    ptr_R_WriteConsole = NULL;
    ptr_R_WriteConsoleEx = &write_console;
    R_Outputfile = NULL;
    R_Consolefile = NULL;
    ptr_R_CleanUp = &tidy_up_all;
    R_running_as_main_program = 1;
    
    Rf_mainloop();
    
    // For the compiler only: Rf_mainloop() does not return
    return 0;
}

void build_bootstrap_string ()
{
    // R prototype is: function (scriptFile, workingDirectory = getwd(), reportFile = NULL, outputLevel = OL$Warning,
    //           configFiles = NULL, configText = NULL, parallelisationFactor = 1, standalone = TRUE, debug = FALSE)
    
    size_t len, offset;
    const char *fixed_part = "library(utils); library(tractor.utils); bootstrapExperiment(";
    
    // Work out the length of string required
    len = strlen(fixed_part) +      // Length of fixed part of the string
          3 +                       // Closing bracket, newline and terminating characters
          strlen(script_file) + 2;  // Length of script file name and surrounding quotes
    
    // Two added to each length for quotes (where needed)
    if (working_dir != NULL)
        len += strlen(working_dir) + strlen(", workingDirectory=") + 2;
    if (output_level != NULL)
        len += strlen(output_level) + strlen(", outputLevel=OL$");
    if (config_file != NULL)
        len += strlen(config_file) + strlen(", configFiles=") + 2;
    if (script_args != NULL)
        len += strlen(script_args) + strlen(", configText=") + 2;
    if (parallelisation_factor > 1)
        len += ((size_t) parallelisation_factor > 9 ? 2 : 1) + strlen(", parallelisationFactor=");
    if (profile_performance == 1)
        len += strlen(", profile=TRUE");
    
    // Allocate space for the string
    bootstrap_string = (char *) malloc(len);
    
    // Write into the final string
    offset = (size_t) sprintf(bootstrap_string, "%s'%s'", fixed_part, script_file);
    
    if (working_dir != NULL)
        offset += sprintf(bootstrap_string + offset, ", workingDirectory='%s'", working_dir);
    if (output_level != NULL)
        offset += sprintf(bootstrap_string + offset, ", outputLevel=OL$%s", output_level);
    if (config_file != NULL)
        offset += sprintf(bootstrap_string + offset, ", configFiles='%s'", config_file);
    if (script_args != NULL)
        offset += sprintf(bootstrap_string + offset, ", configText='%s'", script_args);
    if (parallelisation_factor > 1)
        offset += sprintf(bootstrap_string + offset, ", parallelisationFactor=%d", parallelisation_factor);
    if (profile_performance == 1)
        offset += sprintf(bootstrap_string + offset, ", profile=TRUE");
    
    sprintf(bootstrap_string + offset, ")\n");
}

int read_console (const char *prompt, unsigned char *buffer, int buffer_len, int add_to_history)
{
    // Preserved across calls to this function
    static size_t remaining_len = -1;
    static size_t current_offset = 0;
    
    int return_value = 1;
    
    // First time: build bootstrap string
    if (remaining_len == -1)
    {
        build_bootstrap_string();
        remaining_len = strlen(bootstrap_string);
        
        // Print bootstrap string to stdout if running in debug mode
        if (output_level != NULL && strcmp(output_level,"Debug") == 0)
        {
            set_colour(stdout, 32);
            fputs(bootstrap_string, stdout);
            set_colour(stdout, 0);
            fflush(stdout);
        }
    }
    
    // First and subsequent times until whole string is transferred: copy (part of) string to buffer
    if (remaining_len != 0)
    {
        if (remaining_len < buffer_len)
        {
            strcpy((char *) buffer, bootstrap_string + current_offset);
            remaining_len = 0;
        }
        else
        {
            strncpy((char *) buffer, bootstrap_string + current_offset, buffer_len);
            remaining_len -= buffer_len;
            current_offset += buffer_len;
        }
    }
    else
    {
        // Once bootstrap string is written, revert to usual prompt
        return_value = (*ptr_R_ReadConsole_default)(prompt, buffer, buffer_len, add_to_history);
        
        if (log_file != NULL)
        {
            fputs(prompt, log_file);
            fputs((const char *) buffer, log_file);
        }
    }
    
    return return_value;
}

int is_error_string (const char *string)
{
    size_t loc, len;
    int index;
    char current_char;
    const char *error_string = "error";
    
    len = strlen(string);
    index = 0;
    for (loc=0; loc<=len; loc++)
    {
        if (index == 5)
            return 1;
        
        current_char = tolower(string[loc]);
        if (current_char == error_string[index])
            index++;
        else if (index > 0)
            return 0;
        else if (current_char != ' ' && current_char != '*')
            return 0;
    }
    
    return 0;
}

void write_console (const char *buffer, int buffer_len, int output_type)
{
    size_t len;
    
    if (output_type == 0)
    {
        fputs(buffer, stdout);
        fflush(stdout);
    }
    else
    {
        set_colour(stderr, is_error_string(buffer) ? 31 : 33);
        
        len = strlen(buffer);
        if (buffer[len-1] == '\n')
            fprintf(stderr, "%.*s\n", (int) len-1, buffer);
        else
            fprintf(stderr, "%s", buffer);
        
        set_colour(stderr, 0);
        
        fflush(stderr);
    }
    
    // Also write to the log file, if one is specified
    if (log_file != NULL)
        fputs(buffer, log_file);
}

void tidy_up ()
{
    if (log_file != NULL)
        fclose(log_file);
    if (bootstrap_string != NULL)
        free(bootstrap_string);
    if (script_file != NULL)
        free(script_file);
    if (working_dir != NULL)
        free(working_dir);
    if (output_level != NULL)
        free(output_level);
    if (config_file != NULL)
        free(config_file);
    if (script_args != NULL)
        free(script_args);
}

void tidy_up_all (SA_TYPE save_action, int status, int run_last)
{
    tidy_up();
    (*ptr_R_CleanUp_default)(save_action, status, run_last);
}
