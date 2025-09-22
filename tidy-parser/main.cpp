//
//  main.cpp
//  tidy-parser
//
//  Created by miyako on 2025/09/09.
//

#include "tidy-parser.h"

static void usage(void)
{
    fprintf(stderr, "Usage:  tidy-parser -r -i in -o out -\n\n");
    fprintf(stderr, "text extractor for html documents\n\n");
    fprintf(stderr, " -%c path: %s\n", 'i' , "document to parse");
    fprintf(stderr, " -%c path: %s\n", 'o' , "text output (default=stdout)");
    fprintf(stderr, " %c: %s\n", '-' , "use stdin for input");
    fprintf(stderr, " -%c: %s\n", 'r' , "raw text output (default=json)");
    exit(1);
}

extern OPTARG_T optarg;
extern int optind, opterr, optopt;

#ifdef _WIN32
OPTARG_T optarg = 0;
int opterr = 1;
int optind = 1;
int optopt = 0;
int getopt(int argc, OPTARG_T *argv, OPTARG_T opts) {

    static int sp = 1;
    register int c;
    register OPTARG_T cp;
    
    if(sp == 1)
        if(optind >= argc ||
             argv[optind][0] != '-' || argv[optind][1] == '\0')
            return(EOF);
        else if(wcscmp(argv[optind], L"--") == NULL) {
            optind++;
            return(EOF);
        }
    optopt = c = argv[optind][sp];
    if(c == ':' || (cp=wcschr(opts, c)) == NULL) {
        ERR(L": illegal option -- ", c);
        if(argv[optind][++sp] == '\0') {
            optind++;
            sp = 1;
        }
        return('?');
    }
    if(*++cp == ':') {
        if(argv[optind][sp+1] != '\0')
            optarg = &argv[optind++][sp+1];
        else if(++optind >= argc) {
            ERR(L": option requires an argument -- ", c);
            sp = 1;
            return('?');
        } else
            optarg = argv[optind++];
        sp = 1;
    } else {
        if(argv[optind][++sp] == '\0') {
            sp = 1;
            optind++;
        }
        optarg = NULL;
    }
    return(c);
}
#define ARGS (OPTARG_T)L"i:o:-rh"
#else
#define ARGS "i:o:-rh"
#endif

struct Document {
    std::string type;
    std::string text;
};

static void document_to_json(Document& document, std::string& text, bool rawText) {
    
    if(rawText){
        text = document.text;
    }else{
        Json::Value documentNode(Json::objectValue);
        documentNode["type"] = document.type;
        documentNode["text"] = document.text;

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        text = Json::writeString(writer, documentNode);
    }
}

static void print_text(TidyDoc tdoc, TidyNode tnode, std::string& text) {
    
    for (TidyNode child = tidyGetChild(tnode); child; child = tidyGetNext(child)) {
        TidyNodeType ttype = tidyNodeGetType(child);
        if (ttype == TidyNode_Text) {
            TidyBuffer buf;
            tidyBufInit(&buf);
            tidyNodeGetValue(tdoc, child, &buf);
            text += std::string((char*)buf.bp, buf.size);
            tidyBufFree(&buf);
        } else if (ttype == TidyNode_Start) {
            print_text(tdoc, child, text);
        }
    }
}

int main(int argc, OPTARG_T argv[]) {
        
    const OPTARG_T input_path  = NULL;
    const OPTARG_T output_path = NULL;
    
    std::vector<unsigned char>html_data(0);

    int ch;
    std::string text;
    bool rawText = false;
    
    while ((ch = getopt(argc, argv, ARGS)) != -1){
        switch (ch){
            case 'i':
                input_path  = optarg;
                break;
            case 'o':
                output_path = optarg;
                break;
            case '-':
            {
                std::vector<uint8_t> buf(BUFLEN);
                size_t n;
                
                while ((n = fread(buf.data(), 1, buf.size(), stdin)) > 0) {
                    html_data.insert(html_data.end(), buf.begin(), buf.begin() + n);
                }
            }
                break;
            case 'r':
                rawText = true;
                break;
            case 'h':
            default:
                usage();
                break;
        }
    }
        
    if((!html_data.size()) && (input_path != NULL)) {
        FILE *f = _fopen(input_path, _rb);
        if(f) {
            _fseek(f, 0, SEEK_END);
            size_t len = (size_t)_ftell(f);
            _fseek(f, 0, SEEK_SET);
            html_data.resize(len);
            fread(html_data.data(), 1, html_data.size(), f);
            fclose(f);
        }
    }
    
    if(!html_data.size()) {
        usage();
    }
    
    Document document;
    
    TidyDoc tdoc = tidyCreate();
    TidyBuffer errbuf;
    tidyBufInit(&errbuf);
    
    tidyOptSetBool(tdoc, TidyXhtmlOut, yes);
    tidyOptSetBool(tdoc, TidyXmlOut, no);
    tidyOptSetBool(tdoc, TidyForceOutput, yes);
    
    tidyOptSetBool(tdoc, TidyQuiet, yes);
    tidyOptSetBool(tdoc, TidyShowWarnings, no);
    tidySetErrorBuffer(tdoc, &errbuf);

    tidyOptSetValue(tdoc, TidyCustomTags, "blocklevel");
    tidyOptSetValue(tdoc, TidyDoctype, "auto");
    
    tidyOptSetBool(tdoc, TidyMark, no);
    tidyOptSetInt(tdoc, TidyWrapLen, 0);
    tidyOptSetBool(tdoc, TidyDropEmptyElems, yes);
    tidyOptSetBool(tdoc, TidyDropEmptyParas, yes);
    tidyOptSetBool(tdoc, TidyDropPropAttrs, yes);

    tidyOptSetBool(tdoc, TidyIndentContent, no);
    tidyOptSetInt(tdoc, TidyIndentSpaces, 0);

    tidyOptSetBool(tdoc, TidyQuoteAmpersand, no);
    tidyOptSetBool(tdoc, TidyAsciiChars, no);
    tidyOptSetBool(tdoc, TidyPreserveEntities, no);
    tidyOptSetBool(tdoc, TidyNumEntities, yes);
    
    if(tidyParseString(tdoc, (const char *)html_data.data()) >= 0) {
        document.type = "html";
        if(tidyCleanAndRepair(tdoc) >= 0) {
            TidyNode body = tidyGetBody(tdoc);
            print_text(tdoc, body, document.text);
        }
    }
    
    tidyRelease(tdoc);
    tidyBufFree(&errbuf);
    
    document_to_json(document, text, rawText);
        
    if(!output_path) {
        std::cout << text << std::endl;
    }else{
        FILE *f = _fopen(output_path, _wb);
        if(f) {
            fwrite(text.c_str(), 1, text.length(), f);
            fclose(f);
        }
    }

    end:
        
    return 0;
}
