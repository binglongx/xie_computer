#pragma once

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <filesystem>

#include "ref.h"

struct SourceFile;

struct CodeLine
{
    int                         number;         // line number in the file
    std::string                 original;       // verbatim text of line.
    std::string                 regularized;    // comment removed text of line. if this is #include, the file content is in `inclusion` below;
    std::unique_ptr<SourceFile> inclusion;      //   otherwise `inclusion` is empty
    std::vector<std::string>    tokens;         //== comment-removed, label-removed, tokens. [empty after loading; used in assembling only]
};

struct SourceFile
{
    std::string                 filePath;   // file path
    std::vector<CodeLine>       lines;      // contents of this file as individual lines
};

// bool fo(std::string& filePath, CodeLine& line);  // false: error.
template<class FO>
bool do_for_each_line(SourceFile& file, FO fo, int level)
{
    for(CodeLine& line : file.lines)
    {
        if( ! line.inclusion )
        {
            if( !fo(file.filePath, line) )
                return false;
        }
        else
        {
            if( ! do_for_each_line(*line.inclusion, fo, level+1) )
            {
                std::cout << "["<<level<<"] " << "At: " << file.filePath << ", Line: " << line.number << std::endl
                    <<"    " << line.original << std::endl;
                return false;
            }
        }
    }
    return true;
}

// bool fo(std::string& filePath, CodeLine& line);  // false: error.
template<class FO>
bool for_each_line(SourceFile& file, FO fo)
{
    return do_for_each_line(file, fo, 0);
}


std::string                 upper(const std::string& str);
int                         string_to_number(const std::string& str);
std::vector<std::string>    tokenize(const std::string& str);
bool                        remove_inline_comments(std::string& line);
bool                        detect_and_remove_label_for_line(std::vector<std::string>& tokens, std::string& label);

struct Loader
{
    std::filesystem::path               currentDir;
    std::vector<std::filesystem::path>  extraIncludeDirs;
    
    // load the source code lines from file path and also handle #include recursively.
    bool    load(const std::string& filePath, SourceFile& file);

    // load the source code lines from a string of source code. It should not have #includes.
    bool    loadFromString(const std::string& sourceCodeContents, SourceFile& file);
    
private:
    bool    recurse_load(const std::string& includePath, SourceFile& file, int level);
    
    template<class FO>
    bool load_istream(const std::string& filePath, std::istream& is, SourceFile& file, int level, FO inclusionHandler);

    std::map<std::string, bool>    mapIncludedFiles;    // absolute path
};


//==============================================================================================================================
//==============================================================================================================================

// str could be "1288", "0x3000", "0X3000", "0b00110011", "0B00110011", "'a'".
inline int string_to_number(const std::string& str)
{
    int val = 0;
    if( str.size()>2 )
    {
        std::string prefix_ = str.substr(0, 2);
        auto prefix = upper(prefix_);
        if( prefix=="0X" )
        {
            std::string proper = str.substr(2);
            std::istringstream ss(proper);
            ss >> std::hex >> val;
            return val;
        }
        else if( prefix=="0B" )
        {
            std::string proper = str.substr(2);
            int term = 1;
            for (int i=0; i<proper.length(); i++)
            {
                int ind = (int)(proper.size()-1-i);
                char bit = proper[ind];
                if( bit=='1' )
                    val += term;
                else if( bit != '0' )
                {
                    // there is something wrong
                }
                term *= 2;
            }
            return val;
        }
        else if(( str.front() == '\'' && str.back() == '\'') || ( str.front() == '"' && str.back() == '"'))
        {
            std::string proper = str.substr(1, str.size()-2);
            for(int i=0; i<proper.size(); ++i)
            {
                int ind = int(proper.size()-1-i);
                unsigned int ch = (unsigned char)(proper[ind]);
                val |= (ch << (8*ind));
            }
            return val;
        }
    }
    val = stoi(str);
    return val;
}

inline std::vector<std::string> tokenize(const std::string& str)
{
    std::vector<std::string> tokens;
    std::istringstream stream{str};
    std::string token;
    while (stream >> token)
        tokens.push_back(token);
    return tokens;
}

inline std::string upper(const std::string& str)
{
    std::string s = str;
    for (char& letter : s)
        letter = toupper(letter);
    return s;
}

