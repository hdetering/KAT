//  ********************************************************************
//  This file is part of KAT - the K-mer Analysis Toolkit.
//
//  KAT is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  KAT is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with KAT.  If not, see <http://www.gnu.org/licenses/>.
//  *******************************************************************

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <glob.h>
using std::fstream;
using std::stringstream;

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/timer/timer.hpp>
#include <boost/algorithm/string.hpp>
namespace bfs = boost::filesystem;
using bfs::path;
using boost::timer::auto_cpu_timer;
using boost::split;

#include <kat/jellyfish_helper.hpp>
using kat::JellyfishHelper;

#include <kat/input_handler.hpp>

void kat::InputHandler::setMultipleInputs(const vector<path>& inputs) {
    input.clear();
    trim5p.clear();
    for(auto& p : inputs) {
        input.push_back(p);
        trim5p.push_back(0);
    }
}

void kat::InputHandler::set5pTrim(const vector<uint16_t>& trim_list) {
    trim5p.clear();
    if (trim_list.empty()) {
        for(auto& p : input) {
            trim5p.push_back(0);
        }
    }
    else if (trim_list.size() == 1 && input.size() > 1) {
        for(auto& p : input) {
            trim5p.push_back(trim_list[0]);
        }
    }
    else if (trim_list.size() == input.size()) {
        for(auto& p : trim_list) {
            trim5p.push_back(p);
        }
    }
    else {
        BOOST_THROW_EXCEPTION(InputFileException() << InputFileErrorInfo(string(
            "Inconsistent number of inputs and trimming settings.  Please establish your inputs before trying to set trimming vector.  Also ensure you have the same number of input files to trimming settings.")));
    }
}

void kat::InputHandler::set3pTrim(const vector<uint16_t>& trim_list) {
    trim3p.clear();
    if (trim_list.empty()) {
        for(auto& p : input) {
            trim3p.push_back(0);
        }
    }
    else if (trim_list.size() == 1 && input.size() > 1) {
        for(auto& p : input) {
            trim3p.push_back(trim_list[0]);
        }
    }
    else if (trim_list.size() == input.size()) {
        for(auto& p : trim_list) {
            trim3p.push_back(p);
        }
    }
    else {
        BOOST_THROW_EXCEPTION(InputFileException() << InputFileErrorInfo(string(
            "Inconsistent number of inputs and trimming settings.  Please establish your inputs before trying to set trimming vector.  Also ensure you have the same number of input files to trimming settings.")));
    }
}

void kat::InputHandler::validateInput() {

    bool start = true;
    //if (input.size() != trim3p.size() || trim3p.size() != trim5p.size()) {
    if (input.size() != trim5p.size()) {
        BOOST_THROW_EXCEPTION(InputFileException() << InputFileErrorInfo(string(
            "Inconsistent number of inputs and trimming settings.")));
    }

    // Check input file(s) exists
    for(auto& rp : input) {

        path p(rp);

        if (bfs::is_symlink(p)) {
            if (bfs::symbolic_link_exists(p)) {
                p = bfs::canonical(p);
            }
            else {
                BOOST_THROW_EXCEPTION(InputFileException() << InputFileErrorInfo(string(
                    "Could not find input file at: ") + p.string() + "; please check the path and try again."));
            }
        }

        if (!JellyfishHelper::isPipe(p) && !bfs::exists(p)) {
            BOOST_THROW_EXCEPTION(InputFileException() << InputFileErrorInfo(string(
                    "Could not find input file at: ") + p.string() + "; please check the path and try again."));
        }

        InputMode m = JellyfishHelper::isSequenceFile(p) ? InputMode::COUNT : InputMode::LOAD;
        if (start) {
            mode = m;
        }
        else {
            if (m != mode) {
                BOOST_THROW_EXCEPTION(InputFileException() << InputFileErrorInfo(string(
                    "Cannot mix sequence files and jellyfish hashes.  Input: ") + p.string()));
            }
        }
    }
}

void kat::InputHandler::loadHeader() {
    if (mode == InputMode::LOAD) {
        header = JellyfishHelper::loadHashHeader(input[0]);
    }
}

void kat::InputHandler::validateMerLen(const uint16_t merLen) {

    if (mode == InputMode::LOAD) {
        if (header->key_len() != merLen * 2) {

            BOOST_THROW_EXCEPTION(JellyfishException() << JellyfishErrorInfo(string(
                "Cannot process hashes that were created with different K-mer lengths.  Expected: ") +
                lexical_cast<string>(merLen) +
                ".  Key length was " +
                lexical_cast<string>(header->key_len() / 2) +
                " for : " + input[0].string()));
        }
    }
}

string kat::InputHandler::pathString() {

    string s;
    uint16_t index = 1;
    for(auto& p : input) {
        string msg = JellyfishHelper::isPipe(p) ? string("<pipe>") : p.string();
        s += msg + " ";
    }
    return boost::trim_right_copy(s);
}

string kat::InputHandler::fileName() {

    string s;
    for(auto& p : input) {
        s += p.leaf().string() + " ";
    }
    return boost::trim_right_copy(s);
}

