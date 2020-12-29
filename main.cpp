#include <sys/ptrace.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief dump_memory_region Function to dump a memory region from the /proc/pid/mem file.
 * @param pMemFile Open and traced /proc/pid/mem file.
 * @param start_address Addrese to start dumping from.
 * @param length Number of bytes to dump.
 */
void dump_memory_region(FILE* pMemFile, const unsigned long start_address, const unsigned long length) {
    // Define block allocate buffer
    unsigned long pageLength = 1872;
    unsigned char page[pageLength];
    // Shift mem file to start addresse
    fseeko(pMemFile, start_address, SEEK_SET);

    // Dump memory in block
    for (unsigned long current_address=start_address; current_address < start_address + length; current_address += pageLength) {
        // Read block and dump it.
        fread(&page, 1, pageLength, pMemFile);
        fwrite(&page, 1, pageLength, stdout);
    }
}

/**
 * @brief print_help Dumps a usage message to the terminal and exits the application.
 */
void print_help_and_exit() {
    printf("Usage: reDumper -p <process id> -b <addresse to dump> -s <bytes to skip> -c <bytes to dump>\n"
           "\tPrints c bytes at the starting addresse b after skipping s bytes from /proc/p/mem file.\n"
           "\tThe dumper assumes that the region to dump is in one on memory zone as defined in the /proc/p/map file.\n"
           "\tThe region to will be parsed in block of size b, make sure that c can be split in block of size b.");
    exit(EXIT_SUCCESS);
}

/**
 * @brief print_version Dumps the current version of the command to the terminal and exits the application.
 */
void print_version_and_exit(){
    printf("reDump Version 1.0\nWritten by Simon Schäfer\n");
    exit(EXIT_SUCCESS);
}

#define INVALID_OPTION 1
#define INVALID_OPTION_INPUT 2
#define READING_ERROR 3
#define FILE_NOT_FOUND 4
#define UNABLE_TO_ATTACHE 5
#define SKIPPED_TO_MANY_BYTES 6


/**
 * @brief error Will print an error to the terminal and stop the programm.
 * @param intern_error_code[in] Code to determine the type of error.
 * @param message[in] Parameter to add details to the error message.
 */
void print_error_and_exit(const unsigned short intern_error_code, const char* message){
    switch(intern_error_code){
    case(INVALID_OPTION):
        printf("reDumper: Invalid option %s\nTry 'reDumper --help' for more information.\n", message);
        break;
    case(INVALID_OPTION_INPUT):
        printf("reDumper: Invalid option value: %s\nNumber must be positiv. For zero, drop the option entirely.\n", message);
        break;
    case(READING_ERROR):
        printf("reDumper: Unknown error while parsing the memory file '%s'. Make sure you have admin access.\n", message);
        break;
    case(FILE_NOT_FOUND):
         printf("reDumper: Cannot open '%s' for reading: No such file or directory\nCheck the process id. \n", message);
         break;
    case(UNABLE_TO_ATTACHE):
        printf("reDumper: Unable to attache to process '%s'. Make sure you have admin access and check the process id.\n", message);
        break;
    case(SKIPPED_TO_MANY_BYTES):
        printf("reDumper: To many bytes skipped '%s'. The number of skipped bytes exceeds the allocated memory region.\n", message);
        break;
    default:
        printf("reDumper: Unknown error.\n");
    }
    exit(EXIT_FAILURE);
}

/**
 * @brief process_memory_dump_and_exit_on_failure Handles the setup to get access to the memory if the process.
 * @param pid Process to trace and dump memory of.
 * @param addresse Addresse to start the dump.
 * @param bytes_to_skip Bytes to skip before dumping.
 * @param bytes_to_dump Bytes to dump
 */
