# Nanoize++ - A simple C++ library to minimize HTML

Quick and dirty dependency-free C++ 20 library to minimize HTML. It removes comments, unnecessary spaces and newlines. As well as fixing some common mistakes base on the HTML5 standard.

The author created this library to minimize the HTML transmitted from [his blog](https://clehaxze.tw/blog). Contributions are welcome.

## Features

- Dependency-free
- Written in safe C++
- Remove comments
- Remove unnecessary spaces and newlines
- Unlike some minizeers, Nanoize++ conserves HTML tag sementics
- Fix common HTML errors that affects the AST

### Limitations

- Supports leaving CDATA as is. But no one uses it so no guarantee
- HTML5 only. No previous HTML versions or XHTML support

## Usage

There's only one API - `nanoizepp::minimize`. It takes your HTML string and returns a minimized version of it.

```cpp
#include <nanoizepp/nanoizepp.hpp>

int main() {
    std::string html = "<html>  <head>    <title>Test</title>  </head>  <body>    <h1>Test</h1>  </body></html>";
    std::string minimized = nanoizepp::minimize(html);
    std::cout << minimized << std::endl;
}
```

Output:

```html
<html><head><title>Test</title></head><body><h1>Test</h1></body></html>
```
