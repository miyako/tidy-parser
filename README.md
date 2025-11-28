![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/tidy-parser)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/tidy-parser/total)

### Dependencies and Licensing

* the source code of this CLI tool is licensed under the MIT license.
* see [tidy-html5](https://github.com/htacg/tidy-html5/blob/next/README/LICENSE.md) for the licensing of **tidy-html5** (W3C).
 
# tidy-parser
CLI tool to extract text from HTML

```
text extractor for html documents

 -i path: document to parse
 -o path: text output (default=stdout)
 -: use stdin for input
 -r: raw text output (default=json)
```

## JSON

|Property|Level|Type|Description|
|-|-|-|-|
|document|0|||
|document.type|0|Text||
|document.pages|0|Array||
|document.pages[].paragraphs|1|Array||
|document.pages[].paragraphs[].text|2|Text||
