#include <nanoizepp/nanoizepp.hpp>
#include <iostream>

int main()
{
    std::string html = 
R"(
<!DOCTYPE html>
<html>
    <head>
        <title>Test</title>
    </head>
    <body>
        <h1>Test</h1>
        <p>Test</p>
    </body>
</html>
)";
    std::string miniaturized = nanoizepp::nanoize(html);
    return 0;
}