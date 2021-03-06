/* 
 * File:   main.cpp
 * Author: Dr. Ivan S. Zapreev
 *
 * Visit my Linked-in profile:
 *      <https://nl.linkedin.com/in/zapreevis>
 * Visit my GitHub:
 *      <https://github.com/ivan-zapreev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.#
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created on April 18, 2015, 11:35 AM
 */

#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream, std::stringbuf
#include <fstream>      // std::ifstream
#include <algorithm>    // std::transform

#include "Exceptions.hpp"
#include "StatisticsMonitor.hpp"
#include "Logger.hpp"
#include "ATrie.hpp"
#include "HashMapTrie.hpp"
#include "TrieBuilder.hpp"
#include "Globals.hpp"
#include "NGramBuilder.hpp"

using namespace std;
using namespace tries;
    
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
    LOG_USAGE << " ------------------------------------------------------------------ " << END_LOG;
    LOG_USAGE << "|                    Automated Translation Tires         :)\\___/(: |" << END_LOG;
    LOG_USAGE << "|                     Test software version 1.1          {(@)v(@)} |" << END_LOG;
    LOG_USAGE << "|                                                        {|~- -~|} |" << END_LOG;
    LOG_USAGE << "|             Copyright (C) Dr. Ivan S Zapreev, 2015     {/^'^'^\\} |" << END_LOG;
    LOG_USAGE << "|  ═════════════════════════════════════════════════════════m-m══  |" << END_LOG;
    LOG_USAGE << "|        This software is distributed under GPL 2.0 license        |" << END_LOG;
    LOG_USAGE << "|          (GPL stands for GNU General Public License)             |" << END_LOG;
    LOG_USAGE << "|          The product comes with ABSOLUTELY NO WARRANTY.          |" << END_LOG;
    LOG_USAGE << "|   This is a free software, you are welcome to redistribute it.   |" << END_LOG;
    LOG_USAGE << " ------------------------------------------------------------------ " << END_LOG;
}

/**
 * This function prints the usage information for the software
 * @param name the absolute name of the application 
 */
static void printUsage(const string name) {
    //Since we do not want the absolute file name, we need to chop off the directory prefix
    const unsigned int lastSlashBeforeFileName = name.find_last_of(PATH_SEPARATION_SYMBOLS);
    const string shortName = name.substr(lastSlashBeforeFileName + 1);

    LOG_USAGE << "Running: " << END_LOG;
    LOG_USAGE << "  " << shortName.c_str() << " <train_file> <test_file> [debug-level]" << END_LOG;
    LOG_USAGE << "      <train_file> - a text file containing the training text corpus." << END_LOG;
    LOG_USAGE << "                     This corpus should be already tokenized, i.e.," << END_LOG;
    LOG_USAGE << "                     all words are already separated by white spaces," << END_LOG;
    LOG_USAGE << "                     including punctuation marks. Also, each line in " << END_LOG;
    LOG_USAGE << "                     this, file corresponds to one sentence." << END_LOG;
    LOG_USAGE << "      <test_file>  - a text file containing test data." << END_LOG;
    LOG_USAGE << "                     The test file consists of a number of 5-grams," << END_LOG;
    LOG_USAGE << "                     where each line in the file consists of one 5-gram." << END_LOG;
    LOG_USAGE << "     [debug-level] - the optional debug flag from " << DEBUG_OPTION_VALUES << END_LOG;

    LOG_USAGE << "Output: " << END_LOG;
    LOG_USAGE << "    The program reads in the test lines from the <test_file>. " << END_LOG;
    LOG_USAGE << "    Each of these lines is a 5-gram of the following form: " << END_LOG;
    LOG_USAGE << "       word1 word2 word3 word4 word5" << END_LOG;
    LOG_USAGE << "    For each of such 5-grams the frequency information is " << END_LOG;
    LOG_USAGE << "    computed, based on the data from the <train_file>. For" << END_LOG;
    LOG_USAGE << "    example, for a 5-gram such as:" << END_LOG;
    LOG_USAGE << "       mortgages had lured borrowers and" << END_LOG;
    LOG_USAGE << "    the program may give the following output:" << END_LOG;
    LOG_USAGE << "        frequency( mortgages had lured borrowers and ) = 0" << END_LOG;
    LOG_USAGE << "        frequency( had lured borrowers and ) = 2" << END_LOG;
    LOG_USAGE << "        frequency( lured borrowers and ) = 4" << END_LOG;
    LOG_USAGE << "        frequency( borrowers and ) = 56" << END_LOG;
    LOG_USAGE << "        frequency( and ) = 6453" << END_LOG;
}