// from the line, remove `//` and beyond; or replace well-formed `/*` and `*/` pairs with ' ', then return true.
// if it finds non-well formed `/*` following `/*` sequence, it returns false.
inline bool remove_inline_comments(std::string& line)
{
    if( line.empty() )
        return true;
    
    // check for // or /*
    size_t locCommentStart = line.find("/*");
    size_t locInlineCommentStart = line.find("//");
    if( std::string::npos != locInlineCommentStart && (std::string::npos == locCommentStart || locInlineCommentStart<locCommentStart) )
    {
        // `//` takes effect, remove everything after `//`.
        line.erase(locInlineCommentStart);
    }
    
    // remove those `/*` and `*/` pairs in the line.
    size_t loc = 0;
    while( loc<line.size() )
    {
        // each iteration should remove a pair of `/*` and `*/`
        
        // check if we can find `/*`
        loc = line.find("/*", loc);
        if( std::string::npos != loc )
        {
            // `/*` found.
            size_t loc2a = line.find("*/", loc+2);
            size_t loc2b = line.find("/*", loc+2);
            if( loc2a < loc2b )
            {
                // `*/` inline.
                line.replace(loc, loc2a-loc+2, 1, ' ');
            }
            else if( loc2b < loc2a )
            {
                // `/*` following `/*`.
                return false;
            }
            else
            {
                // no `*/` or `/*` found.
                return true;
            }
        }
        else
            break;
    }
    return true; //
}

//==============================================================================================================================
//==============================================================================================================================

// load the contents of of the filePath into file.
inline bool Loader::recurse_load(const std::string& includePath, SourceFile& file, int level)
{
    std::ifstream ifs;
    std::filesystem::path filePath = includePath;
    if( filePath.is_absolute() )
    {
        // absolute include path, use it verbatim.
        ifs.open(filePath);
        if( !ifs.is_open() )
        {
            std::cout << "["<<level<<"] " << "Cannot open source file (absolute path): " << filePath << std::endl;
            return false;
        }
    }
    else
    {
        // 1. currentDir
        filePath = std::filesystem::absolute(this->currentDir / includePath);
        ifs.open(filePath);
        if( !ifs.is_open() )
        {
            // 2. try includeDirs
            for(const auto& dir : extraIncludeDirs)
            {
                filePath = std::filesystem::absolute(dir / includePath);
                ifs.open(filePath);
                if( ifs.is_open() )
                {
                    break;
                }
            }
        }
        
        if( !ifs.is_open() )
        {
            std::cout << "["<<level<<"] " << "Cannot open source file: " << includePath << "\n"
                << "    tried current directory: " << "\n"
                << "      " << currentDir << "\n";
            if( !extraIncludeDirs.empty() )
            {
                std::cout << "    and additional include directories: \n";
                for(size_t i=0; i<extraIncludeDirs.size(); ++i)
                    std::cout << "      [0]: " << extraIncludeDirs[i] << "\n";
            }
            return false;
        }
    }
    
    // the file has been opened.
    if( mapIncludedFiles.find(filePath.string()) != mapIncludedFiles.end() )
    {
        // the file has been included before. do not process it, or we have duplicated labels and machine codes.
        std::cout << "Hint: ["<<level<<"] " << "Source was already included: " << filePath << std::endl;
        file.filePath = filePath;
        file.lines.clear();
        return true;    // skip the lines in this file.
    }
    mapIncludedFiles[filePath.string()] = true;

    // load ifstream as istream.
    return load_istream(filePath.string(), ifs, file, level,
                        // if load_istream encounters #include, this function below will be called to handle that.
                        [this](const std::string& includePath, SourceFile& file, int level){
                            return this->recurse_load(includePath, file, level);
                        }
    );
}