void process_memory_dump_and_exit_on_failure(const int pid, const unsigned long addresse, const unsigned long bytes_to_skip, const unsigned long bytes_to_dump){
    // Attache to process to grant access to memory
    long ptraceResult = ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    if (ptraceResult < 0){
        char process_id_string[32];
        sprintf(process_id_string, "%i", pid);
        print_error_and_exit(UNABLE_TO_ATTACHE, process_id_string);
    }
    // Wait for trace to finish
    wait(NULL);

    // Get map file. This file contains the mapping of the application memory.
    char mapsFilename[1024];
    sprintf(mapsFilename, "/proc/%i/maps", pid);
    FILE* pMapsFile = fopen(mapsFilename, "r");

    // Get mem file. This file contains the content of the application memory.
    char memFilename[1024];
    sprintf(memFilename, "/proc/%i/mem", pid);
    FILE* pMemFile = fopen(memFilename, "r");

    // Parse the start and end addresses from the mapping file.
    char line[256];
    while (fgets(line, 256, pMapsFile) != NULL)
    {
        // Get addresses
        unsigned long start_address;
        unsigned long end_address;
        sscanf(line, "%08lx-%08lx\n", &start_address, &end_address);

        // Check if address is the frame buffer. If not skip to next memory region.
        if(start_address != addresse) continue;

        // Frame buffer found. Shift the pointer to skip bytes.
        start_address = start_address+bytes_to_skip;
        if (start_address>=end_address){
            char skipped_bytes_string[32];
            sprintf(skipped_bytes_string, "%lu", bytes_to_skip);
            print_error_and_exit(SKIPPED_TO_MANY_BYTES, skipped_bytes_string);
        }
        // Actually dump memory
        dump_memory_region(pMemFile, start_address, bytes_to_dump);

    }
    // Close files
    fclose(pMapsFile);
    fclose(pMemFile);

    // Detach from process.
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
}

/**
 * @brief process_parameter Will parse the input parameter from the commandline and trigger advanced field like --help.
 * @param argc[in] Number of input parameters.
 * @param argv[in] Input parameters.
 * @param bytes_to_print[out] If option if given and valid, the number of bytes to dump will be written to this addresse.
 * @param bytes_to_skip[out] If option if given and valid, the number of bytes to skip will be written to this addresse.
 * @param memory_adress[out] If option if given and valid, target memory address will be written to this addresse.
 * @param process_id[out] If option if given and valid, the process id will be written to this addresse.
 */
void process_parameter_and_exit_on_failure(const int argc, char ** const argv, unsigned long *bytes_to_print, unsigned long *bytes_to_skip, unsigned long *memory_adress, unsigned int *process_id) {
    char buffer_option[32];
    const char *pointer_long_option = buffer_option + 2;

    for(int i = 1; i < argc; i++){
        strcpy(buffer_option, argv[i]);
        if(buffer_option[0]=='-'){
            switch(buffer_option[1]){
                // Short Options
            case 'p':
                if(atoi(argv[++i]) == 0) print_error_and_exit(INVALID_OPTION_INPUT, argv[i]);
                *process_id = atoi(argv[i]);
                break;
            case 'c':
                if(atoi(argv[++i]) == 0) print_error_and_exit(INVALID_OPTION_INPUT, argv[i]);
                *bytes_to_print = atoi(argv[i]);
                break;
            case 'b':
                if(atoi(argv[++i]) == 0)print_error_and_exit(INVALID_OPTION_INPUT, argv[i]);
                *memory_adress = atoi(argv[i]);
                break;
            case 's':
                if(atoi(argv[++i]) == 0)print_error_and_exit(INVALID_OPTION_INPUT, argv[i]);
                *bytes_to_skip = atoi(argv[i]);
                break;
            case '-':
                // Long options
                if(strcmp(pointer_long_option, "version") == 0){
                    print_version_and_exit();
                }else if(strcmp(pointer_long_option, "help") == 0){
                    print_help_and_exit();
                }else{
                    print_error_and_exit(INVALID_OPTION, argv[i]);
                }
                break;
            default:
                print_error_and_exit(INVALID_OPTION, argv[i]);
                break;
            }
        }
        // Just ignore everything that is not an option.
    }
}

/**
 * @brief main Main function call. Will triggere the main process after parsing the input parameter.
 * @param argc Number of input parameter.
 * @param argv Input parameter content.
 * @return Zero if anything went fine and -1 otherwise.
 */
int main(int argc, char *argv[]){
    // Buffers the needed to be piped from the terminal to the process
    unsigned long bytes_to_print = 0, bytes_to_skip = 0, addresse = 0;
    unsigned int pid = 0;

    // Call main process functions.
    process_parameter_and_exit_on_failure(argc, argv, &bytes_to_print, &bytes_to_skip, &addresse, &pid);

    process_memory_dump_and_exit_on_failure(pid, addresse, bytes_to_skip, bytes_to_print);

    exit(EXIT_SUCCESS);
}
