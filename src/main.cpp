/* 
 * File:   main.cpp
 * Author: zapreevis
 *
 * Created on April 18, 2015, 11:35 AM
 */

#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream, std::stringbuf
#include <fstream>      // std::ifstream

#include "Exceptions.hpp"
#include "StatisticsMonitor.hpp"
#include "BasicLogger.hpp"
#include "ITrie.hpp"
#include "HashMapTrie.hpp"
#include "TrieBuilder.hpp"

using namespace std;

#define PATH_SEPARATION_SYMBOLS "/\\"
#define TOKEN_DELIMITER_CHAR ' '
#define N_GRAM_PARAM 5u
#define EXPECTED_NUMBER_OF_ARGUMENTS 3
#define EXPECTED_USER_NUMBER_OF_ARGUMENTS (EXPECTED_NUMBER_OF_ARGUMENTS - 1)
#define BYTES_ONE_MB 1024

/**
 * This structure is needed to store the application parameters
 */
typedef struct {
    //The train file name
    string trainFileName;
    //The test file name
    string testFileName;
} TAppParams;

/**
 * This functions does nothing more but printing the program header information
 */
static void printInfo() {
    BasicLogger::printInfo(" ------------------------------------------------------------------ ");
    BasicLogger::printInfo("|                    Automated Translation Tires                   |");
    BasicLogger::printInfo("|                     Test software version 1.0                    |");
    BasicLogger::printInfo("|               Copyright (C) Fontys Hogescholen, 2015.            |");
    BasicLogger::printInfo("|                            Authors:                              |");
    BasicLogger::printInfo("|                  Ivan S Zapreev (since 2015),                    |");
    BasicLogger::printInfo("|        This software is distributed under GPL 2.0 license        |");
    BasicLogger::printInfo("|          (GPL stands for GNU General Public License)             |");
    BasicLogger::printInfo("|          The product comes with ABSOLUTELY NO WARRANTY.          |");
    BasicLogger::printInfo("| This is a free software, and you are welcome to redistribute it. |");
    BasicLogger::printInfo(" ------------------------------------------------------------------ ");
}

/**
 * This function prints the usage information for the software
 * @param name the absolute name of the application 
 */
static void printUsage(const string name) {
    //Since we do not want the absolute file name, we need to chop off the directory prefix
    const unsigned int lastSlashBeforeFileName = name.find_last_of(PATH_SEPARATION_SYMBOLS);
    const string shortName = name.substr(lastSlashBeforeFileName + 1);

    BasicLogger::printUsage("Running: ");
    BasicLogger::printUsage("  %s <train_file> <test_file>", shortName.c_str());
    BasicLogger::printUsage("     <train_file> - a text file containing the training text corpus.");
    BasicLogger::printUsage("                    This corpus should bealready tokenized, i.e.,");
    BasicLogger::printUsage("                    all words are already separated by white spaces,");
    BasicLogger::printUsage("                    including punctuation marks. Also, each line in ");
    BasicLogger::printUsage("                    this, file corresponds to one sentence.");
    BasicLogger::printUsage("     <test_file>  - a text file containing test data.");
    BasicLogger::printUsage("                    The test file consists of a number of 5-grams,");
    BasicLogger::printUsage("                    where each line in the file consists of one 5-gram.");

    BasicLogger::printUsage("Output: ");
    BasicLogger::printUsage("    The programm reads in the test file and say for each 5-gram of the");
    BasicLogger::printUsage("    form word 1 word 2 word 3 word 4 word 5 what the frequencies");
    BasicLogger::printUsage("    for all n-grams (1 <= n <= 5) ending in word 5, i.e.:");
    BasicLogger::printUsage("    frequency(word1 word2 word 3 word 4 word 5)=?");
    BasicLogger::printUsage("    frequency(word2 word 3 word 4 word 5)=?");
    BasicLogger::printUsage("    frequency(word3 word 4 word 5)=?");
    BasicLogger::printUsage("    frequency(word4 word 5)=?");
    BasicLogger::printUsage("    frequency(word5)=?");
}

/**
 * This function tries to extract the 
 * @param argc the number of program arguments
 * @param argv the array of program arguments
 * @param params the structure that will be filled in with the parsed program arguments
 */
static void extractArguments(const int argc, char const * const * const argv, TAppParams & params) {
    if (argc != EXPECTED_NUMBER_OF_ARGUMENTS) {
        stringstream msg;
        msg << "Incorrect number of arguments, expected " << EXPECTED_USER_NUMBER_OF_ARGUMENTS << ", got " << (argc - 1);
        throw Exception(msg.str());
    } else {
        params.trainFileName = argv[1];
        params.testFileName = argv[2];
    }
}

/**
 * THis method creates a string for the "file exists" information
 * @param fname the file name
 * @param fstr the file stream
 * @return the resulting string to be print
 */
static const string getFileExistsString(string const & fname, ifstream const & fstr) {
    string result = ((bool) fstr ? "is present" : "is missing");
    return fname + " (" + result + ")";
}

/**
 * This is a helper function for creating the memory statistic strings
 * @param vmsize Virtual memory size in Kb
 * @param vmpeak Peak virtual memory size in Kb
 * @param vmrss Resident set size in Kb
 * @param vmhwm Peak resident set size in Kb
 * @return the resulting string reference to the text to be printed
 */