template<class FO>
inline bool Loader::load_istream(const std::string& filePath, std::istream& is, SourceFile& file, int level, FO inclusionHandler)
{
    file.filePath = filePath;
    file.lines.clear();
    
    std::string original, regularized;
    bool multiline_comment_open = false;
    int number = 1;
    while( std::getline(is, original) )
    {
        regularized = original;
        
        //==== Take care of comments. regularized line will contain comment-removed text.
        // in-line comments take precedence to be removed.
        if( ! remove_inline_comments(regularized) )
        {
            // `/*` following `/*`.
            std::cout << "["<<level<<"] " << "Syntax error: `/*` following `/*`: " << filePath << ", Line: " << number << std::endl
                <<"    " << original << std::endl;
            return false;
        }
        
        // multi-line comments are processed here.
        if( multiline_comment_open )
        {
            size_t loc2a = regularized.find("*/");
            size_t loc2b = regularized.find("/*");
            if( loc2b < loc2a )
            {
                // `/*` following earlier `/*`.
                std::cout << "["<<level<<"] " << "Syntax error: `/*` following earlier line's `/*`: " << filePath << ", Line: " << number << std::endl
                    <<"    " << original << std::endl;
                return false;
            }
            else if( loc2a < loc2b )
            {
                // `*/`
                regularized.replace(0, loc2a+2, 1, ' ');
                multiline_comment_open = false;
            }
            else
            {
                // neither of `/*` and `*/`: this is a commented line
                regularized.clear();
            }
        }
        if( ! multiline_comment_open )
        {
            if( std::string::npos != regularized.find("*/") )
            {
                // `*/`, this is illegal.
                std::cout << "["<<level<<"] " << "Syntax error: `*/` without earlier matching `/*`: " << filePath << ", Line: " << number << std::endl
                    <<"    " << original << std::endl;
                return false;
            }

            size_t locCommentStart = regularized.find("/*");
            if( std::string::npos != locCommentStart )
            {
                // has `/*`: this is multiple line comment opening.
                regularized.erase(locCommentStart);
                multiline_comment_open = true;
            }
        }

        //==== Now regularized is stripped of comments.
        //==== Take care of #include.
        std::string includeFilePath;    // will be non-empty if this is a valid #include line.
        
        // check if this is #include.
        std::vector<std::string> tokens = tokenize(regularized);
        if( tokens.size()>=2 )
        {
            std::string included;
            bool syntaxError = false;
            if( tokens[0]=="#" && tokens[1]=="include" )
            {
                if( tokens.size()==3 )
                    included = tokens[2];
                else
                    syntaxError = true;
            }
            else if( tokens[0]=="#include" )
            {
                if( tokens.size()==2 )
                    included = tokens[1];
                else
                    syntaxError = true;
            }
            if( !included.empty() )
            {
                if( included.size()>2 && included.front()=='\"' && included.back()=='\"' )
                    includeFilePath = included.substr(1, included.size()-2);
                else
                    syntaxError = true;
            }
            if( syntaxError )
            {
                std::cout << "["<<level<<"] " << "Syntax error: #include: " << filePath << ", Line: " << number << std::endl
                    <<"    " << original << std::endl;
                return false;
            }
        }
        
        
        std::unique_ptr<SourceFile> inclusion;
        // if we have a valid #include line
        if( ! includeFilePath.empty() )
        {
            inclusion = std::make_unique<SourceFile>();
            //if( !recurse_load(includeFilePath, *inclusion, level+1) )
            if( !inclusionHandler(includeFilePath, *inclusion, level+1) )
            {
                std::cout << "["<<level<<"] " << "At: " << filePath << ", Line: " << number << std::endl
                    <<"    " << original << std::endl;
                return false;
            }
        }
        
        CodeLine line{ number, original, regularized, std::move(inclusion), {} };
        file.lines.push_back(std::move(line));
        
        ++number;
    }
    return true;
}

inline bool Loader::load(const std::string& filePath, SourceFile& file)
{
    return recurse_load(filePath, file, 0);
}

inline bool Loader::loadFromString(const std::string& sourceCodeContents, SourceFile& file)
{
    std::istringstream ss(sourceCodeContents);
    return load_istream("<<DUMMY_MEMORY>>", ss, file, 0,
                      // if load_istream encounters #include, this function below will be called to handle that.
                      [](const std::string& includePath, SourceFile& file, int level){
                            assert(false);
                            std::cout << "["<<level<<"] " << "<<DUMMY_MEMORY>> source code cannot handle #include: " << includePath << std::endl;
                            return false;
                      }
    );
}

//==============================================================================================================================
//==============================================================================================================================

// label must be the first token like "label:"
// input tokens must have stripped away comments
// return non-empty label string excluding ':' if this line has a leading label
// tokens will not include label after calling this function.
inline bool detect_and_remove_label_for_line(std::vector<std::string>& tokens, std::string& label)
{
    if (tokens.size()>0 && tokens.front().back()==':')
    {
        std::string lab = tokens.front();
        lab.pop_back();
        if( lab.empty() )
            return false;   // label cannot be empty: error
        label = lab;
        tokens.erase(tokens.begin());
        return true;
    }
    label.clear();  // no label.
    return true;
}