void kat::InputHandler::count(const uint16_t threads) {

    auto_cpu_timer timer(1, "  Time taken: %ws\n\n");

    hashCounter = make_shared<HashCounter>(hashSize, merLen * 2, 7, threads);
    hashCounter->do_size_doubling(!disableHashGrow);

    cout << "Input " << index << " is a sequence file.  Counting kmers for input " << index << " (" << pathString() << ") ...";
    cout.flush();

    hash = JellyfishHelper::countSeqFile(input, *hashCounter, canonical, threads, trim5p, trim3p);

    // Create header for newly counted hash
    header = make_shared<file_header>();
    header->fill_standard();
    header->update_from_ary(*hash);
    header->counter_len(4);  // Hard code for now.
    header->canonical(canonical);
    header->format(binary_dumper::format);

    cout << " done.";
    cout.flush();
}

void kat::InputHandler::loadHash() {

    auto_cpu_timer timer(1, "  Time taken: %ws\n\n");

    cout << "Loading hashes into memory...";
    cout.flush();

    hashLoader = make_shared<HashLoader>();
    hashLoader->loadHash(input[0], false);
    hash = hashLoader->getHash();
    canonical = hashLoader->getCanonical();
    merLen = hashLoader->getMerLen();

    cout << " done.";
    cout.flush();
}

void kat::InputHandler::dump(const path& outputPath, const uint16_t threads) {

    // Remove anything that exists at the target location
    if (bfs::is_symlink(outputPath) || bfs::exists(outputPath)) {
        bfs::remove(outputPath.c_str());
    }

    // Either dump or symlink as appropriate
    if (mode == InputHandler::InputHandler::InputMode::COUNT) {

        auto_cpu_timer timer(1, "  Time taken: %ws\n\n");
        cout << "Dumping hash to " << outputPath.string() << " ...";
        cout.flush();

        JellyfishHelper::dumpHash(hash, *header, threads, outputPath);

        cout << " done.";
        cout.flush();
    }
    else {
        bfs::create_symlink(getSingleInput(), outputPath);
    }
}

shared_ptr<vector<path>> kat::InputHandler::globFiles(const string& input) {

    vector<string> inputvec;
    boost::split(inputvec, input, boost::is_any_of(" "));

    vector<path> pathvec;
    for(auto& s : inputvec) {
        pathvec.push_back(s);
    }

    return globFiles(pathvec);
}

int kat::InputHandler::globerr(const char *path, int eerrno) {

    fprintf(stderr, "Globbing error: %s: %s\n", path, strerror(eerrno));
    return eerrno;
}

shared_ptr<vector<path>> kat::InputHandler::globFiles(const vector<path>& input) {

    glob_t globbuf;

    if (input.empty()) {
        BOOST_THROW_EXCEPTION(InputFileException() << InputFileErrorInfo(string("No input provided for this input group")));
    }

    // Translate glob patterns into real paths
    int i = 0;
    for(auto& g : input) {
        // Build the flags.
        // This means if the user has listed files
        // in this input group separated with a space
        // then combine results together.
        // Also expand tildes (home directory) and braces regardless and don't check to see
        // if the target file exists... let that check happen downstream
        int flags = GLOB_TILDE | GLOB_NOCHECK | GLOB_BRACE;
        if (i > 0)
            flags |= GLOB_APPEND;

        // Run glob, puts results in globbuf
        int ret = glob(g.c_str(), flags, globerr, &globbuf);
        if (ret != 0) {
            stringstream ss;
            ss << "Problem globbing input pattern: " << g.c_str() << ". Non-zero return code.  Error type: " <<
                    (   ret == GLOB_ABORTED ? "filesystem problem" :
                        ret == GLOB_NOMATCH ? "no match of pattern" :
                        ret == GLOB_NOSPACE ? "no dynamic memory" :
                        "unknown problem");
            BOOST_THROW_EXCEPTION(InputFileException() << InputFileErrorInfo(ss.str()));
        }
        i++;
    }

    // Put globbed data into the results array
    shared_ptr<vector<path>> globbed = make_shared<vector<path>>();
    for( size_t i = 0; i < globbuf.gl_pathc; ++i )
        globbed->push_back( path(string(globbuf.gl_pathv[i])) );

    // Only free glob results if there's something to free
    if( globbuf.gl_pathc > 0 )
        globfree( &globbuf );

    // Check for content.  If there isn't any then probably the input file doesn't
    // exist.  But we add the basic input regardless, the user will have to check
    // for file non-existence later.
    if (globbed->empty()) {
        globbed->push_back(input[0]);
    }

    return globbed;
}

string kat::InputHandler::determineSequenceFileType(const path& filename) {

    string fn_str = filename.string();
    string ext = filename.extension().string();

    // Check for gz extension first, if found remove it
    if (boost::iequals(ext, ".gz")) {
        size_t lastindex = fn_str.find_last_of(".");
        path wogz_filename = path(fn_str.substr(0, lastindex));
        ext = wogz_filename.extension().string();
    }

    // Check extension first
    if (boost::iequals(ext, ".fastq") || boost::iequals(ext, ".fq")) {
        return "fastq";
    }
    else if (   boost::iequals(ext, ".fasta") ||
                boost::iequals(ext, ".fa") ||
                boost::iequals(ext, ".fna") ||
                boost::iequals(ext, ".fas") ||
                boost::iequals(ext, ".scafSeq")) {
        return "fasta";
    }
    else {
        // Now check first character of the file
        char ch;
        fstream fin(filename.string(), fstream::in);
        fin >> ch;
        fin.close();

        if (ch == '>') {
            return "fasta";
        }
        else if (ch == '@') {
            return "fastq";
        }
    }

    // If we've got this far then it's not obviously a sequence file we recognise.
    BOOST_THROW_EXCEPTION(InputFileException() << InputFileErrorInfo("Unknown file type"));
}