static string getMemoryUsageString(unsigned int const & vmsize,
        unsigned int const & vmpeak,
        unsigned int const & vmrss,
        unsigned int const & vmhwm) {
    stringstream msg;
    msg << "vmsize=" << vmsize << " Kb, vmpeak=" <<
            vmpeak << " Kb, vmrss=" << vmrss <<
            " Kb, vmhwm=" << vmhwm << " Kb";



    return msg.str();
}

/**
 * This function is meant to give the memory statistics information delta
 * @param action the monitored action
 * @param msStart the start memory usage statistics
 * @param msEnd the end memory usage statistics
 */
static void reportMemotyUsage(const char* action, TMemotyUsage msStart, TMemotyUsage msEnd) {

    BasicLogger::printInfo("Action: \'%s\' memory increase:", action);
    BasicLogger::printDebug("memory before: vmsize=%d Kb, vmpeak=%d Kb, vmrss=%d Kb, vmhwm=%d Kb",
            msStart.vmsize, msStart.vmpeak, msStart.vmrss, msStart.vmhwm);
    BasicLogger::printDebug("memory after: vmsize=%d Kb, vmpeak=%d Kb, vmrss=%d Kb, vmhwm=%d Kb",
            msEnd.vmsize, msEnd.vmpeak, msEnd.vmrss, msEnd.vmhwm);
    BasicLogger::printInfo("memory delta: vmsize=%lf Mb, vmpeak=%lf Mb, vmrss=%lf Mb, vmhwm=%lf Mb",
            double(msEnd.vmsize - msStart.vmsize)/BYTES_ONE_MB, double(msEnd.vmpeak - msStart.vmpeak)/BYTES_ONE_MB,
            double(msEnd.vmrss - msStart.vmrss)/BYTES_ONE_MB, double(msEnd.vmhwm - msStart.vmhwm)/BYTES_ONE_MB);
}

/**
 * THis method is used to read from the corpus and initialize the Trie
 * @param fstr the file to read data from
 * @param trie the trie to put the data into
 */
static void fillInTrie(ifstream & fstr, ITrie & trie) {
    //A trie container and the corps file stream are already instantiated and are given

    //A.1. Create the TrieBuilder and give the trie to it
    TrieBuilder builder(trie, fstr, TOKEN_DELIMITER_CHAR);

    //A.2. Build the trie
    builder.build();
}

/**
 * This method will perform the main tasks of this application:
 * Read the text corpus and create a trie and then read the test
 * file and query the trie for frequencies.
 * @param trainFile the text corpus file
 * @param testFile the test file with queries
 */
static void performTasks(ifstream &trainFile, ifstream &testFile) {
    //Declare time variables for CPU times in seconds
    double startTime, endTime;

    BasicLogger::printDebug("Getting the initial memory statistics ...");
    //Declare the statistics monitor and its data
    TMemotyUsage memStatStart = {}, memStatInterm = {};
    StatisticsMonitor::getMemoryStatistics(memStatStart);

    //Create a trie and pass it to the algorithm method
    HashMapTrie<N_GRAM_PARAM> trie;
    BasicLogger::printInfo("Start reading the text corpus and filling in the Trie ...");
    startTime = StatisticsMonitor::getCPUTime();
    fillInTrie(trainFile, trie);
    endTime = StatisticsMonitor::getCPUTime();
    BasicLogger::printInfo("Reading the text corpus is done, it took %lf CPU seconds.", endTime - startTime);

    BasicLogger::printDebug("Getting the intermediate memory statistics ...");
    StatisticsMonitor::getMemoryStatistics(memStatInterm);

    BasicLogger::printDebug("Reporting on the memory consumption");
    reportMemotyUsage("Loading of the text corpus Trie", memStatStart, memStatInterm);

    BasicLogger::printInfo("Reading and executing the test queries ...");
    //ToDo: Think through how to work with test data, in principle we
    //      can read the file consequently (get 5-gram's) and then query
    //      the tire for the resulting frequences:
    //      Create the NGramReader for the file, give it to the NGramBuilder
    //      Ask the NGramBuilder for subsequent

    BasicLogger::printInfo("Done");
}

/**
 * The main program entry point
 */
int main(int argc, char** argv) {
    //Declare the return code
    int returnCode = 0;

    //First print the program info
    printInfo();

    try {
        BasicLogger::printInfo("Checking on the program arguments ...");
        //Attempt to extract the program arguments
        TAppParams params = {};
        extractArguments(argc, argv, params);

        BasicLogger::printInfo("Checking on the provided files \'%s\' and \'%s\' ...",
                params.trainFileName.c_str(), params.testFileName.c_str());

        //Attempt to open the files
        ifstream trainFile(params.trainFileName.c_str());
        ifstream testFile(params.testFileName.c_str());

        //If the files could be opened then proceed with training and then testing
        if ((trainFile != NULL) && (testFile != NULL)) {
            //Do the actual work, read the text corpse, create trie and do queries
            performTasks(trainFile, testFile);
        } else {
            stringstream msg;
            msg << "One of the input files does not exist: " +
                    getFileExistsString(params.trainFileName, trainFile)
                    + " , " +
                    getFileExistsString(params.testFileName, testFile);
            throw Exception(msg.str());
        }
    } catch (Exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        BasicLogger::printError(ex);
        printUsage((string) argv[0]);
        returnCode = 1;
    }

    return returnCode;
}