/**
 * This function tries to extract the 
 * @param argc the number of program arguments
 * @param argv the array of program arguments
 * @param params the structure that will be filled in with the parsed program arguments
 */
static void extractArguments(const int argc, char const * const * const argv, TAppParams & params) {
    if (argc < EXPECTED_NUMBER_OF_ARGUMENTS) {
        stringstream msg;
        msg << "Incorrect number of arguments, expected >= " << EXPECTED_USER_NUMBER_OF_ARGUMENTS << ", got " << (argc - 1);
        throw Exception(msg.str());
    } else {
        params.trainFileName = argv[1];
        params.testFileName = argv[2];
        
        //This here is a fast hack, it is not a really the
        //nicest way to handle the program parameters but
        //this is ok for a test software.
        if( argc > EXPECTED_NUMBER_OF_ARGUMENTS ) {
            string data = argv[3];
            transform(data.begin(), data.end(), data.begin(), ::tolower);
            if(!data.compare( INFO_PARAM_VALUE )) {
                Logger::ReportingLevel() = Logger::INFO;
                LOG_INFO << "Setting the debugging level to \'" << INFO_PARAM_VALUE << "\'" << END_LOG;
            } else {
                if(!data.compare( DEBUG_PARAM_VALUE )){
                    Logger::ReportingLevel() = Logger::DEBUG;
                    LOG_INFO << "Setting the debugging level to \'" << DEBUG_PARAM_VALUE << "\'" << END_LOG;
                } else {
                    LOG_WARNING << "Ignoring an unknown value of [debug-level] parameter: '" << argv[3] << "'" << END_LOG;
                }
            }
        }
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
    LOG_RESULT << "Action: \'" << action << "\' memory increase:" << END_LOG;
    LOG_DEBUG << "memory before: vmsize=" << msStart.vmsize << " Kb, vmpeak="
                               << msStart.vmpeak << " Kb, vmrss=" << msStart.vmrss
                               << " Kb, vmhwm=" << msStart.vmhwm << " Kb" << END_LOG;
    LOG_DEBUG << "memory after: vmsize=" << msEnd.vmsize << " Kb, vmpeak="
                               << msEnd.vmpeak << " Kb, vmrss=" << msEnd.vmrss
                               << " Kb, vmhwm=" << msEnd.vmhwm << " Kb" << END_LOG;
    LOG_RESULT << "vmsize=" << double(msEnd.vmsize - msStart.vmsize)/BYTES_ONE_MB
                                << " Mb, vmpeak=" << double(msEnd.vmpeak - msStart.vmpeak)/BYTES_ONE_MB
                                << " Mb, vmrss=" << double(msEnd.vmrss - msStart.vmrss)/BYTES_ONE_MB
                                << " Mb, vmhwm=" << double(msEnd.vmhwm - msStart.vmhwm)/BYTES_ONE_MB << " Mb" << END_LOG;
    LOG_INFO << "  vmsize - Virtual memory size; vmpeak - Peak virtual memory size" << END_LOG;
    LOG_INFO << "    Virtual memory size is how much virtual memory the process has in total (RAM+SWAP)" << END_LOG;
    LOG_INFO << "  vmrss  - Resident set size; vmhwm  - Peak resident set size" << END_LOG;
    LOG_INFO << "    Resident set size is how much memory this process currently has in main memory (RAM)" << END_LOG;
}

/**
 * THis method is used to read from the corpus and initialize the Trie
 * @param fstr the file to read data from
 * @param trie the trie to put the data into
 */
template<TTrieSize N, bool doCache>
static void fillInTrie(ifstream & fstr, ATrie<N,doCache> & trie) {
    //A trie container and the corps file stream are already instantiated and are given

    //A.1. Create the TrieBuilder and give the trie to it
    TrieBuilder<N,doCache> builder(trie, fstr, TOKEN_DELIMITER_CHAR);

    //A.2. Build the trie
    builder.build();
}

/**
 * Allows to read and execute test queries from the given file on the given trie.
 * @param trie the given trie, filled in with some data
 * @param testFile the file containing the N-Gram (5-Gram queries)
 * @return the CPU seconds used to run the queries, without time needed to read the test file
 */
template<TTrieSize N, bool doCache>
static double readAndExecuteQueries( ATrie<N,doCache> & trie, ifstream &testFile) {
    //Declare time variables for CPU times in seconds
    double totalTime, startTime, endTime;
    //Will store the read line (word1 word2 word3 word4 word5)
    string line;
    //Will store the N-gram [word1 word2 word3 word4 word5] corresponding to the line
    vector<string> ngram;
    //Will store the N-gram frequencies for N-gram:
    //freqs[0] = frequency( [word1 word2 word3 word4 word5] )
    //freqs[1] = frequency( [word2 word3 word4 word5] )
    //freqs[2] = frequency( [word3 word4 word5] )
    //freqs[3] = frequency( [word4 word5] )
    //freqs[4] = frequency( [word5] )
    SFrequencyResult<N> freqs;
        
    //Read the test file line by line
    while( getline(testFile, line) )
    {
        //First get the complete N-gram
        ngrams::NGramBuilder<N,doCache>::buildNGram(line, N, TOKEN_DELIMITER_CHAR, ngram);
        
        LOG_DEBUG <<  line << ":" << END_LOG;
        
        //Second qury the Trie for the results
        startTime = StatisticsMonitor::getCPUTime();
        trie.queryNGramFreqs( ngram, freqs );
        endTime = StatisticsMonitor::getCPUTime();
        
        //Print the results:
        ;
        unsigned idx = -1;
        for(int i=0;i<N;i++){
            LOG_RESULT << "frequency( " << line << " ) = " << freqs.result[i] << END_LOG;
            
            idx = line.find_first_of(TOKEN_DELIMITER_CHAR);
            line = line.substr(idx+1);
        }
        LOG_RESULT << "CPU Time needed: " << (endTime - startTime) << " sec." << END_LOG;

        //update total time
        totalTime += (endTime - startTime);
    }

    return totalTime;
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

    LOG_DEBUG << "Getting the initial memory statistics ..." << END_LOG;
    //Declare the statistics monitor and its data
    TMemotyUsage memStatStart = {}, memStatInterm = {};
    StatisticsMonitor::getMemoryStatistics(memStatStart);

    //Create a trie and pass it to the algorithm method
    TFiveCacheHashMapTrie trie;
    LOG_RESULT << "Start reading the text corpus and filling in the Trie ..." << END_LOG;
    startTime = StatisticsMonitor::getCPUTime();
    fillInTrie(trainFile, trie);
    endTime = StatisticsMonitor::getCPUTime();
    LOG_RESULT << "Reading the text corpus is done, it took " << (endTime - startTime) << " CPU seconds." << END_LOG;

    LOG_DEBUG << "Getting the intermediate memory statistics ..." << END_LOG;
    StatisticsMonitor::getMemoryStatistics(memStatInterm);

    LOG_DEBUG << "Reporting on the memory consumption" << END_LOG;
    reportMemotyUsage("Loading of the text corpus Trie", memStatStart, memStatInterm);

    LOG_RESULT << "Reading and executing the test queries ..." << END_LOG;
    const double queryCPUTimes = readAndExecuteQueries(trie, testFile);
    LOG_RESULT << "Total query execution time is " << queryCPUTimes << " CPU seconds." << END_LOG;
  
    LOG_RESULT << "Done" << END_LOG;
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
        LOG_INFO << "Checking on the program arguments ..." << END_LOG;
        //Attempt to extract the program arguments
        TAppParams params = {};
        extractArguments(argc, argv, params);

        LOG_INFO << "Checking on the provided files \'"
                                  << params.trainFileName << "\' and \'"
                                  << params.testFileName << "\' ..." << END_LOG;

        //Attempt to open the files
        ifstream trainFile(params.trainFileName.c_str());
        ifstream testFile(params.testFileName.c_str());

        //If the files could be opened then proceed with training and then testing
        if ((trainFile.is_open()) && (testFile.is_open())) {
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
        LOG_ERROR << ex.getMessage() << END_LOG;
        printUsage((string) argv[0]);
        returnCode = 1;
    }

    return returnCode;
}

